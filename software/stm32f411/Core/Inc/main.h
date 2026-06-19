/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void dwt_init(void);
void get_device_id(uint8_t id[16]);
uint32_t millis32(void);
uint64_t micros64(void);
uint32_t rand32(void);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define INLA_Pin GPIO_PIN_13
#define INLA_GPIO_Port GPIOC
#define ADC_Ph0_Curr_Pin GPIO_PIN_0
#define ADC_Ph0_Curr_GPIO_Port GPIOA
#define ADC_Ph1_Curr_Pin GPIO_PIN_1
#define ADC_Ph1_Curr_GPIO_Port GPIOA
#define ADC_Ph2_Curr_Pin GPIO_PIN_2
#define ADC_Ph2_Curr_GPIO_Port GPIOA
#define SENS1_Pin GPIO_PIN_3
#define SENS1_GPIO_Port GPIOA
#define NTC_FET_Pin GPIO_PIN_6
#define NTC_FET_GPIO_Port GPIOA
#define NTC_MTR_Pin GPIO_PIN_7
#define NTC_MTR_GPIO_Port GPIOA
#define INLB_Pin GPIO_PIN_0
#define INLB_GPIO_Port GPIOB
#define INLC_Pin GPIO_PIN_1
#define INLC_GPIO_Port GPIOB
#define SPI1_CS_Pin GPIO_PIN_2
#define SPI1_CS_GPIO_Port GPIOB
#define PROT_D4_Pin GPIO_PIN_13
#define PROT_D4_GPIO_Port GPIOB
#define PROT_D5_Pin GPIO_PIN_14
#define PROT_D5_GPIO_Port GPIOB
#define INHA_Pin GPIO_PIN_8
#define INHA_GPIO_Port GPIOA
#define INHB_Pin GPIO_PIN_9
#define INHB_GPIO_Port GPIOA
#define INHC_Pin GPIO_PIN_10
#define INHC_GPIO_Port GPIOA
#define SPI1_EN_Pin GPIO_PIN_6
#define SPI1_EN_GPIO_Port GPIOB
#define SPI1_FAULT_Pin GPIO_PIN_7
#define SPI1_FAULT_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define MAJOR_SW 1
#define MINOR_SW 0

#define MAJOR_HW 1
#define MINOR_HW 0
#if defined(RNG)
#define HAS_HW_RNG 1
#else
#define HAS_HW_RNG 0
#endif


/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
