/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"

#include "string.h"

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
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
sr04_t sr04_note = {
    .trig_port = TRIG_GPIO_Port,
    .trig_pin = TRIG_Pin,
    .echo_htim = & htim4,
    .capture_flag = 0,
    .time = 0,
    .time_begin = 0,
    .time_end = 0,
    .last_time = 20,
    .counter = 0,
};

sr04_t sr04_oct = {
    .trig_port = TRIG_OCT_GPIO_Port,
    .trig_pin = TRIG_OCT_Pin,
    .echo_htim = & htim2,
    .capture_flag = 0,
    .time = 0,
    .time_begin = 0,
    .time_end = 0,
    .last_time = 20,
    .counter = 0,
};

char trans_str[64] = {
    0,
};
uint32_t f, signal_time, n_octaves, n_notes, first_octave, distance_begin_note, distance_begin_oct, distance_end_note, distance_end_oct, width_note, width_oct, in_seg_note, last_in_seg_note, in_seg_oct, last_in_seg_oct, buf, width_seg_note, width_seg_oct;
uint32_t OctaveZero[7] = {
    16,
    18,
    21,
    22,
    25,
    28,
    31
};
uint32_t first_note = 16;
uint8_t bit;
uint8_t get_bits[5] = {
    0,
};
uint32_t dist_note_table[7][2];
    uint32_t dist_oct_table[6][2];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
/* USER CODE BEGIN PFP */
void Timer_OverflowHandler(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void Tone(uint32_t Frequency) {
    TIM3 -> ARR = (1000000UL / Frequency) - 1; // Set The PWM Frequency
    TIM3 -> CCR1 = (TIM3 -> ARR >> 1); // Set Duty Cycle 50%
}

void sr04_trigger(sr04_t * sr04) {
    if (!(sr04 -> capture_flag)) {
        HAL_GPIO_WritePin(sr04 -> trig_port, sr04 -> trig_pin, GPIO_PIN_SET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(sr04 -> trig_port, sr04 -> trig_pin, GPIO_PIN_RESET);
    } else {
        (sr04 -> counter) ++;
        if ((sr04 -> counter) == 5) {
            sr04 -> counter = 0;
            sr04 -> capture_flag = 0;
            sr04 -> echo_htim -> State = HAL_TIM_STATE_READY;
        }
    }
}

void sr04_measure(sr04_t * sr04) {
    if (sr04 -> echo_htim -> State == HAL_TIM_STATE_READY) //start timer if stopped
    {
        sr04 -> capture_flag = 1;
        __HAL_TIM_SET_COUNTER(sr04 -> echo_htim, 0x0000); // обнуление счётчика
        HAL_TIM_Base_Start_IT(sr04 -> echo_htim);

    } else if (sr04 -> echo_htim -> State == HAL_TIM_STATE_BUSY) //stop timer if started
    {
        HAL_TIM_Base_Stop_IT(sr04 -> echo_htim);
        signal_time = __HAL_TIM_GET_COUNTER(sr04 -> echo_htim);
        sr04 -> time = signal_time / 58;

        sr04 -> last_time = sr04 -> time;
        sr04 -> capture_flag = 0;
    }
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == (ECHO_Pin)) {
        sr04_measure( & sr04_note);
    } else if (GPIO_Pin == (ECHO_OCT_Pin)) {
        sr04_measure( & sr04_oct);
    }

}

void init_note_table(uint32_t (*table)[7]) {
    uint32_t i, j;
    for (i = 0; i < 6; i++) {
        for (j = 0; j < 7; j++) {
            table[i][j] = OctaveZero[j] << (i + first_octave);
        }
    }
    width_note = ((sr04_note.time_end - sr04_note.time_begin + buf) / n_notes) + buf;
    width_seg_note = width_note - buf;
    width_oct = ((sr04_oct.time_end - sr04_oct.time_begin + buf) / n_octaves) + buf;
    width_seg_oct = width_oct - buf;
}

void init_dist_table(uint32_t (*table)[2], uint32_t begin, uint32_t end, uint32_t n) {
    uint32_t i;
    uint32_t width = (end - begin + buf) / n;
    for (i = 0; i < n; i++) {
        table[i][0] = begin - buf + i * width;
        table[i][1] = begin + (i + 1) * width;
    }
    if (begin<buf){
    	table[0][0]=0;
    }
}

void error() {
    HAL_UART_Transmit( & huart1, (uint8_t * )
        "Wrong input\n", 11, 1000);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef * huart) {
    if (huart == & huart1) {
    	if (bit=='\n'){
    		bit=' ';
    		HAL_UART_Receive_IT( & huart1, (uint8_t * ) get_bits, 5);
    	}
    	else{
        if ((get_bits[0] == 'h') && (get_bits[1] == 'e') && (get_bits[2] == 'l') && (get_bits[3] == 'p') && (get_bits[4] == '\n')) {
            HAL_UART_Transmit( & huart1, (uint8_t * )
                "bn <digit> - set the beginning of note interval equal to <digit> cm. <digit> = 5..25\n"
                "ln <digit> - set the length of note interval equal to <digit> cm. <digit> = 74..99\n"
                "bo <digit> - set the ending of octave interval equal to <digit> cm. <digit> = 5..37\n"
                "lo <digit> - set the length of octave interval equal to <digit> cm. <digit> = 62..99\n"
                "no <digit> - set the number of octaves equal to <digit>. <digit> = 1..6\n"
                "fo <digit> - set the first octave equal to <digit> cm. <digit> = 3..8\n"
                "help - view this menu", 500, 1000);
        } else if ((get_bits[0] == 'b') && (get_bits[1] == 'n')) {
            if ((get_bits[2] == ' ') && (48 <= get_bits[3]) && (get_bits[3] <= 57)) {
                int digit1 = get_bits[3] - '0';
                if ((get_bits[4] == '\n') && (digit1 > 4)) {
                    sr04_note.time_begin = digit1;
                    init_dist_table(dist_note_table, sr04_note.time_begin, sr04_note.time_end, n_notes);
                } else if ((48 <= get_bits[4]) && (get_bits[4] <= 57)) {
                    int digit2 = get_bits[4] - '0';
                    int number = digit1 * 10 + digit2;
                    if (number < 26) {
                        sr04_note.time_begin = digit1 * 10 + digit2;
                        init_dist_table(dist_note_table, sr04_note.time_begin, sr04_note.time_end, n_notes);

                    } else {
                        error();
                    }
                } else {
                    error();
                }
            } else {
                error();
            }
        } else if ((get_bits[0] == 'l') && (get_bits[1] == 'n')) {
            if ((get_bits[2] == ' ') && (48 <= get_bits[3]) && (get_bits[3] <= 57)) {
                int digit1 = get_bits[3] - '0';
                if ((48 <= get_bits[4]) && (get_bits[4] <= 57)) {
                    int digit2 = get_bits[4] - '0';
                    int number = digit1 * 10 + digit2;
                    if ((74 <= number) && (number <= 99)) {
                        sr04_note.time_end = sr04_note.time_begin + digit1 * 10 + digit2;
                        init_dist_table(dist_note_table, sr04_note.time_begin, sr04_note.time_end, n_notes);
                    } else {
                        error();
                    }
                } else {
                    error();
                }
            } else {
                error();
            }
        } else if ((get_bits[0] == 'b') && (get_bits[1] == 'o')) {
            if ((get_bits[2] == ' ') && (48 <= get_bits[3]) && (get_bits[3] <= 57)) {
                int digit1 = get_bits[3] - '0';
                if ((get_bits[4] == '\n') && (digit1 > 4)) {
                    sr04_oct.time_begin = digit1;
                    init_dist_table(dist_oct_table, sr04_oct.time_begin, sr04_oct.time_end, n_octaves);
                } else if ((48 <= get_bits[4]) && (get_bits[4] <= 57)) {
                    int digit2 = get_bits[4] - '0';
                    int number = digit1 * 10 + digit2;
                    if (number < 38) {
                        sr04_oct.time_begin = digit1 * 10 + digit2;
                        init_dist_table(dist_oct_table, sr04_oct.time_begin, sr04_oct.time_end, n_octaves);
                    } else {
                        error();
                    }
                } else {
                    error();
                }
            } else {
                error();
            }
        } else if ((get_bits[0] == 'l') && (get_bits[1] == 'o')) {
            if ((get_bits[2] == ' ') && (48 <= get_bits[3]) && (get_bits[3] <= 57)) {
                int digit1 = get_bits[3] - '0';
                if ((48 <= get_bits[4]) && (get_bits[4] <= 57)) {
                    int digit2 = get_bits[4] - '0';
                    int number = digit1 * 10 + digit2;
                    if ((62 <= number) && (number <= 99)) {
                        sr04_oct.time_end = sr04_oct.time_begin + digit1 * 10 + digit2;
                        init_dist_table(dist_oct_table, sr04_oct.time_begin, sr04_oct.time_end, n_octaves);
                    } else {
                        error();
                    }
                } else {
                    error();
                }
            }
        } else if ((get_bits[0] == 'n') && (get_bits[1] == 'o')) {
            if ((get_bits[2] == ' ') && (48 <= get_bits[3]) && (get_bits[3] <= 57)) {
                int digit1 = get_bits[3] - '0';
                if ((get_bits[4] == '\n') && (digit1 >= 1) && (digit1 <= 6)) {
                    n_octaves = digit1;
                    init_dist_table(dist_oct_table, sr04_oct.time_begin, sr04_oct.time_end, n_octaves);
                }

            } else {
                error();
            }
    } else if ((get_bits[0] == 'f') && (get_bits[1] == 'o')) {
        if ((get_bits[2] == ' ') && (48 <= get_bits[3]) && (get_bits[3] <= 57)) {
            int digit1 = get_bits[3] - '0';
            if ((get_bits[4] == '\n') && ((n_octaves + digit1 - 1) <= 8) && (digit1 > 2)) {
                first_octave = digit1;
            }

        } else {
            error();
        }
    } else {
        error();
    }
    }
	if (get_bits[4] != '\n'){
	    HAL_UART_Receive_IT( & huart1, &bit, 1);
	}
	else{
		HAL_UART_Receive_IT( & huart1, (uint8_t * ) get_bits, 5);
	}
    }
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
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */

    HAL_TIM_Base_Start_IT( & htim2);

    HAL_TIM_IC_Start_IT( & htim1, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT( & htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start_IT( & htim3, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT( & htim4, TIM_CHANNEL_1);

    HAL_UART_Transmit( & huart1, (uint8_t * )
        "Print help to view command list\n", 32, 1000);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    sr04_note.capture_flag = 1;
    sr04_oct.capture_flag = 1;
    n_octaves = 4;
   n_notes = 7;

    uint32_t table[6][7];
    first_octave = 3;
    distance_begin_note = 5;
    distance_end_note = 80;
    distance_begin_oct = 10;
    distance_end_oct = 60;
    buf = 10;
    sr04_note.time_begin = distance_begin_note;
    sr04_note.time_end = distance_end_note;
    sr04_oct.time_begin = distance_begin_oct;
    sr04_oct.time_end = distance_end_oct;

    init_note_table(table);
    init_dist_table(dist_note_table, sr04_note.time_begin, sr04_note.time_end, n_notes);
    init_dist_table(dist_oct_table, sr04_oct.time_begin, sr04_oct.time_end, n_octaves);

    sr04_note.capture_flag = 0;
    sr04_oct.capture_flag = 0;
    uint32_t prev_i, prev_j;
    prev_i = 0;
    prev_j = 0;
    HAL_UART_Receive_IT( & huart1, (uint8_t * ) get_bits, 5);
    while (1) {

        uint32_t i, j;

        HAL_Delay(50);
        sr04_trigger( & sr04_note);

        HAL_Delay(100);
        sr04_trigger( & sr04_oct);

        if (!((dist_note_table[prev_j][0] < sr04_note.time) && (sr04_note.time < dist_note_table[prev_j][1]))) {
        	j = ((sr04_note.time - sr04_note.time_begin) * n_notes) / (sr04_note.time_end - sr04_note.time_begin);
        }
        if (!((dist_oct_table[prev_i][0] < sr04_oct.time) && (sr04_oct.time < dist_oct_table[prev_i][1]))) {
        	i = ((sr04_oct.time - sr04_oct.time_begin) * n_octaves) / (sr04_oct.time_end - sr04_oct.time_begin);
        }

        if ((i < n_octaves) && (j < n_notes)) {
            f = table[i+first_octave-3][j];
            Tone(f);
            prev_i = i;
            prev_j = j;
        } else if (j > n_notes) {
            HAL_Delay(50);
            TIM3 -> CCR1 = 0;
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
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

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 31;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
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

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 31;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 60000;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 30000;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

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

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 0;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 65535;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
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
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, TRIG_OCT_Pin|TRIG_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ECHO_Pin ECHO_OCT_Pin */
  GPIO_InitStruct.Pin = ECHO_Pin|ECHO_OCT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : TRIG_OCT_Pin TRIG_Pin */
  GPIO_InitStruct.Pin = TRIG_OCT_Pin|TRIG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

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
    while (1) {}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
