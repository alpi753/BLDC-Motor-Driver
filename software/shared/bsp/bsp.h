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

/* Portable motor handle — timer bindings come from board.conf + board.c */
typedef struct {
    TIM_HandleTypeDef *htim_high;
    TIM_HandleTypeDef *htim_low;
    TIM_HandleTypeDef *htim_aux;
    uint32_t chA;
    uint32_t chB;
    uint32_t chC;
    uint32_t aux_chA;
    uint32_t aux_chB;
    uint32_t aux_chC;
    ADC_HandleTypeDef *hadc;
} BLDC_Handle_t;

void bsp_init(void);
void bsp_usb_init(void);
const char *bsp_board_name(void);
BLDC_Handle_t *bsp_get_motor_handle(void);

/* Implemented in boards/<name>/board.c */
void bsp_board_init(void);
BLDC_Handle_t *bsp_board_get_motor_handle(void);

#if !BLDC_TELEM_USE_DEMO
int bsp_telem_adc_init(void);
int bsp_telem_adc_snapshot(uint16_t *samples, unsigned count);
void bsp_telem_adc_conv_cplt(ADC_HandleTypeDef *hadc);
#endif

#if CONFIG_FOC_ENABLE
int bsp_foc_sample_sensors(float *ia, float *ib, float *ic, float *vbus);
#endif

#ifdef __cplusplus
}
#endif

#endif /* BLDC_BSP_H */