#include "bsp.h"
#include "main.h"

extern TIM_HandleTypeDef htim3;
extern ADC_HandleTypeDef hadc1;

static BLDC_Handle_t motor_handle;

void bsp_board_init(void)
{
    motor_handle = (BLDC_Handle_t){
        .htim = &htim3,
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

void bsp_board_pwm_fixup(TIM_HandleTypeDef *htim)
{
    (void)htim;
}

void bsp_usb_init(void)
{
#if CONFIG_BLDC_HAS_USB_TELEM
    extern void MX_USB_DEVICE_Init(void);
    MX_USB_DEVICE_Init();
#endif
}