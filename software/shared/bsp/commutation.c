#include "bldc.h"

BLDC_Handle_t bldc_h;

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
static const CommutationStep_t commutation_table[6] = {
    {PHASE_PWM_HIGH, PHASE_PWM_LOW,  PHASE_FLOAT,     1, 2}, /* A+ B- Cz */
    {PHASE_PWM_HIGH, PHASE_FLOAT,    PHASE_PWM_LOW,   1, 3}, /* A+ C- Bz */
    {PHASE_FLOAT,    PHASE_PWM_HIGH, PHASE_PWM_LOW,   2, 3}, /* B+ C- Az */
    {PHASE_PWM_LOW,  PHASE_PWM_HIGH, PHASE_FLOAT,     2, 1}, /* B+ A- Cz */
    {PHASE_PWM_LOW,  PHASE_FLOAT,    PHASE_PWM_HIGH,  3, 1}, /* C+ A- Bz */
    {PHASE_FLOAT,    PHASE_PWM_LOW,  PHASE_PWM_HIGH,  3, 2}, /* C+ B- Az */
};

static void set_phase(TIM_TypeDef *tim,
                      uint32_t ch_e,
                      uint32_t ch_ne,
                      PhaseState state)
{
#if BLDC_COMPLEMENTARY_DRIVE
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
#else
    (void)ch_ne;
    switch (state) {
    case PHASE_PWM_HIGH:
        tim->CCER |= ch_e;
        break;
    case PHASE_PWM_LOW:
    case PHASE_FLOAT:
    default:
        tim->CCER &= ~ch_e;
        break;
    }
#endif
}

static void apply_phase_outputs(const CommutationStep_t *cfg)
{
    TIM_TypeDef *tim = bldc_h.htim->Instance;

    set_phase(tim, CH1E, CH1NE, cfg->phase_a);
    set_phase(tim, CH2E, CH2NE, cfg->phase_b);
    set_phase(tim, CH3E, CH3NE, cfg->phase_c);
}

static void apply_step_compare(const CommutationStep_t *cfg, uint16_t duty)
{
    const uint16_t low_ccr = 0U; /* NE-only phase: CCR=0 keeps the low FET fully on */
    const uint32_t channels[3] = {bldc_h.chA, bldc_h.chB, bldc_h.chC};

    for (uint8_t phase = 1U; phase <= 3U; phase++) {
        uint16_t ccr = 0U;
        if (phase == cfg->pwm_phase) {
            ccr = duty;
        } else if (phase == cfg->low_phase) {
            ccr = low_ccr;
        }
        __HAL_TIM_SET_COMPARE(bldc_h.htim, channels[phase - 1U], ccr);
    }
}

static void ensure_pwm_channels_started(void)
{
    if (pwm_started) {
        return;
    }

#if BLDC_COMPLEMENTARY_DRIVE
    bsp_pwm_fixup(bldc_h.htim);

    (void)HAL_TIM_PWM_Start(bldc_h.htim, bldc_h.chA);
    (void)HAL_TIMEx_PWMN_Start(bldc_h.htim, bldc_h.chA);
    (void)HAL_TIM_PWM_Start(bldc_h.htim, bldc_h.chB);
    (void)HAL_TIMEx_PWMN_Start(bldc_h.htim, bldc_h.chB);
    (void)HAL_TIM_PWM_Start(bldc_h.htim, bldc_h.chC);
    (void)HAL_TIMEx_PWMN_Start(bldc_h.htim, bldc_h.chC);
#else
    (void)HAL_TIM_PWM_Start(bldc_h.htim, bldc_h.chA);
    (void)HAL_TIM_PWM_Start(bldc_h.htim, bldc_h.chB);
    (void)HAL_TIM_PWM_Start(bldc_h.htim, bldc_h.chC);
#endif

    pwm_started = 1U;
}

static void stop_pwm_channels(void)
{
    (void)HAL_TIM_PWM_Stop(bldc_h.htim, bldc_h.chA);
    (void)HAL_TIM_PWM_Stop(bldc_h.htim, bldc_h.chB);
    (void)HAL_TIM_PWM_Stop(bldc_h.htim, bldc_h.chC);
#if BLDC_COMPLEMENTARY_DRIVE
    (void)HAL_TIMEx_PWMN_Stop(bldc_h.htim, bldc_h.chA);
    (void)HAL_TIMEx_PWMN_Stop(bldc_h.htim, bldc_h.chB);
    (void)HAL_TIMEx_PWMN_Stop(bldc_h.htim, bldc_h.chC);
#endif
}

void bldc_comm_init(BLDC_Handle_t *motor)
{
    bldc_h = *motor;
    comm_duty = 0U;
    comm_step = 0U;

    ensure_pwm_channels_started();

    bldc_h.htim->Instance->CCER &= ~(CH1E | CH1NE | CH2E | CH2NE | CH3E | CH3NE);
    __HAL_TIM_SET_COMPARE(bldc_h.htim, bldc_h.chA, 0U);
    __HAL_TIM_SET_COMPARE(bldc_h.htim, bldc_h.chB, 0U);
    __HAL_TIM_SET_COMPARE(bldc_h.htim, bldc_h.chC, 0U);
}

void bldc_comm_enable(void)
{
    __HAL_TIM_MOE_ENABLE(bldc_h.htim);
}

void bldc_comm_disable(void)
{
    bldc_h.htim->Instance->CCER &= ~(CH1E | CH1NE | CH2E | CH2NE | CH3E | CH3NE);
    __HAL_TIM_SET_COMPARE(bldc_h.htim, bldc_h.chA, 0U);
    __HAL_TIM_SET_COMPARE(bldc_h.htim, bldc_h.chB, 0U);
    __HAL_TIM_SET_COMPARE(bldc_h.htim, bldc_h.chC, 0U);
    __HAL_TIM_MOE_DISABLE(bldc_h.htim);
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
    bldc_comm_enable();
}