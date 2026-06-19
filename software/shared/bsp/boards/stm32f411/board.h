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

/* ADC1 regular DMA scan order (single ADC — mirrors g431 signal groups).
 * Phase currents: PA0–PA2 (g431: ADC1 injected IN1–IN3).
 * NTC sensors:    PA6–PA7 (g431: ADC2 IN3/IN4).
 * VBus on PB2 has no ADC input on STM32F411; bus voltage is not sampled. */
#define ADC_HAS_VBUS_ADC          0
#define ADC_IDX_PHASE_A           0U
#define ADC_IDX_PHASE_B           1U
#define ADC_IDX_PHASE_C           2U
#define ADC_IDX_TEMP              3U
#define ADC_IDX_TEMP_MTR          4U

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
/*  Phase currents: PA0–PA2 (same nets as g431 Curr_Sense_A/B/C)             */
/*  NTC sensors:    PA6–PA7 (same nets as g431 NTC_Mosfet / NTC_Motor)       */
/*  VBus divider:   PB2 on PCB — no ADC pin on F411 (see ADC_HAS_VBUS_ADC)   */
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