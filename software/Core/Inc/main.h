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
#include "stm32g4xx_hal.h"

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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define CURR_A_Pin GPIO_PIN_0
#define CURR_A_GPIO_Port GPIOA
#define CURR_B_Pin GPIO_PIN_1
#define CURR_B_GPIO_Port GPIOA
#define CURR_C_Pin GPIO_PIN_2
#define CURR_C_GPIO_Port GPIOA
#define VOLT_B_Pin GPIO_PIN_3
#define VOLT_B_GPIO_Port GPIOA
#define VOLT_A_Pin GPIO_PIN_5
#define VOLT_A_GPIO_Port GPIOA
#define NTC_FET_Pin GPIO_PIN_6
#define NTC_FET_GPIO_Port GPIOA
#define VBUS_Pin GPIO_PIN_2
#define VBUS_GPIO_Port GPIOB
#define SPI1_NSCS_Pin GPIO_PIN_10
#define SPI1_NSCS_GPIO_Port GPIOB
#define VOLT_C_Pin GPIO_PIN_11
#define VOLT_C_GPIO_Port GPIOB
#define LEDC_Pin GPIO_PIN_12
#define LEDC_GPIO_Port GPIOB
#define LEDB_Pin GPIO_PIN_13
#define LEDB_GPIO_Port GPIOB
#define LEDA_Pin GPIO_PIN_14
#define LEDA_GPIO_Port GPIOB
#define SPI1_ENABLE_Pin GPIO_PIN_15
#define SPI1_ENABLE_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
