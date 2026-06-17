#include "bsp.h"
#include "main.h"

extern TIM_HandleTypeDef htim1;
extern ADC_HandleTypeDef hadc1;

static BLDC_Handle_t motor_handle;

void bsp_board_init(void)
{
    motor_handle = (BLDC_Handle_t){
        .htim = &htim1,
        .chA  = PHASE_1_CH,
        .chB  = PHASE_2_CH,
        .chC  = PHASE_3_CH,
        .hadc = &hadc1,
    };
}

BLDC_Handle_t *bsp_board_get_motor_handle(void)
{
    return &motor_handle;
}

void bsp_usb_init(void)
{
#if CONFIG_BLDC_HAS_USB_TELEM
    extern void MX_USB_Device_Init(void);
    MX_USB_Device_Init();
#endif
}

void bsp_board_pwm_fixup(TIM_HandleTypeDef *htim)
{
#if CONFIG_BLDC_PWM_FIXUP_CH3
    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode       = TIM_OCMODE_PWM1;
    oc.Pulse        = 0;
    oc.OCPolarity   = TIM_OCPOLARITY_HIGH;
    oc.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    oc.OCFastMode   = TIM_OCFAST_DISABLE;
    oc.OCIdleState  = TIM_OCIDLESTATE_RESET;
    oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    (void)HAL_TIM_PWM_ConfigChannel(htim, &oc, PHASE_3_CH);
#else
    (void)htim;
#endif
}