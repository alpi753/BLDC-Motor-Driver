#include "bsp.h"
#include "bldc.h"

void bsp_init(void)
{
    dwt_init();
    bsp_board_init();
}

const char *bsp_board_name(void)
{
    return CONFIG_BLDC_BOARD_NAME;
}

BLDC_Handle_t *bsp_get_motor_handle(void)
{
    return bsp_board_get_motor_handle();
}

void bsp_pwm_fixup(TIM_HandleTypeDef *htim)
{
    bsp_board_pwm_fixup(htim);
}