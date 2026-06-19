#include "bsp.h"

#if CONFIG_FOC_ENABLE

#include "cmsis_os.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define FOC_TWO_PI            (2.0f * M_PI)
#define FOC_LOOP_DT_S         (1.0f / (float)CONFIG_FOC_LOOP_HZ)
#define FOC_SQRT3             1.7320508075688772f
#define FOC_INV_SQRT3         0.5773502691896258f
#define FOC_PWM_MIN_DUTY      0.02f
#define FOC_PWM_MAX_DUTY      0.98f
#define FOC_DEFAULT_POLE_PAIRS 7.0f
#define FOC_DEFAULT_VBUS      24.0f
#define FOC_PLL_LOCK_RPM        120.0f
#define FOC_PLL_LOCK_BEMF       0.15f

typedef struct {
    float kp;
    float ki;
    float integral;
    float out_min;
    float out_max;
} foc_pi_t;

typedef struct {
    foc_pi_t id;
    foc_pi_t iq;
    foc_pi_t speed;
    foc_pi_t pll;

    float i_hat_alpha;
    float i_hat_beta;
    float e_alpha;
    float e_beta;

    float theta_elec;
    float omega_elec;
    float theta_open_loop;

    float rpm_target;
    float open_loop_omega;
    float last_v_alpha;
    float last_v_beta;

    bldc_foc_mode_t mode;
    uint32_t align_until_ms;
    uint32_t ramp_until_ms;

    uint8_t pwm_ready;
    uint8_t driver_ready;
} foc_runtime_t;

#if CONFIG_STM32_FAMILY_F4
#define FOC_LOOP_TIM          TIM2
#define FOC_LOOP_TIM_IRQn     TIM2_IRQn
#define FOC_LOOP_TIM_CLK_EN() __HAL_RCC_TIM2_CLK_ENABLE()
#elif CONFIG_STM32_FAMILY_G4
#define FOC_LOOP_TIM          TIM6
#define FOC_LOOP_TIM_IRQn     TIM6_DAC_IRQn
#define FOC_LOOP_TIM_CLK_EN() __HAL_RCC_TIM6_CLK_ENABLE()
#else
#error "FOC loop timer not defined for this STM32 family"
#endif

static TIM_HandleTypeDef htim_foc_loop;
static TaskHandle_t foc_task_handle;

static void foc_loop_notify_from_isr(void)
{
    BaseType_t higher_priority_woken = pdFALSE;

    if (foc_task_handle == NULL) {
        return;
    }

    vTaskNotifyGiveFromISR(foc_task_handle, &higher_priority_woken);
    portYIELD_FROM_ISR(higher_priority_woken);
}

static foc_runtime_t foc;
static bldc_foc_state_t foc_state;

static void foc_begin_startup(const bldc_settings_t *settings);

static uint32_t foc_timer_clock_hz(void)
{
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    uint32_t ppre1;

#if CONFIG_STM32_FAMILY_F4
    ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;
#elif CONFIG_STM32_FAMILY_G4
    ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;
#else
    ppre1 = 0U;
#endif

    if (ppre1 >= 4U) {
        return pclk1 * 2U;
    }

    return pclk1;
}

static void bldc_foc_timer_init(void)
{
    const uint32_t timclk = foc_timer_clock_hz();
    const uint32_t target_hz = (uint32_t)CONFIG_FOC_LOOP_HZ;
    uint32_t prescaler = 0U;
    uint32_t period;

    if (target_hz == 0U || timclk < target_hz) {
        Error_Handler();
    }

    period = (timclk / target_hz);
    if (period > 0U) {
        period -= 1U;
    }

    while (period > 65535U) {
        prescaler++;
        period = (timclk / (target_hz * (prescaler + 1U)));
        if (period > 0U) {
            period -= 1U;
        }
        if (prescaler > 65535U) {
            Error_Handler();
        }
    }

    FOC_LOOP_TIM_CLK_EN();

    htim_foc_loop.Instance = FOC_LOOP_TIM;
    htim_foc_loop.Init.Prescaler = (uint16_t)prescaler;
    htim_foc_loop.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim_foc_loop.Init.Period = (uint16_t)period;
    htim_foc_loop.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim_foc_loop) != HAL_OK) {
        Error_Handler();
    }

    HAL_NVIC_SetPriority(FOC_LOOP_TIM_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(FOC_LOOP_TIM_IRQn);

    if (HAL_TIM_Base_Start_IT(&htim_foc_loop) != HAL_OK) {
        Error_Handler();
    }
}

void bldc_foc_hal_period_callback(TIM_HandleTypeDef *htim)
{
    BaseType_t higher_priority_woken = pdFALSE;

    if (htim->Instance != FOC_LOOP_TIM || foc_task_handle == NULL) {
        return;
    }

    vTaskNotifyGiveFromISR(foc_task_handle, &higher_priority_woken);
    portYIELD_FROM_ISR(higher_priority_woken);
}

static float foc_cos(float radians)
{
    return bsp_sin_f(radians + (0.5f * M_PI));
}

static float foc_wrap_angle(float radians)
{
    while (radians >= M_PI) {
        radians -= FOC_TWO_PI;
    }
    while (radians < -M_PI) {
        radians += FOC_TWO_PI;
    }
    return radians;
}

static float foc_sat(float value, float limit)
{
    if (value > limit) {
        return limit;
    }
    if (value < -limit) {
        return -limit;
    }
    return value;
}

static void foc_pi_init(foc_pi_t *pi, float kp, float ki, float out_min, float out_max)
{
    pi->kp = kp;
    pi->ki = ki;
    pi->integral = 0.0f;
    pi->out_min = out_min;
    pi->out_max = out_max;
}

static float foc_pi_step(foc_pi_t *pi, float error, float dt)
{
    pi->integral += error * dt;
    pi->integral = foc_sat(pi->integral, pi->out_max);
    const float out = (pi->kp * error) + (pi->ki * pi->integral);
    return foc_sat(out, pi->out_max);
}

static void foc_clarke(float ia, float ib, float ic, float *i_alpha, float *i_beta)
{
    (void)ic;
    *i_alpha = ia;
    *i_beta = FOC_INV_SQRT3 * (ia + (2.0f * ib));
}

static void foc_park(float i_alpha, float i_beta, float theta, float *id, float *iq)
{
    const float c = foc_cos(theta);
    const float s = bsp_sin_f(theta);
    *id = (i_alpha * c) + (i_beta * s);
    *iq = (-i_alpha * s) + (i_beta * c);
}

static void foc_inv_park(float vd, float vq, float theta, float *v_alpha, float *v_beta)
{
    const float c = foc_cos(theta);
    const float s = bsp_sin_f(theta);
    *v_alpha = (vd * c) - (vq * s);
    *v_beta = (vd * s) + (vq * c);
}

static void foc_inv_clarke(float v_alpha, float v_beta, float *va, float *vb, float *vc)
{
    *va = v_alpha;
    *vb = (-0.5f * v_alpha) + (0.5f * FOC_SQRT3 * v_beta);
    *vc = (-0.5f * v_alpha) - (0.5f * FOC_SQRT3 * v_beta);
}

static float foc_effective_pole_pairs(const bldc_settings_t *settings)
{
    if (settings != NULL && settings->pole_pairs >= 1.0f) {
        return settings->pole_pairs;
    }
    return FOC_DEFAULT_POLE_PAIRS;
}

static float foc_effective_vbus(const bldc_settings_t *settings, float measured_vbus)
{
    (void)settings;

    if (measured_vbus > 5.0f) {
        return measured_vbus;
    }

    return FOC_DEFAULT_VBUS;
}

static float foc_rpm_to_omega_elec(float rpm, float pole_pairs)
{
    return (rpm * FOC_TWO_PI * pole_pairs) / 60.0f;
}

static float foc_omega_elec_to_rpm(float omega, float pole_pairs)
{
    if (pole_pairs < 1.0f) {
        return 0.0f;
    }
    return (omega * 60.0f) / (FOC_TWO_PI * pole_pairs);
}

static void foc_ensure_driver_ready(void)
{
    if (foc.driver_ready) {
        return;
    }

    bldc_drv8323r_init();
    foc.driver_ready = 1U;
}

static int foc_gate_driver_ok(void)
{
    const uint32_t faults = bldc_drv8323r_read_faults();
    if (faults == 0U) {
        return 1;
    }

    bldc_drv8323r_reset_faults();
    foc.mode = BLDC_FOC_MODE_FAULT;
    return 0;
}

static void foc_pwm_start(void)
{
    if (foc.pwm_ready) {
        return;
    }

    (void)HAL_TIM_PWM_Start(bldc_h.htim_high, bldc_h.chA);
    (void)HAL_TIM_PWM_Start(bldc_h.htim_high, bldc_h.chB);
    (void)HAL_TIM_PWM_Start(bldc_h.htim_high, bldc_h.chC);

#if CONFIG_BLDC_PWM_TIMER_HIGH_IS_ADVANCED
    (void)HAL_TIMEx_PWMN_Start(bldc_h.htim_high, bldc_h.chA);
    (void)HAL_TIMEx_PWMN_Start(bldc_h.htim_high, bldc_h.chB);
    (void)HAL_TIMEx_PWMN_Start(bldc_h.htim_high, bldc_h.chC);
    __HAL_TIM_MOE_ENABLE(bldc_h.htim_high);
#endif

    TIM_TypeDef *tim = bldc_h.htim_high->Instance;
    tim->CCER |= (TIM_CCER_CC1E | TIM_CCER_CC1NE |
                  TIM_CCER_CC2E | TIM_CCER_CC2NE |
                  TIM_CCER_CC3E | TIM_CCER_CC3NE);

    foc.pwm_ready = 1U;
}

static void foc_pwm_disable(void)
{
    TIM_TypeDef *tim = bldc_h.htim_high->Instance;

#if CONFIG_BLDC_PWM_TIMER_HIGH_IS_ADVANCED
    __HAL_TIM_MOE_DISABLE(bldc_h.htim_high);
#endif

    tim->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC1NE |
                   TIM_CCER_CC2E | TIM_CCER_CC2NE |
                   TIM_CCER_CC3E | TIM_CCER_CC3NE);

    __HAL_TIM_SET_COMPARE(bldc_h.htim_high, bldc_h.chA, 0U);
    __HAL_TIM_SET_COMPARE(bldc_h.htim_high, bldc_h.chB, 0U);
    __HAL_TIM_SET_COMPARE(bldc_h.htim_high, bldc_h.chC, 0U);

    foc.pwm_ready = 0U;
}

static void foc_apply_phase_voltages(float va, float vb, float vc, float vbus)
{
    const float period = (float)(__HAL_TIM_GET_AUTORELOAD(bldc_h.htim_high) + 1U);
    float da;
    float db;
    float dc;

    if (vbus < 1.0f || period < 1.0f) {
        foc_pwm_disable();
        return;
    }

    foc_pwm_start();

    da = 0.5f + (0.5f * (va / vbus));
    db = 0.5f + (0.5f * (vb / vbus));
    dc = 0.5f + (0.5f * (vc / vbus));

    da = foc_sat(da, FOC_PWM_MAX_DUTY);
    db = foc_sat(db, FOC_PWM_MAX_DUTY);
    dc = foc_sat(dc, FOC_PWM_MAX_DUTY);

    if (da < FOC_PWM_MIN_DUTY) da = FOC_PWM_MIN_DUTY;
    if (db < FOC_PWM_MIN_DUTY) db = FOC_PWM_MIN_DUTY;
    if (dc < FOC_PWM_MIN_DUTY) dc = FOC_PWM_MIN_DUTY;

    __HAL_TIM_SET_COMPARE(bldc_h.htim_high, bldc_h.chA, (uint16_t)(da * period));
    __HAL_TIM_SET_COMPARE(bldc_h.htim_high, bldc_h.chB, (uint16_t)(db * period));
    __HAL_TIM_SET_COMPARE(bldc_h.htim_high, bldc_h.chC, (uint16_t)(dc * period));
}

static void foc_sync_controllers_from_settings(const bldc_settings_t *settings)
{
    if (settings == NULL) {
        return;
    }

    if (settings->current_kp > 0.0f) {
        foc.id.kp = settings->current_kp;
        foc.iq.kp = settings->current_kp;
    }
    if (settings->current_ki > 0.0f) {
        foc.id.ki = settings->current_ki;
        foc.iq.ki = settings->current_ki;
    }
    if (settings->speed_kp > 0.0f) {
        foc.speed.kp = settings->speed_kp;
    }
    if (settings->speed_ki > 0.0f) {
        foc.speed.ki = settings->speed_ki;
    }
    if (settings->pll_kp > 0.0f) {
        foc.pll.kp = settings->pll_kp;
    }
    if (settings->pll_ki > 0.0f) {
        foc.pll.ki = settings->pll_ki;
    }
}

static void foc_observer_reset(const bldc_settings_t *settings)
{
    const float gain = (settings != NULL && settings->observer_gain > 0.0f)
                           ? settings->observer_gain
                           : 25.0f;
    const float pll_kp = (settings != NULL && settings->pll_kp > 0.0f)
                             ? settings->pll_kp
                             : 80.0f;
    const float pll_ki = (settings != NULL && settings->pll_ki > 0.0f)
                             ? settings->pll_ki
                             : 2500.0f;

    foc.i_hat_alpha = 0.0f;
    foc.i_hat_beta = 0.0f;
    foc.e_alpha = 0.0f;
    foc.e_beta = 0.0f;
    foc.theta_elec = 0.0f;
    foc.omega_elec = 0.0f;
    foc.theta_open_loop = 0.0f;

    foc_pi_init(&foc.id,
                (settings != NULL && settings->current_kp > 0.0f) ? settings->current_kp : 0.8f,
                (settings != NULL && settings->current_ki > 0.0f) ? settings->current_ki : 120.0f,
                -0.95f * FOC_DEFAULT_VBUS,
                0.95f * FOC_DEFAULT_VBUS);
    foc_pi_init(&foc.iq,
                (settings != NULL && settings->current_kp > 0.0f) ? settings->current_kp : 0.8f,
                (settings != NULL && settings->current_ki > 0.0f) ? settings->current_ki : 120.0f,
                -0.95f * FOC_DEFAULT_VBUS,
                0.95f * FOC_DEFAULT_VBUS);
    foc_pi_init(&foc.speed,
                (settings != NULL && settings->speed_kp > 0.0f) ? settings->speed_kp : 0.0025f,
                (settings != NULL && settings->speed_ki > 0.0f) ? settings->speed_ki : 0.05f,
                -20.0f,
                20.0f);
    foc_pi_init(&foc.pll, pll_kp, pll_ki, -4000.0f, 4000.0f);

    (void)gain;
}

static void foc_observer_step(const bldc_settings_t *settings,
                              float ia,
                              float ib,
                              float ic,
                              float v_alpha,
                              float v_beta,
                              float dt)
{
    const float rs = (settings != NULL && settings->phase_resistance > 0.0f)
                         ? settings->phase_resistance
                         : 0.05f;
    const float ls = (settings != NULL && settings->phase_inductance > 1e-6f)
                         ? settings->phase_inductance
                         : 8.0e-6f;
    const float lambda = (settings != NULL && settings->observer_gain > 0.0f)
                             ? settings->observer_gain
                             : 25.0f;
    float i_alpha;
    float i_beta;
    float i_err_alpha;
    float i_err_beta;
    float pll_err;

    foc_clarke(ia, ib, ic, &i_alpha, &i_beta);

    i_err_alpha = i_alpha - foc.i_hat_alpha;
    i_err_beta = i_beta - foc.i_hat_beta;

    foc.e_alpha += lambda * i_err_alpha * dt;
    foc.e_beta += lambda * i_err_beta * dt;

    foc.i_hat_alpha += ((v_alpha - (rs * foc.i_hat_alpha) - foc.e_alpha) / ls) * dt;
    foc.i_hat_beta += ((v_beta - (rs * foc.i_hat_beta) - foc.e_beta) / ls) * dt;

    pll_err = (-foc.e_alpha * foc_cos(foc.theta_elec)) - (foc.e_beta * bsp_sin_f(foc.theta_elec));
    foc.omega_elec += foc_pi_step(&foc.pll, pll_err, dt) * dt;
    foc.theta_elec = foc_wrap_angle(foc.theta_elec + (foc.omega_elec * dt));

    foc_state.i_alpha = i_alpha;
    foc_state.i_beta = i_beta;
    foc_state.bemf_alpha = foc.e_alpha;
    foc_state.bemf_beta = foc.e_beta;
    foc_state.bemf_magnitude = sqrtf((foc.e_alpha * foc.e_alpha) + (foc.e_beta * foc.e_beta));
    foc_state.theta_elec_rad = foc.theta_elec;
    foc_state.omega_elec_rad_s = foc.omega_elec;
}

static void foc_run_current_controller(const bldc_settings_t *settings,
                                       float ia,
                                       float ib,
                                       float ic,
                                       float theta,
                                       float iq_target,
                                       float vbus,
                                       float dt,
                                       float *va,
                                       float *vb,
                                       float *vc)
{
    float i_alpha;
    float i_beta;
    float id;
    float iq;
    float vd;
    float vq;
    float v_alpha;
    float v_beta;
    const float rs = (settings != NULL && settings->phase_resistance > 0.0f)
                         ? settings->phase_resistance
                         : 0.05f;
    const float ls = (settings != NULL && settings->phase_inductance > 1e-6f)
                         ? settings->phase_inductance
                         : 8.0e-6f;
    const float id_target = 0.0f;
    const float pole_pairs = foc_effective_pole_pairs(settings);
    const float omega = foc.omega_elec;

    foc_clarke(ia, ib, ic, &i_alpha, &i_beta);
    foc_park(i_alpha, i_beta, theta, &id, &iq);

    vd = foc_pi_step(&foc.id, id_target - id, dt);
    vq = foc_pi_step(&foc.iq, iq_target - iq, dt);

    vd -= omega * ls * iq;
    vq += omega * ls * id;
    (void)rs;

    foc_inv_park(vd, vq, theta, &v_alpha, &v_beta);
    foc.last_v_alpha = v_alpha;
    foc.last_v_beta = v_beta;
    foc_inv_clarke(v_alpha, v_beta, va, vb, vc);

    foc_state.i_d = id;
    foc_state.i_q = iq;
    foc_state.v_d = vd;
    foc_state.v_q = vq;

    (void)vbus;
    (void)pole_pairs;
}

static float foc_alignment_current(const bldc_settings_t *settings)
{
    if (settings != NULL && settings->alignment_current > 0.0f) {
        return settings->alignment_current;
    }

    return 2.0f;
}

static float foc_open_loop_current(const bldc_settings_t *settings)
{
    float iq = 0.0f;

    if (settings != NULL && settings->open_loop_current > 0.0f) {
        iq = settings->open_loop_current;
    } else {
        iq = foc_alignment_current(settings);
    }

    if (settings != NULL && settings->max_phase_current > 0.0f) {
        iq = foc_sat(iq, settings->max_phase_current);
    }

    return iq;
}

static float foc_open_loop_start_rpm(const bldc_settings_t *settings)
{
    const float min_cl = (settings != NULL && settings->min_rpm_closed_loop > 0.0f)
                             ? settings->min_rpm_closed_loop
                             : 500.0f;
    float start_rpm;

    if (settings != NULL && settings->open_loop_start_rpm > 0.0f) {
        start_rpm = settings->open_loop_start_rpm;
    } else {
        start_rpm = 150.0f;
    }

    if (start_rpm > min_cl) {
        start_rpm = min_cl;
    }

    if (settings != NULL && settings->max_rpm_open_loop > 0.0f &&
        start_rpm > settings->max_rpm_open_loop) {
        start_rpm = settings->max_rpm_open_loop;
    }

    return start_rpm;
}

static void foc_begin_open_loop(const bldc_settings_t *settings, uint32_t now_ms)
{
    const float pole_pairs = foc_effective_pole_pairs(settings);
    const float start_rpm = foc_open_loop_start_rpm(settings);

    foc.mode = BLDC_FOC_MODE_OPEN_LOOP;
    foc.ramp_until_ms = now_ms + (uint32_t)((settings != NULL && settings->startup_ramp_time_ms > 0.0f)
                                                ? settings->startup_ramp_time_ms
                                                : 500.0f);
    foc.open_loop_omega = foc_rpm_to_omega_elec(start_rpm, pole_pairs);
    foc.theta_open_loop = 0.0f;
}

static void foc_enter_closed_loop_from_open_loop(void)
{
    foc.mode = BLDC_FOC_MODE_CLOSED_LOOP;
    foc.theta_elec = foc.theta_open_loop;
    foc.omega_elec = foc.open_loop_omega;
    foc.pll.integral = foc.omega_elec;
    foc.speed.integral = 0.0f;
}

static int foc_handoff_ready(const bldc_settings_t *settings)
{
    const float pole_pairs = foc_effective_pole_pairs(settings);
    const float min_rpm = (settings != NULL && settings->min_rpm_closed_loop > 0.0f)
                              ? settings->min_rpm_closed_loop
                              : 500.0f;
    const float ol_rpm = foc_omega_elec_to_rpm(foc.open_loop_omega, pole_pairs);
    const float max_ang_deg = (settings != NULL && settings->handoff_angle_err_deg > 0.0f)
                                  ? settings->handoff_angle_err_deg
                                  : 25.0f;
    const float max_ang_rad = max_ang_deg * (M_PI / 180.0f);
    uint8_t min_conf = (settings != NULL && settings->handoff_min_confidence > 0.0f)
                           ? (uint8_t)settings->handoff_min_confidence
                           : 55U;

    if (min_conf > 100U) {
        min_conf = 100U;
    }

    if (ol_rpm < min_rpm) {
        return 0;
    }

    if (!foc_state.pll_locked) {
        return 0;
    }

    if (foc_state.confidence < min_conf) {
        return 0;
    }

    if (fabsf(foc_state.angle_error_rad) > max_ang_rad) {
        return 0;
    }

    return 1;
}

static void foc_update_mode(const bldc_settings_t *settings)
{
    const uint32_t now_ms = millis32();

    switch (foc.mode) {
    case BLDC_FOC_MODE_ALIGN:
        if (now_ms >= foc.align_until_ms) {
            foc_begin_open_loop(settings, now_ms);
        }
        break;

    case BLDC_FOC_MODE_OPEN_LOOP:
        if (foc_handoff_ready(settings)) {
            foc_enter_closed_loop_from_open_loop();
        }
        break;

    case BLDC_FOC_MODE_FAULT:
    case BLDC_FOC_MODE_IDLE:
    case BLDC_FOC_MODE_CLOSED_LOOP:
    default:
        break;
    }
}

static void foc_step_once(void)
{
    bldc_settings_t *settings = bldc_get_settings();
    float ia = 0.0f;
    float ib = 0.0f;
    float ic = 0.0f;
    float vbus = 0.0f;
    float va = 0.0f;
    float vb = 0.0f;
    float vc = 0.0f;
    float theta;
    float iq_target = 0.0f;
    const float pole_pairs = foc_effective_pole_pairs(settings);
    const float dt = FOC_LOOP_DT_S;

    if (!bsp_foc_sample_sensors(&ia, &ib, &ic, &vbus)) {
        return;
    }

    if (!foc_gate_driver_ok()) {
        foc_pwm_disable();
        foc_state.mode = BLDC_FOC_MODE_FAULT;
        return;
    }

    vbus = foc_effective_vbus(settings, vbus);
    foc_state.rpm_target = foc.rpm_target;

    if (foc.rpm_target <= 0.0f) {
        if (foc.mode != BLDC_FOC_MODE_IDLE && foc.mode != BLDC_FOC_MODE_FAULT) {
            foc.mode = BLDC_FOC_MODE_IDLE;
            foc_state.mode = BLDC_FOC_MODE_IDLE;
        }
        foc_pwm_disable();
        return;
    }

    foc_update_mode(settings);

    switch (foc.mode) {
    case BLDC_FOC_MODE_ALIGN: {
        const float align_current = foc_alignment_current(settings);
        theta = 0.0f;
        foc_run_current_controller(settings, ia, ib, ic, theta, align_current, vbus, dt, &va, &vb, &vc);
        foc.theta_open_loop = theta;
        foc_observer_step(settings, ia, ib, ic, foc.last_v_alpha, foc.last_v_beta, dt);
        break;
    }

    case BLDC_FOC_MODE_OPEN_LOOP: {
        const uint32_t now_ms = millis32();
        const float ramp_rpm_s =
            (settings != NULL && settings->open_loop_ramp_rpm_s > 0.0f)
                ? settings->open_loop_ramp_rpm_s
                : 60.0f;
        const float max_ol_rpm =
            (settings != NULL && settings->max_rpm_open_loop > 0.0f)
                ? settings->max_rpm_open_loop
                : 1200.0f;
        const float max_ol_omega = foc_rpm_to_omega_elec(max_ol_rpm, pole_pairs);

        if (now_ms < foc.ramp_until_ms) {
            foc.open_loop_omega += foc_rpm_to_omega_elec(ramp_rpm_s, pole_pairs) * dt;
            if (foc.open_loop_omega > max_ol_omega) {
                foc.open_loop_omega = max_ol_omega;
            }
        }
        foc.theta_open_loop = foc_wrap_angle(foc.theta_open_loop + (foc.open_loop_omega * dt));
        theta = foc.theta_open_loop;
        iq_target = foc_open_loop_current(settings);
        foc_run_current_controller(settings, ia, ib, ic, theta, iq_target, vbus, dt, &va, &vb, &vc);
        foc_observer_step(settings, ia, ib, ic, foc.last_v_alpha, foc.last_v_beta, dt);
        break;
    }

    case BLDC_FOC_MODE_CLOSED_LOOP: {
        theta = foc.theta_elec;
        iq_target = foc_pi_step(&foc.speed,
                                foc_rpm_to_omega_elec(foc.rpm_target, pole_pairs) - foc.omega_elec,
                                dt);
        if (settings != NULL && settings->max_phase_current > 0.0f) {
            iq_target = foc_sat(iq_target, settings->max_phase_current);
        }
        foc_run_current_controller(settings, ia, ib, ic, theta, iq_target, vbus, dt, &va, &vb, &vc);
        foc_observer_step(settings, ia, ib, ic, foc.last_v_alpha, foc.last_v_beta, dt);
        break;
    }

    case BLDC_FOC_MODE_FAULT:
        foc_pwm_disable();
        return;

    case BLDC_FOC_MODE_IDLE:
    default:
        foc_pwm_disable();
        return;
    }

    foc_apply_phase_voltages(va, vb, vc, vbus);

    foc_state.mode = foc.mode;
    foc_state.rpm_actual = foc_omega_elec_to_rpm(foc.omega_elec, pole_pairs);
    foc_state.angle_error_rad = foc_wrap_angle(foc.theta_open_loop - foc.theta_elec);
    foc_state.pll_locked = (foc_state.rpm_actual >= FOC_PLL_LOCK_RPM &&
                            foc_state.bemf_magnitude >= FOC_PLL_LOCK_BEMF)
                               ? 1U
                               : 0U;
    {
        int confidence = (int)(60.0f + (foc_state.bemf_magnitude * 80.0f));
        if (confidence < 0) confidence = 0;
        if (confidence > 100) confidence = 100;
        foc_state.confidence = (uint8_t)confidence;
    }
}

void bldc_foc_init(void)
{
    memset(&foc, 0, sizeof(foc));
    memset(&foc_state, 0, sizeof(foc_state));
    foc_observer_reset(bldc_get_settings());
    foc.mode = BLDC_FOC_MODE_IDLE;
    foc_state.mode = BLDC_FOC_MODE_IDLE;
    foc_state.confidence = 0U;
}

void bldc_foc_reset(void)
{
    foc_pwm_disable();
    foc_observer_reset(bldc_get_settings());
    foc.mode = BLDC_FOC_MODE_IDLE;
    foc_state.mode = BLDC_FOC_MODE_IDLE;
}

void bldc_foc_set_target_rpm(float rpm)
{
    foc.rpm_target = rpm;
    foc_state.rpm_target = rpm;
}

void bldc_foc_apply_settings(void)
{
    bldc_settings_t *settings = bldc_get_settings();

    if (settings == NULL) {
        return;
    }

    foc_sync_controllers_from_settings(settings);
    bldc_foc_set_target_rpm(settings->rpm_target);

    if (settings->rpm_target <= 0.0f) {
        foc_pwm_disable();
        foc.mode = BLDC_FOC_MODE_IDLE;
        foc_state.mode = BLDC_FOC_MODE_IDLE;
        return;
    }

    if (foc.mode == BLDC_FOC_MODE_IDLE || foc.mode == BLDC_FOC_MODE_FAULT) {
        foc_begin_startup(settings);
    }
}

void bldc_foc_get_state(bldc_foc_state_t *state)
{
    if (state == NULL) {
        return;
    }
    *state = foc_state;
}

void bldc_foc_fill_telemetry(bldc_telemetry_t *telem)
{
    const bldc_settings_t *settings = bldc_get_settings();
    const float pole_pairs = foc_effective_pole_pairs(settings);

    if (telem == NULL) {
        return;
    }

    telem->rpm_actual = foc_state.rpm_actual;
    telem->rpm_target = foc_state.rpm_target;
    telem->i_d = foc_state.i_d;
    telem->i_q = foc_state.i_q;
    telem->angle_electrical = foc_state.theta_elec_rad * (180.0f / M_PI);
    telem->angle_mechanical = telem->angle_electrical / pole_pairs;
    {
        int bemf = (int)(foc_state.bemf_magnitude * 40.0f);
        int ang_err = (int)(fabsf(foc_state.angle_error_rad) * (180.0f / M_PI));
        if (bemf < 0) bemf = 0;
        if (bemf > 255) bemf = 255;
        if (ang_err < 0) ang_err = 0;
        if (ang_err > 255) ang_err = 255;
        telem->bemf_strength = (uint8_t)bemf;
        telem->angle_error_deg = (uint8_t)ang_err;
    }
    telem->obs_confidence = foc_state.confidence;
    telem->pll_lock_status = foc_state.pll_locked;
}

static void foc_begin_startup(const bldc_settings_t *settings)
{
    const uint32_t now_ms = millis32();
    const uint8_t startup_mode = settings ? settings->startup_mode : 0U;

    foc_ensure_driver_ready();
    foc_observer_reset(settings);

    if (startup_mode == 2U) {
        const float pole_pairs = foc_effective_pole_pairs(settings);
        const float min_cl = (settings != NULL && settings->min_rpm_closed_loop > 0.0f)
                                 ? settings->min_rpm_closed_loop
                                 : 500.0f;

        foc_begin_open_loop(settings, now_ms);
        foc.open_loop_omega = foc_rpm_to_omega_elec(min_cl, pole_pairs);
    } else if (startup_mode == 1U) {
        foc_begin_open_loop(settings, now_ms);
    } else {
        const float align_ms = (settings != NULL && settings->alignment_time_ms > 0.0f)
                                   ? settings->alignment_time_ms
                                   : 150.0f;

        foc.mode = BLDC_FOC_MODE_ALIGN;
        foc.align_until_ms = now_ms + (uint32_t)align_ms;
    }

    foc_state.mode = foc.mode;
}

void bldc_foc_comm_thread(void *argument)
{
    (void)argument;

    foc_task_handle = xTaskGetCurrentTaskHandle();
    bldc_foc_init();
    bldc_foc_apply_settings();

    if (bsp_foc_loop_uses_hw_adc_trigger()) {
        bsp_foc_loop_hw_init(foc_loop_notify_from_isr);
    } else {
        bldc_foc_timer_init();
    }

    for (;;) {
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        foc_step_once();
    }
}

#if CONFIG_STM32_FAMILY_F4
void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim_foc_loop);
}
#endif

#endif /* CONFIG_FOC_ENABLE */