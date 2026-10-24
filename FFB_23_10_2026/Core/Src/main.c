/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_customhid.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim5;

/* USER CODE BEGIN PV */
extern USBD_HandleTypeDef hUsbDeviceFS;

volatile uint8_t FFB_report[64] = {0};   /* filled by USB HID OUT/EP0 callbacks */
volatile uint8_t FFB_updated    = 0U;   /* set to 1 each time a new FFB report arrives */

int steeringA = 0;
int steeringB = 0;

uint16_t Steering;
uint16_t Y;
uint16_t Z;
uint16_t Rz;
uint16_t Ry;
uint32_t SteeringMapped;
uint32_t y;
uint32_t z;
uint32_t rz;

int duty;
int L_EN;
int R_EN;
int16_t offset;


volatile uint32_t dataout_count = 0;
uint32_t datain_count = 0;

volatile uint8_t actuators_enabled = 0;
volatile uint8_t effect_active     = 0;  /* set by 0x0A Op Effect Start, cleared by Stop */
int32_t ffb_output = 0;  /* current force magnitude (-10000..+10000) */

/* --- Debug counters: watch these in the STM32CubeIDE debugger ------------- *
 *  ep0_count         — incremented in usbd_hid.c EP0_RxReady (feature/ctrl) *
 *  hid_dataout_count — incremented in usbd_hid.c DataOut (interrupt OUT)    *
 *  handler_count     — incremented here each time FFB_updated was set        *
 *  last_report_id    — last report ID dispatched by the main loop            *
 *  actuators_enabled — 1 after 0x0C Enable, 0 after Disable/Reset           *
 *  effect_active     — 1 after 0x0A Start, 0 after 0x0A Stop                *
 *  ffb_output        — current constant-force magnitude                      *
 * -------------------------------------------------------------------------- */
volatile uint32_t handler_count = 0;
volatile uint8_t  last_report_id = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
static void MX_TIM5_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int map_ranges(int value, int origin_max, int origin_min, int mapped_max, int mapped_min){
int j = value * (mapped_max - mapped_min) / (origin_max - origin_min);
return j;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USB_DEVICE_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_TIM5_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL); // Ry axis (handbrake/possibly steering)
  __HAL_TIM_SET_COUNTER(&htim5, 0);

  HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL); // Y axis (throttle)
  __HAL_TIM_SET_COUNTER(&htim3, 300);

  HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL); // Z axis (brake)
  __HAL_TIM_SET_COUNTER(&htim4, 300);

  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL); // Rz axis (clutch)
  __HAL_TIM_SET_COUNTER(&htim2, 300);

  HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL); // steering
  __HAL_TIM_SET_COUNTER(&htim1, 0);

  /* TIM1 is PWM-only (motor). Do NOT start it as encoder — encoder mode
   * overwrites PWM and the two cannot coexist on the same timer. */

  /* Start complementary PWM on CH3 (PA10) and CH3N (PB1).
   * Initial pulse = 32767 → 50 % duty → zero net force. */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
  HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 2404);

  /* Motor driver enable pins — leave LOW until actuators_enabled goes high */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5|GPIO_PIN_4, GPIO_PIN_RESET);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	steeringA = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9);
	steeringB = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8);


	Steering = __HAL_TIM_GET_COUNTER(&htim1);
	y  = __HAL_TIM_GET_COUNTER(&htim3);
	z  = __HAL_TIM_GET_COUNTER(&htim4);
	rz = __HAL_TIM_GET_COUNTER(&htim2);

	//uint32_t SteeringModulo = Steering % 1200;
//    if (steering > 4800);
//    {
//    	__HAL_TIM_SET_COUNTER(&htim1, 4800);
//    }

	SteeringMapped = map_ranges(Steering, 4800, 0, 65535, 0);
	Y = map_ranges(y, 2350, 0, 65535, 0);
	Y = Y - 32767;
	Z = map_ranges(z, 2350, 0, 65535, 0);
	Z = Z - 32767;
	Rz = map_ranges(rz, 2350, 0, 65535, 0);
	Rz = Rz - 32767;
	/* Ry (handbrake) removed — TIM1 is now the motor PWM timer */

	uint8_t buttons = 0;
	buttons |= (uint8_t)((!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5))  << 0U); // Shift down
	buttons |= (uint8_t)((!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4))  << 1U); // Shift up
	buttons |= (uint8_t)((!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8))  << 2U); // button 3
	buttons |= (uint8_t)((!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12)) << 3U); // button 4
	buttons |= (uint8_t)((!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10)) << 4U); // button 5
	buttons |= (uint8_t)((!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15)) << 5U); // button 6
	buttons |= (uint8_t)((!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14)) << 6U); // button 7
	buttons |= (uint8_t)((!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13)) << 7U); // button 8

	/* --- Send joystick input report (ID 1) -------------------------------- */
	uint8_t report1[12];
	report1[0]  = 0x01;
	report1[1]  = (uint8_t)(SteeringMapped & 0xFF);
	report1[2]  = (uint8_t)((SteeringMapped >> 8) & 0xFF);
	report1[3]  = (uint8_t)(Y  & 0xFF);
	report1[4]  = (uint8_t)((Y  >> 8) & 0xFF);
	report1[5]  = (uint8_t)(Z  & 0xFF);
	report1[6]  = (uint8_t)((Z  >> 8) & 0xFF);
	report1[7]  = (uint8_t)(Rz & 0xFF);
	report1[8]  = (uint8_t)((Rz >> 8) & 0xFF);
	report1[9]  = 0x00; /* Ry not used */
	report1[10] = 0x00;
	report1[11] = buttons;

	USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, report1, sizeof(report1));

	/* --- Process incoming FFB reports ------------------------------------- */
	if (FFB_updated)
	{
	    FFB_updated = 0U;
	    uint8_t report_id = FFB_report[0];
	    handler_count++;
	    last_report_id = report_id;

	    switch (report_id)
	    {
	    case 0x0CU: /* PID Device Control */
	    {
	        uint8_t cmd = FFB_report[1];
	        if (cmd == 0x01U)       /* Enable Actuators */
	        {
	            actuators_enabled = 1U;
	            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4|GPIO_PIN_5, GPIO_PIN_SET);
	        }
	        else if (cmd == 0x02U)  /* Disable Actuators */
	        {
	            actuators_enabled = 0U;
	            effect_active = 0U;
	            ffb_output = 0;
	            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4|GPIO_PIN_5, GPIO_PIN_RESET);
	        }
	        else if (cmd == 0x04U)  /* Device Reset */
	        {
	            actuators_enabled = 0U;
	            effect_active = 0U;
	            ffb_output = 0;
	            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4|GPIO_PIN_5, GPIO_PIN_RESET);
	        }
	        break;
	    }
	    case 0x0AU: /* Effect Operation — start or stop an effect */
	    {
	        /* Byte 1: Effect Block Index
	         * Byte 2: Operation (1=Start, 2=StartSolo, 3=Stop)
	         * Byte 3: Loop Count */
	        uint8_t op = FFB_report[2];
	        if (op == 0x01U || op == 0x02U)  /* Op Effect Start / StartSolo */
	        {
	            effect_active = 1U;
	        }
	        else if (op == 0x03U)             /* Op Effect Stop */
	        {
	            effect_active = 0U;
	            ffb_output = 0;
	        }
	        break;
	    }
	    case 0x05U: /* Set Constant Force */
	    {
	        /* Byte 1: Effect Block Index
	         * Bytes 2-3: Magnitude (int16, physical -10000..+10000) */
	        int16_t magnitude = (int16_t)((uint16_t)FFB_report[2]
	                            | ((uint16_t)FFB_report[3] << 8));
	        ffb_output = (int32_t)magnitude;
	        break;
	    }
	    default:
	        break;
	    }
	}

	/* --- Apply FFB force via locked anti-phase PWM ------------------------
	 * Duty 0     → full reverse  (-10000)
	 * Duty 32767 → neutral       (0)
	 * Duty 65535 → full forward  (+10000)
	 *
	 * Both actuators_enabled (from 0x0C) AND effect_active (from 0x0A)
	 * must be set before the motor moves.
	 * ----------------------------------------------------------------------- */
	if (actuators_enabled && effect_active)
	{
	    int32_t duty = 32767 + (ffb_output * 32767 / 10000);
	    if (duty > 65535) duty = 65535;
	    if (duty < 0)     duty = 0;
	    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, (uint32_t)duty);
	}
	else
	{
	    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 32767U); /* neutral */
	}

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 23;
  RCC_OscInitStruct.PLL.PLLN = 354;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sConfig.EncoderMode = TIM_ENCODERMODE_TI1;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 0;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&htim1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 4808;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 96;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI1;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 0;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&htim2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI1;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 0;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&htim3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 0;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 65535;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI1;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 0;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&htim4, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 0;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 65535;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 0;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&htim5, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, L_EN_Pin|R_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : L_EN_Pin R_EN_Pin */
  GPIO_InitStruct.Pin = L_EN_Pin|R_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : button_5_Pin button_4_Pin button_8_Pin button_7_Pin
                           button_6_Pin Shift_up_Pin Shift_down_Pin */
  GPIO_InitStruct.Pin = button_5_Pin|button_4_Pin|button_8_Pin|button_7_Pin
                          |button_6_Pin|Shift_up_Pin|Shift_down_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : button_3_Pin */
  GPIO_InitStruct.Pin = button_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(button_3_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
