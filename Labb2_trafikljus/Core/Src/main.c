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
#include "abuzz.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
enum event {
	ev_none, ev_button_push, ev_state_timeout, ev_error = -99
};
#define EVQ_SIZE 10
enum event evq[ EVQ_SIZE ];
int evq_count = 0;
int evq_front_ix = 0;
int evq_rear_ix = 0;
enum state{
	s_init, s_car_go, s_pushed_wait, s_cars_stopping, s_all_stop,
	s_humans_go, s_stop_after_humans, s_start_go
};
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
uint32_t tick_left_in_state = 0;
void set_traffic_lights(enum state s){
	switch(s){
	case s_car_go:
		HAL_GPIO_WritePin(Car_Red_GPIO_Port, Car_Red_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(Car_Yellow_GPIO_Port, Car_Yellow_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(Car_Green_GPIO_Port, Car_Green_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Human_Red_GPIO_Port, Car_Red_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Human_Green_GPIO_Port, Human_Green_Pin, GPIO_PIN_RESET);
		break;
	case s_pushed_wait:
		HAL_GPIO_WritePin(Car_Red_GPIO_Port, Car_Red_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(Car_Yellow_GPIO_Port, Car_Yellow_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(Car_Green_GPIO_Port, Car_Green_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Human_Red_GPIO_Port, Car_Red_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Human_Green_GPIO_Port, Human_Green_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(Button_light_GPIO_Port, Button_light_Pin, GPIO_PIN_SET);
		break;
	case s_cars_stopping:
		HAL_GPIO_WritePin(Car_Red_GPIO_Port, Car_Red_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(Car_Yellow_GPIO_Port, Car_Yellow_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Car_Green_GPIO_Port, Car_Green_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(Human_Red_GPIO_Port, Car_Red_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Human_Green_GPIO_Port, Human_Green_Pin, GPIO_PIN_RESET);
		break;
	case s_all_stop:
		HAL_GPIO_WritePin(Car_Red_GPIO_Port, Car_Red_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Car_Yellow_GPIO_Port, Car_Yellow_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(Car_Green_GPIO_Port, Car_Green_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(Human_Red_GPIO_Port, Car_Red_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Human_Green_GPIO_Port, Human_Green_Pin, GPIO_PIN_RESET);
		break;
	case s_humans_go:
		HAL_GPIO_WritePin(Car_Red_GPIO_Port, Car_Red_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Car_Yellow_GPIO_Port, Car_Yellow_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(Car_Green_GPIO_Port, Car_Green_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(Human_Red_GPIO_Port, Car_Red_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(Human_Green_GPIO_Port, Human_Green_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Button_light_GPIO_Port, Button_light_Pin, GPIO_PIN_RESET);
		break;
	case s_stop_after_humans:
		HAL_GPIO_WritePin(Car_Red_GPIO_Port, Car_Red_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Car_Yellow_GPIO_Port, Car_Yellow_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(Car_Green_GPIO_Port, Car_Green_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(Human_Red_GPIO_Port, Car_Red_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Human_Green_GPIO_Port, Human_Green_Pin, GPIO_PIN_RESET);
		break;
	case s_start_go:
		HAL_GPIO_WritePin(Car_Red_GPIO_Port, Car_Red_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Car_Yellow_GPIO_Port, Car_Yellow_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Car_Green_GPIO_Port, Car_Green_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(Human_Red_GPIO_Port, Car_Red_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Human_Green_GPIO_Port, Human_Green_Pin, GPIO_PIN_RESET);
		break;
	case s_init:
		HAL_GPIO_WritePin(Car_Red_GPIO_Port, Car_Red_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Car_Yellow_GPIO_Port, Car_Yellow_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Car_Green_GPIO_Port, Car_Green_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Human_Red_GPIO_Port, Car_Red_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Human_Green_GPIO_Port, Human_Green_Pin, GPIO_PIN_SET);
		break;
	}
}

void evq_push_back(enum event e){
	if(evq_count < EVQ_SIZE){
		evq[evq_rear_ix] = e;
		evq_rear_ix ++ ;
		evq_rear_ix %= EVQ_SIZE;
		evq_count ++;
	}
}
enum event evq_pop_front(){
	enum event e = ev_none;
	if(evq_count > 0){
		e = evq[evq_front_ix];
		evq[evq_front_ix] = ev_error;
		evq_front_ix ++;
		evq_front_ix %= EVQ_SIZE;
		evq_count --;
	}
	return e;
}
void push_button_light_on(){
	HAL_GPIO_WritePin(Button_light_GPIO_Port, Button_light_Pin, GPIO_PIN_SET);
}
void push_button_light_off(){
	HAL_GPIO_WritePin(Button_light_GPIO_Port, Button_light_Pin, GPIO_PIN_RESET);
}
void HAL_GPIO_EXTI_Callback( uint16_t GPIO_Pin ){
	if(GPIO_Pin == GPIO_PIN_13){
		evq_push_back(ev_button_push);
	}
}
int systick_count = 0;
void my_systick_handler(){
	systick_count++;
	if (systick_count == 1000)
	{
		HAL_GPIO_TogglePin(LD4_GPIO_Port, LD4_Pin);
		systick_count = 0;
	}
	if (tick_left_in_state > 0)
	{
		tick_left_in_state--;
		if (tick_left_in_state == 0)
		{
			evq_push_back(ev_state_timeout);
		}
	}
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int is_blue_button_pressed(){
	return (GPIOC->IDR & 0x2000) != 0;
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
	MX_USART2_UART_Init();
	MX_TIM2_Init();
	/* USER CODE BEGIN 2 */
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	set_traffic_lights(s_init);
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	enum event ev = ev_none;
	enum state st = s_init;
/*	int curr_press = 0;
	int last_press = 0;
	uint32_t curr_tick = 0;
	uint32_t last_tick = 0;*/
	while (1){
#if 0
		ev = ev_none;
		curr_press = is_blue_button_pressed();
		if(curr_press && !last_press){
			ev = ev_button_push;
		}
		last_press = curr_press;

		curr_tick = HAL_GetTick();
		if (curr_tick != last_tick)
		{
			last_tick = curr_tick;

			if (tick_left_in_state > 0)
			{
				--tick_left_in_state;
				if (tick_left_in_state == 0)
				{
					ev = ev_state_timeout;
				}
			}
		}
		#else
			ev = evq_pop_front();
		switch (st)
		{
		case s_init:
			if (ev == ev_button_push)
			{
				ev = ev_state_timeout;
				tick_left_in_state = 10000;
				set_traffic_lights(s_all_stop);
				st = s_all_stop;
			}
			break;

		case s_car_go:
			if (ev == ev_button_push)
			{
				ev = ev_none;
				tick_left_in_state = 2000;
				set_traffic_lights(s_pushed_wait);
				st = s_pushed_wait;
				abuzz_start();
				abuzz_p_short();
			}
			break;

		case s_pushed_wait:
			if (ev == ev_state_timeout)
			{
				ev = ev_none;
				tick_left_in_state = 2000;
				set_traffic_lights(s_cars_stopping);
				st = s_cars_stopping;
			}
			break;

		case s_cars_stopping:
			if (ev == ev_state_timeout)
			{
				ev = ev_none;
				tick_left_in_state = 3000;
				set_traffic_lights(s_all_stop);
				st = s_all_stop;
			}
			break;

		case s_all_stop:
			if (ev == ev_state_timeout)
			{
				ev = ev_none;
				tick_left_in_state = 10000;
				set_traffic_lights(s_humans_go);
				st = s_humans_go;
				abuzz_stop();
			}
			break;

		case s_humans_go:
			if (ev == ev_state_timeout)
			{
				ev = ev_none;
				tick_left_in_state = 3000;
				set_traffic_lights(s_stop_after_humans);
				st = s_stop_after_humans;
			}
			break;

		case s_stop_after_humans:
			if (ev == ev_state_timeout)
			{
				ev = ev_none;
				tick_left_in_state = 2000;
				set_traffic_lights(s_start_go);
				st = s_start_go;
			}
			break;

		case s_start_go:
			if (ev == ev_state_timeout)
			{
				ev = ev_none;
				st =
						tick_left_in_state = 0;
				set_traffic_lights(s_car_go);
				st = s_car_go;
			}
			break;
		}
	}
#endif
	/* USER CODE END WHILE */

	/* USER CODE BEGIN 3 */
}
/* USER CODE END 3 */


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
	if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 1;
	RCC_OscInitStruct.PLL.PLLN = 10;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
	RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
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
	TIM_OC_InitTypeDef sConfigOC = {0};

	/* USER CODE BEGIN TIM2_Init 1 */

	/* USER CODE END TIM2_Init 1 */
	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 0;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 4294967295;
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
	if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
	{
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 0;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN TIM2_Init 2 */

	/* USER CODE END TIM2_Init 2 */
	HAL_TIM_MspPostInit(&htim2);

}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void)
{

	/* USER CODE BEGIN USART2_Init 0 */

	/* USER CODE END USART2_Init 0 */

	/* USER CODE BEGIN USART2_Init 1 */

	/* USER CODE END USART2_Init 1 */
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart2) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */

	/* USER CODE END USART2_Init 2 */

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
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, Car_Red_Pin|Car_Yellow_Pin|Car_Green_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, SMPS_EN_Pin|SMPS_V1_Pin|SMPS_SW_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, Human_Red_Pin|Human_Green_Pin|Button_light_Pin|LD4_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : B1_Pin */
	GPIO_InitStruct.Pin = B1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : Car_Red_Pin Car_Yellow_Pin Car_Green_Pin */
	GPIO_InitStruct.Pin = Car_Red_Pin|Car_Yellow_Pin|Car_Green_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pins : SMPS_EN_Pin SMPS_V1_Pin SMPS_SW_Pin */
	GPIO_InitStruct.Pin = SMPS_EN_Pin|SMPS_V1_Pin|SMPS_SW_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pin : SMPS_PG_Pin */
	GPIO_InitStruct.Pin = SMPS_PG_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(SMPS_PG_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : Human_Red_Pin Human_Green_Pin Button_light_Pin LD4_Pin */
	GPIO_InitStruct.Pin = Human_Red_Pin|Human_Green_Pin|Button_light_Pin|LD4_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* EXTI interrupt init*/
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
	while (1)
	{
	}
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
