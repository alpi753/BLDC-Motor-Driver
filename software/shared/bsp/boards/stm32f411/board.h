#ifndef BLDC_BOARD_STM32F411_DEMO_H
#define BLDC_BOARD_STM32F411_DEMO_H

#include "main.h"
#include "bsp_autoconf.h"

/* -------------------------------------------------------------------------- */
/* Analog front-end                                                           */
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

#define MAJOR_SW                  1
#define MINOR_SW                  0
#define MAJOR_HW                  1
#define MINOR_HW                  0

/* -------------------------------------------------------------------------- */
/* DRV8323R SPI / enable pins (CubeMX names on demo board)                    */
/* -------------------------------------------------------------------------- */
#define DRV8323R_CS_GPIO_Port     SPI1_CS_GPIO_Port
#define DRV8323R_CS_Pin           SPI1_CS_Pin
#define DRV8323R_EN_GPIO_Port     SPI1_EN_GPIO_Port
#define DRV8323R_EN_Pin           SPI1_EN_Pin
#define DRV8323R_FAULT_GPIO_Port  SPI1_FAULT_GPIO_Port
#define DRV8323R_FAULT_Pin        SPI1_FAULT_Pin

/* -------------------------------------------------------------------------- */
/* Timer phase / CCER bit masks (TIM3 general-purpose PWM on demo board)      */
/* -------------------------------------------------------------------------- */
#define PHASE_1_CH                TIM_CHANNEL_1
#define CH1E                      TIM_CCER_CC1E
#define CH1NE                     TIM_CCER_CC1NE

#define PHASE_2_CH                TIM_CHANNEL_2
#define CH2E                      TIM_CCER_CC2E
#define CH2NE                     TIM_CCER_CC2NE

#define PHASE_3_CH                TIM_CHANNEL_3
#define CH3E                      TIM_CCER_CC3E
#define CH3NE                     TIM_CCER_CC3NE

#endif /* BLDC_BOARD_STM32F411_DEMO_H */