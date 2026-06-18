#include "bsp.h"
#include "main.h"

extern TIM_HandleTypeDef htim1;
extern ADC_HandleTypeDef hadc1;

static BLDC_Handle_t motor_handle;

#if CONFIG_BLDC_PWM_TIMER_HIGH != 1
#error "stm32g431 board.c: CONFIG_BLDC_PWM_TIMER_HIGH must be 1 (htim1)"
#endif

void bsp_board_init(void)
{
    motor_handle = (BLDC_Handle_t){
        .htim_high = &htim1,
        .htim_low  = NULL,
        .htim_aux  = NULL,
        .chA       = BLDC_PHASE1_PWM_CH,
        .chB       = BLDC_PHASE2_PWM_CH,
        .chC       = BLDC_PHASE3_PWM_CH,
        .aux_chA   = 0U,
        .aux_chB   = 0U,
        .aux_chC   = 0U,
        .hadc      = &hadc1,
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