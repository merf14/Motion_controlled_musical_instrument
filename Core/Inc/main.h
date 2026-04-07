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
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
typedef struct {
    GPIO_TypeDef *trig_port;  // Trigger pin port
    uint16_t trig_pin;  // Trigger pin number
    TIM_HandleTypeDef *echo_htim;  // Echo pin timer
    uint8_t capture_flag;  // Echo pin capture flag
    uint32_t time;  // Distance in mm
    uint32_t time_begin;
    uint32_t time_end;
    uint32_t last_time;
    uint16_t counter;
} sr04_t;
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void Timer_OverflowHandler(void);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_Pin GPIO_PIN_0
#define LED_GPIO_Port GPIOA
#define ECHO_Pin GPIO_PIN_8
#define ECHO_GPIO_Port GPIOA
#define ECHO_EXTI_IRQn EXTI9_5_IRQn
#define ECHO_OCT_Pin GPIO_PIN_11
#define ECHO_OCT_GPIO_Port GPIOA
#define ECHO_OCT_EXTI_IRQn EXTI15_10_IRQn
#define TRIG_OCT_Pin GPIO_PIN_5
#define TRIG_OCT_GPIO_Port GPIOB
#define TRIG_Pin GPIO_PIN_8
#define TRIG_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
