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

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Steering_encoder_chanel_A_Pin GPIO_PIN_0
#define Steering_encoder_chanel_A_GPIO_Port GPIOA
#define Steering_encoder_chanel_B_Pin GPIO_PIN_1
#define Steering_encoder_chanel_B_GPIO_Port GPIOA
#define L_EN_Pin GPIO_PIN_4
#define L_EN_GPIO_Port GPIOA
#define R_EN_Pin GPIO_PIN_5
#define R_EN_GPIO_Port GPIOA
#define Y_encoder_chanel_A_Pin GPIO_PIN_6
#define Y_encoder_chanel_A_GPIO_Port GPIOA
#define Y_encoder_chanel_B_Pin GPIO_PIN_7
#define Y_encoder_chanel_B_GPIO_Port GPIOA
#define motor_inverse_PWM_Pin GPIO_PIN_1
#define motor_inverse_PWM_GPIO_Port GPIOB
#define button_5_Pin GPIO_PIN_10
#define button_5_GPIO_Port GPIOB
#define button_4_Pin GPIO_PIN_12
#define button_4_GPIO_Port GPIOB
#define button_8_Pin GPIO_PIN_13
#define button_8_GPIO_Port GPIOB
#define button_7_Pin GPIO_PIN_14
#define button_7_GPIO_Port GPIOB
#define button_6_Pin GPIO_PIN_15
#define button_6_GPIO_Port GPIOB
#define handbrake_encoder_chanel_B_Pin GPIO_PIN_8
#define handbrake_encoder_chanel_B_GPIO_Port GPIOA
#define handbrake_encoder_chanel_A_Pin GPIO_PIN_9
#define handbrake_encoder_chanel_A_GPIO_Port GPIOA
#define motor_PWM_Pin GPIO_PIN_10
#define motor_PWM_GPIO_Port GPIOA
#define Rz_encoder_chanel_B_Pin GPIO_PIN_15
#define Rz_encoder_chanel_B_GPIO_Port GPIOA
#define Rz_encoder_chanel_A_Pin GPIO_PIN_3
#define Rz_encoder_chanel_A_GPIO_Port GPIOB
#define Shift_up_Pin GPIO_PIN_4
#define Shift_up_GPIO_Port GPIOB
#define Shift_down_Pin GPIO_PIN_5
#define Shift_down_GPIO_Port GPIOB
#define Z_encoder_chanel_B_Pin GPIO_PIN_6
#define Z_encoder_chanel_B_GPIO_Port GPIOB
#define Z_encoder_chanel_A_Pin GPIO_PIN_7
#define Z_encoder_chanel_A_GPIO_Port GPIOB
#define button_3_Pin GPIO_PIN_8
#define button_3_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
extern volatile uint32_t datain_count;

extern uint8_t g_get_report_id;

#define USBD_CUSTOMHID_REPORT_BUFFER_EVENT_ENABLED 1
#define USBD_CUSTOMHID_CTRL_REQ_GET_REPORT_ENABLED 1
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
