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
/* DRV8323 inputs (hardware/untitled.kicad_pcb netlist)                       */
/*  INHx: TIM1 CH1/2/3 on PA8/PA9/PA10                                        */
/*  INLB/INLC: TIM1 CH2N/CH3N on PB0/PB1                                      */
/*  INLA: PC13 — no timer AF; driven as GPIO                                  */
/* -------------------------------------------------------------------------- */
#define PHASE_1_CH                TIM_CHANNEL_1
#define PHASE_2_CH                TIM_CHANNEL_2
#define PHASE_3_CH                TIM_CHANNEL_3

#define BLDC_PHASE1_PWM_CH        TIM_CHANNEL_1
#define BLDC_PHASE1_LOW_CCR_CH    TIM_CHANNEL_1
#define BLDC_PHASE1_CCER_E        TIM_CCER_CC1E
#define BLDC_PHASE1_CCER_NE       0U
#define BLDC_PHASE1_LOW_USE_GPIO  1
#define BLDC_PHASE1_LOW_GPIO_Port INLA_GPIO_Port
#define BLDC_PHASE1_LOW_GPIO_Pin  INLA_Pin

#define BLDC_PHASE2_PWM_CH        TIM_CHANNEL_2
#define BLDC_PHASE2_LOW_CCR_CH    TIM_CHANNEL_2
#define BLDC_PHASE2_CCER_E        TIM_CCER_CC2E
#define BLDC_PHASE2_CCER_NE       TIM_CCER_CC2NE

#define BLDC_PHASE3_PWM_CH        TIM_CHANNEL_3
#define BLDC_PHASE3_LOW_CCR_CH    TIM_CHANNEL_3
#define BLDC_PHASE3_CCER_E        TIM_CCER_CC3E
#define BLDC_PHASE3_CCER_NE       TIM_CCER_CC3NE

#endif /* BLDC_BOARD_STM32F411_DEMO_H */