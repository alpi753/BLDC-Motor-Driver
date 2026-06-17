#include "bldc.h"

// global declartion of BLDC handle.
BLDC_Handle_t bldc_h;


void bldc_comm_init(BLDC_Handle_t *motor){
	bldc_h = *motor;
	bldc_comm_enable();
}

void bldc_comm_enable(void)
{
    __HAL_TIM_MOE_ENABLE(bldc_h.htim);   // Main output enable (TIM1/TIM8)
}

void bldc_comm_disable(void)
{
    // Stop all PWM channels
    HAL_TIM_PWM_Stop(bldc_h.htim, PHASE_1_CH);
    HAL_TIM_PWM_Stop(bldc_h.htim, PHASE_2_CH);
    HAL_TIM_PWM_Stop(bldc_h.htim, PHASE_3_CH);

    // Ensure duty cycles are zeroed
    __HAL_TIM_SET_COMPARE(bldc_h.htim, PHASE_1_CH, 0);
    __HAL_TIM_SET_COMPARE(bldc_h.htim, PHASE_2_CH, 0);
    __HAL_TIM_SET_COMPARE(bldc_h.htim, PHASE_3_CH, 0);

    // Force all outputs off
    __HAL_TIM_MOE_DISABLE(bldc_h.htim);
    bldc_h.htim->Instance->CCER &= ~(CH1E | CH1NE |
                                 CH2E | CH2NE |
                                 CH3E | CH3NE);
}

// Set phases depending on hardware driver topology
#ifdef BLDC_COMPLEMENTARY_DRIVE
static void set_phase(TIM_TypeDef *TIMx,
                      uint32_t chE,
                      uint32_t chNE,
                      PhaseState state)
{
    switch (state)
    {
        case PHASE_PWM_HIGH:
            // Enable high-side PWM, disable low-side
            TIMx->CCER |= chE;
            TIMx->CCER &= ~chNE;
            break;

        case PHASE_PWM_LOW:
            // Disable high-side, enable complementary (low-side PWM)
            TIMx->CCER &= ~chE;
            TIMx->CCER |= chNE;
            break;

        case PHASE_FLOAT:
        default:
            // Disable both outputs (true Hi-Z)
            TIMx->CCER &= ~(chE | chNE);
            break;
    }
}
#else
static void set_phase(TIM_TypeDef *TIMx,
                      uint32_t chE,
                      uint32_t chNE /* unused here, kept for macro compatibility */,
                      PhaseState state)
{
    // Implementation for non-complementary outputs (e.g. IN/EN driver)
    switch (state)
    {
        case PHASE_PWM_HIGH:
            // Enable PWM output
            TIMx->CCER |= chE;
            break;

        case PHASE_PWM_LOW:
            // For simple single-channel drives, we may disable PWM to pull low 
            // (assuming external pull-down or driver logic handles the low side)
            TIMx->CCER &= ~chE;
            break;

        case PHASE_FLOAT:
        default:
            // Disable output (float)
            TIMx->CCER &= ~chE;
            break;
    }
}
#endif

void bldc_comm_set_duty(uint16_t duty)
{
    __HAL_TIM_SET_COMPARE(bldc_h.htim, bldc_h.chA, duty);
    __HAL_TIM_SET_COMPARE(bldc_h.htim, bldc_h.chB, duty);
    __HAL_TIM_SET_COMPARE(bldc_h.htim, bldc_h.chC, duty);
}

void bldc_comm_trapeziod(uint8_t step)
{
    TIM_TypeDef *t = bldc_h.htim->Instance;

    switch (step)
    {
        case 1:
            // A+ B- C float
            set_phase(t, CH1E, CH1NE, PHASE_PWM_HIGH);
            set_phase(t, CH2E, CH2NE, PHASE_PWM_LOW);
            set_phase(t, CH3E, CH3NE, PHASE_FLOAT);
            break;

        case 2:
            // A+ C- B float
            set_phase(t, CH1E, CH1NE, PHASE_PWM_HIGH);
            set_phase(t, CH2E, CH2NE, PHASE_FLOAT);
            set_phase(t, CH3E, CH3NE, PHASE_PWM_LOW);
            break;

        case 3:
            // B+ C- A float
            set_phase(t, CH1E, CH1NE, PHASE_FLOAT);
            set_phase(t, CH2E, CH2NE, PHASE_PWM_HIGH);
            set_phase(t, CH3E, CH3NE, PHASE_PWM_LOW);
            break;

        case 4:
            // B+ A- C float
            set_phase(t, CH1E, CH1NE, PHASE_PWM_LOW);
            set_phase(t, CH2E, CH2NE, PHASE_PWM_HIGH);
            set_phase(t, CH3E, CH3NE, PHASE_FLOAT);
            break;

        case 5:
            // C+ A- B float
            set_phase(t, CH1E, CH1NE, PHASE_PWM_LOW);
            set_phase(t, CH2E, CH2NE, PHASE_FLOAT);
            set_phase(t, CH3E, CH3NE, PHASE_PWM_HIGH);
            break;

        case 6:
            // C+ B- A float
            set_phase(t, CH1E, CH1NE, PHASE_FLOAT);
            set_phase(t, CH2E, CH2NE, PHASE_PWM_LOW);
            set_phase(t, CH3E, CH3NE, PHASE_PWM_HIGH);
            break;

        default:
            bldc_comm_disable();
            break;
    }
}
