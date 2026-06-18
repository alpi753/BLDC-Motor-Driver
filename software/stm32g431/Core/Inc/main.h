/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define INL1_Pin GPIO_PIN_13
#define INL1_GPIO_Port GPIOC
#define Curr_Sense_A_Pin GPIO_PIN_0
#define Curr_Sense_A_GPIO_Port GPIOA
#define Curr_Sense_B_Pin GPIO_PIN_1
#define Curr_Sense_B_GPIO_Port GPIOA
#define Curr_Sense_C_Pin GPIO_PIN_2
#define Curr_Sense_C_GPIO_Port GPIOA
#define NTC_Mosfet_Pin GPIO_PIN_6
#define NTC_Mosfet_GPIO_Port GPIOA
#define NTC_Motor_Pin GPIO_PIN_7
#define NTC_Motor_GPIO_Port GPIOA
#define INL2_Pin GPIO_PIN_0
#define INL2_GPIO_Port GPIOB
#define INL3_Pin GPIO_PIN_1
#define INL3_GPIO_Port GPIOB
#define V_Bus_Sense_Pin GPIO_PIN_2
#define V_Bus_Sense_GPIO_Port GPIOB
#define SPI1_EN_Pin GPIO_PIN_13
#define SPI1_EN_GPIO_Port GPIOB
#define SPI1_CS_Pin GPIO_PIN_14
#define SPI1_CS_GPIO_Port GPIOB
#define INH1_Pin GPIO_PIN_8
#define INH1_GPIO_Port GPIOA
#define INH2_Pin GPIO_PIN_9
#define INH2_GPIO_Port GPIOA
#define INH3_Pin GPIO_PIN_10
#define INH3_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
