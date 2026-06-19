#ifndef BLDC_BOARD_STM32G431_ESC_H
#define BLDC_BOARD_STM32G431_ESC_H

#include "main.h"
#include "bsp_autoconf.h"

/* -------------------------------------------------------------------------- */
/* Analog front-end (shared constants — override per board if needed)         */
/* -------------------------------------------------------------------------- */
#define ADC_MAX_COUNT             4095.0f
#define ADC_REF_VOLT              3.3f
#define PHASE_CURRENT_ZERO_V      1.65f
#define PHASE_CURRENT_V_PER_A     0.100f
#define BUS_VOLTAGE_DIVIDER_RATIO 11.0f
#define THERMISTOR_PULLUP         10000.0f
#define THERMISTOR_R25            10000.0f
#define THERMISTOR_BETA           3950.0f
#define BATTERY_CAPACITY_WH       100.0f

/* Unified snapshot layout produced by bsp_telem_adc_snapshot() in board.c:
 * Phase currents from ADC1 injected (PA0–PA2 / IN1–IN3).
 * Slow sensors from ADC2 regular scan rank order: CH3, CH4, CH12. */
#define ADC_HAS_VBUS_ADC          1
#define ADC_IDX_PHASE_A           0U
#define ADC_IDX_PHASE_B           1U
#define ADC_IDX_PHASE_C           2U
#define ADC_IDX_VBUS              3U
#define ADC_IDX_TEMP              4U
#define ADC_SLOW_RANK_NTC_FET     0U
#define ADC_SLOW_RANK_NTC_MTR     1U
#define ADC_SLOW_RANK_VBUS        2U

#define MAJOR_SW                  1
#define MINOR_SW                  0
#define MAJOR_HW                  1
#define MINOR_HW                  0

/* -------------------------------------------------------------------------- */
/* Timer phase / CCER bit masks (TIM1 advanced timer)                         */
/* -------------------------------------------------------------------------- */
#define PHASE_1_CH                TIM_CHANNEL_1
#define PHASE_2_CH                TIM_CHANNEL_2
#define PHASE_3_CH                TIM_CHANNEL_3

#define BLDC_PHASE1_PWM_CH        TIM_CHANNEL_1
#define BLDC_PHASE1_LOW_CCR_CH    TIM_CHANNEL_1
#define BLDC_PHASE1_CCER_E        TIM_CCER_CC1E
#define BLDC_PHASE1_CCER_NE       TIM_CCER_CC1NE

#define BLDC_PHASE2_PWM_CH        TIM_CHANNEL_2
#define BLDC_PHASE2_LOW_CCR_CH    TIM_CHANNEL_2
#define BLDC_PHASE2_CCER_E        TIM_CCER_CC2E
#define BLDC_PHASE2_CCER_NE       TIM_CCER_CC2NE

#define BLDC_PHASE3_PWM_CH        TIM_CHANNEL_3
#define BLDC_PHASE3_LOW_CCR_CH    TIM_CHANNEL_3
#define BLDC_PHASE3_CCER_E        TIM_CCER_CC3E
#define BLDC_PHASE3_CCER_NE       TIM_CCER_CC3NE


/* -------------------------------------------------------------------------- */
/* DRV8323R SPI / enable pins (CubeMX names on demo board)                    */
/* -------------------------------------------------------------------------- */
#define DRV8323R_CS_GPIO_Port     SPI1_CS_GPIO_Port
#define DRV8323R_CS_Pin           SPI1_CS_Pin
#define DRV8323R_EN_GPIO_Port     SPI1_EN_GPIO_Port
#define DRV8323R_EN_Pin           SPI1_EN_Pin
#define DRV8323R_FAULT_GPIO_Port  SPI1_FAULT_GPIO_Port
#define DRV8323R_FAULT_Pin        SPI1_FAULT_Pin

#endif /* BLDC_BOARD_STM32G431_ESC_H */