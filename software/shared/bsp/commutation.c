#include "bldc.h"
#include "cmsis_os.h"

BLDC_Handle_t bldc_h;

#ifndef BLDC_COMM_DEFAULT_RPM
#define BLDC_COMM_DEFAULT_RPM       300.0f
#endif

#ifndef BLDC_COMM_DEFAULT_POLE_PAIRS
#define BLDC_COMM_DEFAULT_POLE_PAIRS 7.0f
#endif

#ifndef BLDC_COMM_DUTY_PERCENT
#define BLDC_COMM_DUTY_PERCENT      25U
#endif

#ifndef BLDC_COMM_ALIGN_MS
#define BLDC_COMM_ALIGN_MS          100U
#endif

static uint16_t comm_duty;
static uint8_t comm_step;
static uint8_t pwm_started;

typedef struct {
    PhaseState phase_a;
    PhaseState phase_b;
    PhaseState phase_c;
    uint8_t pwm_phase; /* 1=A, 2=B, 3=C phase receiving PWM duty; 0 = none */
    uint8_t low_phase; /* phase held fully on through the low FET */
} CommutationStep_t;

/* Standard 6-step trapezoidal sequence: one high-side PWM, one low-side on, one float. */
static const uint32_t phase_pwm_ch[3] = {
    BLDC_PHASE1_PWM_CH, BLDC_PHASE2_PWM_CH, BLDC_PHASE3_PWM_CH,
};

static const uint32_t phase_low_ccr_ch[3] = {
    BLDC_PHASE1_LOW_CCR_CH, BLDC_PHASE2_LOW_CCR_CH, BLDC_PHASE3_LOW_CCR_CH,
};

#if BLDC_PWM_LAYOUT_COMPLEMENTARY
static const uint32_t phase_ccer_e[3] = {
    BLDC_PHASE1_CCER_E, BLDC_PHASE2_CCER_E, BLDC_PHASE3_CCER_E,
};

static const uint32_t phase_ccer_ne[3] = {
    BLDC_PHASE1_CCER_NE, BLDC_PHASE2_CCER_NE, BLDC_PHASE3_CCER_NE,
};
#endif

static const CommutationStep_t commutation_table[6] = {
    {PHASE_PWM_HIGH, PHASE_PWM_LOW,  PHASE_FLOAT,     1, 2}, /* A+ B- Cz */
    {PHASE_PWM_HIGH, PHASE_FLOAT,    PHASE_PWM_LOW,   1, 3}, /* A+ C- Bz */
    {PHASE_FLOAT,    PHASE_PWM_HIGH, PHASE_PWM_LOW,   2, 3}, /* B+ C- Az */
    {PHASE_PWM_LOW,  PHASE_PWM_HIGH, PHASE_FLOAT,     2, 1}, /* B+ A- Cz */
    {PHASE_PWM_LOW,  PHASE_FLOAT,    PHASE_PWM_HIGH,  3, 1}, /* C+ A- Bz */
    {PHASE_FLOAT,    PHASE_PWM_LOW,  PHASE_PWM_HIGH,  3, 2}, /* C+ B- Az */
};

static void ccer_masks_for_channel(uint32_t channel, uint32_t *ch_e, uint32_t *ch_ne)
{
    switch (channel) {
    case TIM_CHANNEL_1:
        *ch_e  = TIM_CCER_CC1E;
        *ch_ne = TIM_CCER_CC1NE;
        break;
    case TIM_CHANNEL_2:
        *ch_e  = TIM_CCER_CC2E;
        *ch_ne = TIM_CCER_CC2NE;
        break;
    case TIM_CHANNEL_3:
        *ch_e  = TIM_CCER_CC3E;
        *ch_ne = TIM_CCER_CC3NE;
        break;
    case TIM_CHANNEL_4:
        *ch_e  = TIM_CCER_CC4E;
        *ch_ne = 0U;
        break;
    default:
        *ch_e  = 0U;
        *ch_ne = 0U;
        break;
    }
}

#if BLDC_PWM_LAYOUT_COMPLEMENTARY
static void set_phase_complementary(TIM_TypeDef *tim,
                                    uint32_t ch_e,
                                    uint32_t ch_ne,
                                    PhaseState state)
{
    switch (state) {
    case PHASE_PWM_HIGH:
        tim->CCER |= ch_e;
        tim->CCER &= ~ch_ne;
        break;
    case PHASE_PWM_LOW:
        tim->CCER &= ~ch_e;
        tim->CCER |= ch_ne;
        break;
    case PHASE_FLOAT:
    default:
        tim->CCER &= ~(ch_e | ch_ne);
        break;
    }
}
#endif

#if BLDC_PWM_LAYOUT_DUAL
static void set_phase_single(TIM_TypeDef *tim, uint32_t ch_e, PhaseState state)
{
    switch (state) {
    case PHASE_PWM_HIGH:
    case PHASE_PWM_LOW:
        tim->CCER |= ch_e;
        break;
    case PHASE_FLOAT:
    default:
        tim->CCER &= ~ch_e;
        break;
    }
}
#endif

static void apply_gpio_low_outputs(const CommutationStep_t *cfg)
{
#if defined(BLDC_PHASE1_LOW_USE_GPIO) && BLDC_PHASE1_LOW_USE_GPIO
    HAL_GPIO_WritePin(BLDC_PHASE1_LOW_GPIO_Port, BLDC_PHASE1_LOW_GPIO_Pin,
                      cfg->phase_a == PHASE_PWM_LOW ? GPIO_PIN_SET : GPIO_PIN_RESET);
#endif
}

static void apply_phase_outputs(const CommutationStep_t *cfg)
{
#if BLDC_PWM_LAYOUT_COMPLEMENTARY
    TIM_TypeDef *tim = bldc_h.htim_high->Instance;

    set_phase_complementary(tim, phase_ccer_e[0], phase_ccer_ne[0], cfg->phase_a);
    set_phase_complementary(tim, phase_ccer_e[1], phase_ccer_ne[1], cfg->phase_b);
    set_phase_complementary(tim, phase_ccer_e[2], phase_ccer_ne[2], cfg->phase_c);
    apply_gpio_low_outputs(cfg);
#elif BLDC_PWM_LAYOUT_DUAL
    TIM_TypeDef *tim_hi = bldc_h.htim_high->Instance;
    TIM_TypeDef *tim_lo = bldc_h.htim_low->Instance;
    uint32_t e;
    uint32_t ne;

    ccer_masks_for_channel(bldc_h.chA, &e, &ne);
    (void)ne;
    set_phase_single(tim_hi, e, cfg->phase_a == PHASE_PWM_HIGH ? PHASE_PWM_HIGH : PHASE_FLOAT);
    set_phase_single(tim_lo, e, cfg->phase_a == PHASE_PWM_LOW ? PHASE_PWM_LOW : PHASE_FLOAT);

    ccer_masks_for_channel(bldc_h.chB, &e, &ne);
    set_phase_single(tim_hi, e, cfg->phase_b == PHASE_PWM_HIGH ? PHASE_PWM_HIGH : PHASE_FLOAT);
    set_phase_single(tim_lo, e, cfg->phase_b == PHASE_PWM_LOW ? PHASE_PWM_LOW : PHASE_FLOAT);

    ccer_masks_for_channel(bldc_h.chC, &e, &ne);
    set_phase_single(tim_hi, e, cfg->phase_c == PHASE_PWM_HIGH ? PHASE_PWM_HIGH : PHASE_FLOAT);
    set_phase_single(tim_lo, e, cfg->phase_c == PHASE_PWM_LOW ? PHASE_PWM_LOW : PHASE_FLOAT);
#endif
}

static void aux_channels(uint32_t out[3])
{
    out[0] = bldc_h.aux_chA ? bldc_h.aux_chA : bldc_h.chA;
    out[1] = bldc_h.aux_chB ? bldc_h.aux_chB : bldc_h.chB;
    out[2] = bldc_h.aux_chC ? bldc_h.aux_chC : bldc_h.chC;
}

static void apply_aux_outputs(const CommutationStep_t *cfg)
{
    if (bldc_h.htim_aux == NULL) {
        return;
    }

    TIM_TypeDef *tim = bldc_h.htim_aux->Instance;
    uint32_t channels[3];
    const PhaseState states[3] = {cfg->phase_a, cfg->phase_b, cfg->phase_c};
    uint32_t e;
    uint32_t unused_ne;

    aux_channels(channels);

    for (uint8_t i = 0U; i < 3U; i++) {
        ccer_masks_for_channel(channels[i], &e, &unused_ne);
        if (states[i] == PHASE_PWM_LOW) {
            tim->CCER |= e;
        } else {
            tim->CCER &= ~e;
        }
    }
}

static uint16_t comm_timer_full_duty(TIM_HandleTypeDef *htim)
{
    return (uint16_t)__HAL_TIM_GET_AUTORELOAD(htim);
}

static void apply_step_compare(const CommutationStep_t *cfg, uint16_t duty)
{
    /* Complementary NE low-side: CCR=0 keeps CHN fully on. */
    const uint16_t comp_low_ccr = 0U;

    for (uint8_t phase = 1U; phase <= 3U; phase++) {
        if (phase == cfg->pwm_phase) {
            __HAL_TIM_SET_COMPARE(bldc_h.htim_high,
                                  phase_pwm_ch[phase - 1U],
                                  duty);
        } else if (phase == cfg->low_phase) {
            __HAL_TIM_SET_COMPARE(bldc_h.htim_high,
                                  phase_low_ccr_ch[phase - 1U],
                                  comp_low_ccr);
        }
    }

#if BLDC_PWM_LAYOUT_DUAL
    const uint16_t dual_low_ccr = comm_timer_full_duty(bldc_h.htim_low);

    for (uint8_t phase = 1U; phase <= 3U; phase++) {
        uint16_t ccr = 0U;
        if (phase == cfg->pwm_phase) {
            ccr = duty;
        } else if (phase == cfg->low_phase) {
            ccr = dual_low_ccr;
        }
        __HAL_TIM_SET_COMPARE(bldc_h.htim_low, phase_pwm_ch[phase - 1U], ccr);
    }
#endif

    if (bldc_h.htim_aux != NULL) {
        uint32_t aux_channels_arr[3];
        const uint16_t aux_low_ccr = comm_timer_full_duty(bldc_h.htim_aux);

        aux_channels(aux_channels_arr);

        for (uint8_t phase = 1U; phase <= 3U; phase++) {
            uint16_t ccr = 0U;
            if (phase == cfg->low_phase) {
                ccr = aux_low_ccr;
            }
            __HAL_TIM_SET_COMPARE(bldc_h.htim_aux, aux_channels_arr[phase - 1U], ccr);
        }
    }
}

static void pwm_moe_enable(void)
{
#if CONFIG_BLDC_PWM_TIMER_HIGH_IS_ADVANCED
    __HAL_TIM_MOE_ENABLE(bldc_h.htim_high);
#endif
}

static void pwm_moe_disable(void)
{
#if CONFIG_BLDC_PWM_TIMER_HIGH_IS_ADVANCED
    __HAL_TIM_MOE_DISABLE(bldc_h.htim_high);
#endif
}

static void start_timer_channels(TIM_HandleTypeDef *htim,
                                 const uint32_t channels[3],
                                 uint8_t use_complementary)
{
    (void)HAL_TIM_PWM_Start(htim, channels[0]);
    (void)HAL_TIM_PWM_Start(htim, channels[1]);
    (void)HAL_TIM_PWM_Start(htim, channels[2]);

    if (use_complementary) {
        (void)HAL_TIMEx_PWMN_Start(htim, channels[0]);
        (void)HAL_TIMEx_PWMN_Start(htim, channels[1]);
        (void)HAL_TIMEx_PWMN_Start(htim, channels[2]);
    }
}

static void stop_timer_channels(TIM_HandleTypeDef *htim,
                                const uint32_t channels[3],
                                uint8_t use_complementary)
{
    (void)HAL_TIM_PWM_Stop(htim, channels[0]);
    (void)HAL_TIM_PWM_Stop(htim, channels[1]);
    (void)HAL_TIM_PWM_Stop(htim, channels[2]);

    if (use_complementary) {
        (void)HAL_TIMEx_PWMN_Stop(htim, channels[0]);
        (void)HAL_TIMEx_PWMN_Stop(htim, channels[1]);
        (void)HAL_TIMEx_PWMN_Stop(htim, channels[2]);
    }
}

static void ensure_pwm_channels_started(void)
{
    if (pwm_started) {
        return;
    }

    const uint32_t high_channels[3] = {bldc_h.chA, bldc_h.chB, bldc_h.chC};
    uint32_t aux_channels_arr[3];

#if BLDC_PWM_LAYOUT_COMPLEMENTARY
    start_timer_channels(bldc_h.htim_high, high_channels, 1U);
#else
    start_timer_channels(bldc_h.htim_high, high_channels, 0U);
#endif

#if BLDC_PWM_LAYOUT_DUAL
    start_timer_channels(bldc_h.htim_low, high_channels, 0U);
#endif

    if (bldc_h.htim_aux != NULL) {
        aux_channels(aux_channels_arr);
        start_timer_channels(bldc_h.htim_aux, aux_channels_arr, 0U);
    }

    pwm_started = 1U;
}

static void stop_pwm_channels(void)
{
    const uint32_t high_channels[3] = {bldc_h.chA, bldc_h.chB, bldc_h.chC};
    uint32_t aux_channels_arr[3];

#if BLDC_PWM_LAYOUT_COMPLEMENTARY
    stop_timer_channels(bldc_h.htim_high, high_channels, 1U);
#else
    stop_timer_channels(bldc_h.htim_high, high_channels, 0U);
#endif

#if BLDC_PWM_LAYOUT_DUAL
    stop_timer_channels(bldc_h.htim_low, high_channels, 0U);
#endif

    if (bldc_h.htim_aux != NULL) {
        aux_channels(aux_channels_arr);
        stop_timer_channels(bldc_h.htim_aux, aux_channels_arr, 0U);
    }
}

static void clear_all_phase_outputs(void)
{
    TIM_TypeDef *tim = bldc_h.htim_high->Instance;

#if BLDC_PWM_LAYOUT_COMPLEMENTARY
    tim->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC1NE | TIM_CCER_CC2E | TIM_CCER_CC2NE |
                   TIM_CCER_CC3E | TIM_CCER_CC3NE);
#else
    tim->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E);
#endif

    __HAL_TIM_SET_COMPARE(bldc_h.htim_high, bldc_h.chA, 0U);
    __HAL_TIM_SET_COMPARE(bldc_h.htim_high, bldc_h.chB, 0U);
    __HAL_TIM_SET_COMPARE(bldc_h.htim_high, bldc_h.chC, 0U);

#if BLDC_PWM_LAYOUT_DUAL
    tim = bldc_h.htim_low->Instance;
    tim->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E);
    __HAL_TIM_SET_COMPARE(bldc_h.htim_low, bldc_h.chA, 0U);
    __HAL_TIM_SET_COMPARE(bldc_h.htim_low, bldc_h.chB, 0U);
    __HAL_TIM_SET_COMPARE(bldc_h.htim_low, bldc_h.chC, 0U);
#endif

    if (bldc_h.htim_aux != NULL) {
        uint32_t aux_channels_arr[3];
        aux_channels(aux_channels_arr);

        tim = bldc_h.htim_aux->Instance;
        tim->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E);
        __HAL_TIM_SET_COMPARE(bldc_h.htim_aux, aux_channels_arr[0], 0U);
        __HAL_TIM_SET_COMPARE(bldc_h.htim_aux, aux_channels_arr[1], 0U);
        __HAL_TIM_SET_COMPARE(bldc_h.htim_aux, aux_channels_arr[2], 0U);
    }

#if defined(BLDC_PHASE1_LOW_USE_GPIO) && BLDC_PHASE1_LOW_USE_GPIO
    HAL_GPIO_WritePin(BLDC_PHASE1_LOW_GPIO_Port, BLDC_PHASE1_LOW_GPIO_Pin, GPIO_PIN_RESET);
#endif
}

void bldc_comm_init(BLDC_Handle_t *motor)
{
    bldc_h = *motor;
    comm_duty = 0U;
    comm_step = 0U;

    ensure_pwm_channels_started();
    clear_all_phase_outputs();
}

void bldc_comm_enable(void)
{
    pwm_moe_enable();
}

void bldc_comm_disable(void)
{
    clear_all_phase_outputs();
    pwm_moe_disable();
    stop_pwm_channels();
    comm_step = 0U;
    pwm_started = 0U;
}

void bldc_comm_set_duty(uint16_t duty)
{
    comm_duty = duty;
    if (comm_step >= 1U && comm_step <= 6U) {
        apply_step_compare(&commutation_table[comm_step - 1U], comm_duty);
    }
}

void bldc_comm_commutate(uint8_t step)
{
    if (step < 1U || step > 6U) {
        bldc_comm_disable();
        return;
    }

    ensure_pwm_channels_started();
    comm_step = step;

    const CommutationStep_t *cfg = &commutation_table[step - 1U];
    apply_step_compare(cfg, comm_duty);
    apply_phase_outputs(cfg);
    apply_aux_outputs(cfg);
    bldc_comm_enable();
}

static uint16_t comm_duty_from_percent(uint8_t percent)
{
    const uint32_t period = (uint32_t)__HAL_TIM_GET_AUTORELOAD(bldc_h.htim_high) + 1U;
    uint32_t duty = (period * (uint32_t)percent) / 100U;

    if (duty < 1U) {
        duty = 1U;
    }
    if (duty > period) {
        duty = period;
    }

    return (uint16_t)duty;
}

static float comm_effective_pole_pairs(const bldc_settings_t *settings)
{
    if (settings != NULL && settings->pole_pairs >= 1.0f) {
        return settings->pole_pairs;
    }

    return BLDC_COMM_DEFAULT_POLE_PAIRS;
}

static float comm_effective_rpm(const bldc_settings_t *settings)
{
    if (settings != NULL && settings->max_rpm_open_loop >= 60.0f) {
        return settings->max_rpm_open_loop;
    }

    return BLDC_COMM_DEFAULT_RPM;
}

static uint32_t comm_step_period_ms(float rpm, float pole_pairs)
{
    const float steps_per_sec = (rpm / 60.0f) * pole_pairs * 6.0f;

    if (steps_per_sec < 1.0f) {
        return 500U;
    }

    uint32_t period_ms = (uint32_t)(1000.0f / steps_per_sec);
    if (period_ms < 1U) {
        period_ms = 1U;
    }
    if (period_ms > 500U) {
        period_ms = 500U;
    }

    return period_ms;
}

static void comm_ensure_driver_ready(void)
{
    static uint8_t driver_ready;

    if (driver_ready) {
        return;
    }

    bldc_drv8323r_init();
    driver_ready = 1U;
}

static int comm_gate_driver_ok(void)
{
    const uint32_t faults = bldc_drv8323r_read_faults();
    if (faults == 0U) {
        return 1;
    }

    bldc_comm_disable();
    bldc_drv8323r_reset_faults();
    return 0;
}

void CommThread(void *argument)
{
    uint8_t step = 1U;

    (void)argument;

    comm_ensure_driver_ready();
    bldc_comm_set_duty(comm_duty_from_percent(BLDC_COMM_DUTY_PERCENT));

    /* Hold step 1 briefly so the rotor can settle before open-loop rotation. */
    bldc_comm_commutate(step);
    osDelay(BLDC_COMM_ALIGN_MS);

    for (;;) {
        bldc_settings_t *settings = bldc_get_settings();
        const float rpm = comm_effective_rpm(settings);
        const float pole_pairs = comm_effective_pole_pairs(settings);
        const uint32_t period_ms = comm_step_period_ms(rpm, pole_pairs);

        if (!comm_gate_driver_ok()) {
            osDelay(200);
            continue;
        }

        bldc_comm_commutate(step);

        step++;
        if (step > 6U) {
            step = 1U;
        }

        osDelay(period_ms);
    }
}