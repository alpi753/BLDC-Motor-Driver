#ifndef BLDC_BSP_H
#define BLDC_BSP_H

#include <stdint.h>
#include "bsp_autoconf.h"

#if CONFIG_STM32_FAMILY_G4
#include "stm32g4xx_hal.h"
#elif CONFIG_STM32_FAMILY_F4
#include "stm32f4xx_hal.h"
#else
#error "Unsupported CONFIG_STM32_FAMILY — check board.conf"
#endif

#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Portable motor handle — timer/ADC bindings come from the active board.c */
typedef struct {
    TIM_HandleTypeDef *htim;
    uint32_t chA;
    uint32_t chB;
    uint32_t chC;
    ADC_HandleTypeDef *hadc;
} BLDC_Handle_t;

void bsp_init(void);
void bsp_usb_init(void);
const char *bsp_board_name(void);
BLDC_Handle_t *bsp_get_motor_handle(void);
void bsp_pwm_fixup(TIM_HandleTypeDef *htim);

/* Implemented in boards/<name>/board.c */
void bsp_board_init(void);
BLDC_Handle_t *bsp_board_get_motor_handle(void);
void bsp_board_pwm_fixup(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif

#endif /* BLDC_BSP_H */