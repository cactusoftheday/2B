/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
uint8_t byte;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim5;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM5_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t rcv_intpt_flag = 0;
uint16_t distance = 56;
uint8_t us100_Rx_flag;
uint8_t cmd_dist = 0x55;
int rpm_tick_count = 0;
double rpm = 0;
int clock_secs = 0;
int clock_mins = 0;
int clock_hours = 0;
int seconds = 0;
int sec = 0;
int clock_seccs = 0; // for RPM
uint8_t us100_buffer[2] = {0};
int TIM3_DCM_PWM = 0;
int digits[2] = {0};
double percent = 0;
int temp = 0;
int pastHour = -1;
int isFull = 0;

void DIGIT_A_Display(uint8_t DIGIT_A)
{
	 uint8_t DIGITA_VAL = 0x0F & DIGIT_A; //mask off higher4 buts

	 switch(DIGITA_VAL)
	 {
	 case 0:
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A0_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A1_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A2_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOA, DIGIT_A3_Pin, GPIO_PIN_RESET);
	 break;
	 case 1:
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A0_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A1_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A2_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOA, DIGIT_A3_Pin, GPIO_PIN_RESET);
	 break;
	 case 2:
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A0_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A1_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A2_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOA, DIGIT_A3_Pin, GPIO_PIN_RESET);
	 break;
	 case 3:
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A0_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A1_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A2_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOA, DIGIT_A3_Pin, GPIO_PIN_RESET);
	 break;
	 case 4:
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A0_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A1_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A2_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOA, DIGIT_A3_Pin, GPIO_PIN_RESET);
	 break;
	 case 5:
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A0_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A1_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A2_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOA, DIGIT_A3_Pin, GPIO_PIN_RESET);
	 break;
	 case 6:
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A0_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A1_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A2_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOA, DIGIT_A3_Pin, GPIO_PIN_RESET);
	 break;
	 case 7:
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A0_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A1_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A2_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOA, DIGIT_A3_Pin, GPIO_PIN_RESET);
	 break;
	 case 8:
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A0_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A1_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A2_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOA, DIGIT_A3_Pin, GPIO_PIN_SET);
	 break;
	 case 9:
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A0_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A1_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOC, DIGIT_A2_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOA, DIGIT_A3_Pin, GPIO_PIN_SET);
	 break;
	 }
}

void DIGIT_B_Display(uint8_t DIGIT_B)
{
	 uint8_t DIGITB_VAL = 0x0F & DIGIT_B; //mask off higher4 buts

	 switch(DIGITB_VAL)
	 {
	 case 0:
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B0_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B1_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B2_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B3_Pin, GPIO_PIN_RESET);
	 break;
	 case 1:
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B0_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B1_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B2_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B3_Pin, GPIO_PIN_RESET);
	 break;
	 case 2:
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B0_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B1_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B2_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B3_Pin, GPIO_PIN_RESET);
	 break;
	 case 3:
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B0_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B1_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B2_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B3_Pin, GPIO_PIN_RESET);
	 break;
	 case 4:
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B0_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B1_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B2_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B3_Pin, GPIO_PIN_RESET);
	 break;
	 case 5:
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B0_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B1_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B2_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B3_Pin, GPIO_PIN_RESET);
	 break;
	 case 6:
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B0_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B1_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B2_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B3_Pin, GPIO_PIN_RESET);
	 break;
	 case 7:
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B0_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B1_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B2_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B3_Pin, GPIO_PIN_RESET);
	 break;
	 case 8:
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B0_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B1_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B2_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B3_Pin, GPIO_PIN_SET);
	 break;
	 case 9:
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B0_Pin, GPIO_PIN_SET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B1_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B2_Pin, GPIO_PIN_RESET);
		 HAL_GPIO_WritePin(GPIOB, DIGIT_B3_Pin, GPIO_PIN_SET);
	 break;
	 }
}


void ADC_Select_CH(int CH)
{
	ADC_ChannelConfTypeDef sConfig = {0};
	switch(CH)
	{
		case 0:
			sConfig.Channel = ADC_CHANNEL_0;
			sConfig.Rank = 1;
			if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			{
				Error_Handler();
			}
		break;

		case 1:
			sConfig.Channel = ADC_CHANNEL_1;
			sConfig.Rank = 1;
			if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			{
				Error_Handler();
			}
		break;

		case 2:
			sConfig.Channel = ADC_CHANNEL_2;
			sConfig.Rank = 1;
			if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			{
				Error_Handler();
			}
		break;

		case 3:
			sConfig.Channel = ADC_CHANNEL_3;
			sConfig.Rank = 1;
			if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			{
				Error_Handler();
			}
		break;

		case 4:
			sConfig.Channel = ADC_CHANNEL_4;
			sConfig.Rank = 1;
			if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			{
				Error_Handler();
			}
		break;

		case 5:
			sConfig.Channel = ADC_CHANNEL_5;
			sConfig.Rank = 1;
			if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			{
				Error_Handler();
			}
		break;

		case 6:
			sConfig.Channel = ADC_CHANNEL_6;
			sConfig.Rank = 1;
			if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			{
				Error_Handler();
			}
		break;

		case 7:
			sConfig.Channel = ADC_CHANNEL_7;
			sConfig.Rank = 1;
			if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			{
				Error_Handler();
			}
		break;

		case 8:
			sConfig.Channel = ADC_CHANNEL_8;
			sConfig.Rank = 1;
			if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			{
				Error_Handler();
			}
		break;

		case 9:
			sConfig.Channel = ADC_CHANNEL_9;
			sConfig.Rank = 1;
			if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			{
				Error_Handler();
			}
		break;

		case 10:
			sConfig.Channel = ADC_CHANNEL_10;
			sConfig.Rank = 1;
			if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			{
				Error_Handler();
			}
		break;

		case 11:
			sConfig.Channel = ADC_CHANNEL_11;
			sConfig.Rank = 1;
			if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			{
				Error_Handler();
			}
		break;

		case 12:
			sConfig.Channel = ADC_CHANNEL_12;
			sConfig.Rank = 1;
			if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			{
				Error_Handler();
			}
		break;

		case 13:
			sConfig.Channel = ADC_CHANNEL_13;
			sConfig.Rank = 1;
			if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			{
				Error_Handler();
			}
		break;

		case 14:
			sConfig.Channel = ADC_CHANNEL_14;
			sConfig.Rank = 1;
			if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			{
				Error_Handler();
			}
		break;

		case 15:
			sConfig.Channel = ADC_CHANNEL_15;
			sConfig.Rank = 1;
			if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			{
				Error_Handler();
			}
		break;
	}
}

void distance_sensor()
{
	// Distance Sensor
	  HAL_UART_Receive_IT(&huart1,us100_buffer,2);
	  HAL_UART_Transmit(&huart1, &cmd_dist, 1, 500);
	  while(us100_Rx_flag = (00)) {};
	  distance = us100_buffer[0] << 8 | us100_buffer[1];
	  if(distance >= 650){
		  uint8_t buffer[128] = {0};
		TIM3->CCR3 = 0;
		TIM3->CCR1 = 0;
		sprintf((char*)buffer, "\r\n---------------RESERVOIR IS EMPTY----------------");
		HAL_UART_Transmit(&huart6, buffer, strlen((char*)buffer), 1000);



	HAL_GPIO_WritePin(GPIOA, LD2_Pin, GPIO_PIN_RESET); // Turn off LD2 LED

	  while(1){
		  HAL_GPIO_WritePin(GPIOA, GRN_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(GPIOA, BLU_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(GPIOA, RFD_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(GPIOA, RFD_Pin, GPIO_PIN_SET);

		  HAL_Delay(1000);

		  HAL_GPIO_WritePin(GPIOA, GRN_Pin, GPIO_PIN_RESET);
				  HAL_GPIO_WritePin(GPIOA, BLU_Pin, GPIO_PIN_RESET);
				  HAL_GPIO_WritePin(GPIOA, RFD_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(GPIOA, GRN_Pin, GPIO_PIN_SET);


		  HAL_Delay(1000);
		  HAL_GPIO_WritePin(GPIOA, GRN_Pin, GPIO_PIN_RESET);
				  HAL_GPIO_WritePin(GPIOA, BLU_Pin, GPIO_PIN_RESET);
				  HAL_GPIO_WritePin(GPIOA, RFD_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(GPIOA, BLU_Pin, GPIO_PIN_SET);

		  HAL_Delay(1000);
	  }

	}
}

void manual_mode_INLET()
{
	ADC_Select_CH(9);
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 1000);
	uint8_t ADC_CH9 = HAL_ADC_GetValue(&hadc1);
	TIM3_DCM_PWM = (((ADC_CH9 << 6) + (ADC_CH9 << 5) + (ADC_CH9 << 2)) >> 8 );
	TIM3->CCR1 = TIM3_DCM_PWM;
	TIM3->CCR3 = 0;
}

void manual_mode_ZONES()
{
	ADC_Select_CH(9);
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 1000);
	uint8_t ADC_CH9 = HAL_ADC_GetValue(&hadc1);
	TIM3_DCM_PWM = (((ADC_CH9 << 6) + (ADC_CH9 << 5) + (ADC_CH9 << 2)) >> 8 );
	TIM3->CCR3 = TIM3_DCM_PWM;
	TIM3->CCR1 = 0;
}

void digits_set()
{
	// calculate percentage fill
	  percent = ((650-distance)*100)/650;
	  digits[0] = ((int)(percent))%10;
	  digits[1] = ((int)percent / 10)%10;

	  HAL_Delay(100);

	  // display percentage full on timer board
	  DIGIT_A_Display(digits[1]);
	  DIGIT_B_Display(digits[0]);
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  uint8_t txd_msg_buffer[128] = {0};
  uint8_t txd_msg_buffer1[128] = {0};
  uint8_t txd_msg_buffer2[128] = {0};
  uint8_t txd_msg_buffer3[128] = {0};
  uint8_t txd_msg_buffer4[128] = {0};
  uint8_t txd_msg_buffer5[128] = {0};
  uint8_t txd_msg_buffer6[128] = {0};
  uint8_t txd_msg_buffer7[128] = {0};
  uint8_t txd_msg_buffer8[128] = {0};
  uint8_t txd_msg_buffer9[128] = {0};
  uint8_t txd_msg_buffer10[128] = {0};
  uint8_t txd_msg_buffer11[128] = {0};
  uint8_t txd_msg_buffer12[128] = {0};
  uint8_t txd_msg_buffer13[128] = {0};
  uint8_t txd_msg_buffer14[128] = {0};
  uint8_t txd_msg_buffer15[128] = {0};
  uint8_t txd_msg_buffer16[128] = {0};

  int zones[4] = {0};
  int a;
  int b;
  int servo;
  int time[9] = {0};

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
  MX_USART6_UART_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  MX_TIM3_Init();
  MX_TIM2_Init();
  MX_TIM5_Init();
  /* USER CODE BEGIN 2 */

  // Servo Motor
  int TIM2_Ch1_DCVAL = 500;
  int TIM2_CH1_STEP = 150;

  HAL_TIM_Base_Init(&htim3);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

  // Servo Motor
  HAL_TIM_Base_Start(&htim2);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
  TIM2->PSC = 16-1;
  TIM2->ARR = 20000-1;
  TIM2->CCR1= TIM2_Ch1_DCVAL;


  	// SET UP MODE
	sprintf((char*)txd_msg_buffer, "\r\n----------------SETUP MODE----------------");
	HAL_UART_Transmit(&huart6, txd_msg_buffer, strlen((char*)txd_msg_buffer), 1000);

	sprintf((char*)txd_msg_buffer16, "\r\nCURRENT WALL CLOCK TIME (00-23): ");
	HAL_UART_Transmit(&huart6, txd_msg_buffer16, strlen((char*)txd_msg_buffer16), 1000);
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}
	a = byte - 48;
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}  // Take second number
	b = byte - 48;
	time[0] = (10*a + b); // total scaled time in hours


	// INLET
	sprintf((char*)txd_msg_buffer15, "\r\nFor Inlet INPUT PWM (in %%) 0) Manual 1) 60 2) 80 3) 99 : ");
	HAL_UART_Transmit(&huart6, txd_msg_buffer15, strlen((char*)txd_msg_buffer15), 1000);
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}
	zones[1] = byte - 48;

	switch(zones[1]){
		case 1:
			zones[1] = 60;
			break;
		case 2:
			zones[1] = 80;
			break;
		case 3:
			zones[1] = 99;
			break;
	}

	sprintf((char*)txd_msg_buffer2, "\r\nFor Zone 1 INPUT PWM (in %%) 0) Manual 1) 60 2) 80 3) 99 : ");
	HAL_UART_Transmit(&huart6, txd_msg_buffer2, strlen((char*)txd_msg_buffer2), 1000);
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}
	zones[1] = byte - 48;

	switch(zones[1]){
		case 1:
			zones[1] = 60;
			break;
		case 2:
			zones[1] = 80;
			break;
		case 3:
			zones[1] = 99;
			break;
	}

	sprintf((char*)txd_msg_buffer4, "\r\nFor Zone 2 INPUT PWM (in %%) 0) Manual 1) 60 2) 80 3) 99 : ");
	HAL_UART_Transmit(&huart6, txd_msg_buffer4, strlen((char*)txd_msg_buffer4), 1000);
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}
	zones[2] = byte - 48;

	switch(zones[2]){
		case 1:
			zones[2] = 60;
			break;
		case 2:
			zones[2] = 80;
			break;
		case 3:
			zones[2] = 99;
			break;
	}

	sprintf((char*)txd_msg_buffer6, "\r\nFor Zone 3 INPUT PWM (in %%) 0) Manual 1) 60 2) 80 3) 99 : ");
	HAL_UART_Transmit(&huart6, txd_msg_buffer6, strlen((char*)txd_msg_buffer6), 1000);
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}
	zones[3] = byte - 48;

	switch(zones[3]){
		case 1:
			zones[3] = 60;
			break;
		case 2:
			zones[3] = 80;
			break;
		case 3:
			zones[3] = 99;
			break;
	}

	sprintf((char*)txd_msg_buffer1, "\r\nFor Inlet Enter START TIME (00-23): ");
	HAL_UART_Transmit(&huart6, txd_msg_buffer1, strlen((char*)txd_msg_buffer1), 1000);
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}  // Take first number
	a = byte - 48;
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}  // Take second number
	b = byte - 48;
	time[1] = (10*a + b); // total scaled time in hours

	memset(txd_msg_buffer1, 0, sizeof(txd_msg_buffer1));  // Clear the buffer
	sprintf((char*)txd_msg_buffer1, "\r\nFor Inlet Enter STOP TIME (00-23): ");
	HAL_UART_Transmit(&huart6, txd_msg_buffer1, strlen((char*)txd_msg_buffer1), 1000);
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}  // Take first number
	a = byte - 48;
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}  // Take second number
	b = byte - 48;
	time[2] = (10*a + b); // total scaled time in hours


	// ZONE 1

	sprintf((char*)txd_msg_buffer3, "\r\nFor Zone 1 Enter START TIME (00-23): ");
	HAL_UART_Transmit(&huart6, txd_msg_buffer3, strlen((char*)txd_msg_buffer3), 1000);
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}  // Take first number
	a = byte - 48;
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}  // Take second number
	b = byte - 48;
	time[3] = (10*a + b); // total scaled time in hours

	memset(txd_msg_buffer3, 0, sizeof(txd_msg_buffer1));  // Clear the buffer

	sprintf((char*)txd_msg_buffer3, "\r\nFor Zone 1 Enter STOP TIME (00-23): ");
	HAL_UART_Transmit(&huart6, txd_msg_buffer3, strlen((char*)txd_msg_buffer3), 1000);
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}  // Take first number
	a = byte - 48;
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}  // Take second number
	b = byte - 48;
	time[4] = (10*a + b); // total scaled time in hours




	// ZONE 2


	sprintf((char*)txd_msg_buffer5, "\r\nFor Zone 2 Enter START TIME (00-23): ");
	HAL_UART_Transmit(&huart6, txd_msg_buffer5, strlen((char*)txd_msg_buffer5), 1000);
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}  // Take first number
	a = byte - 48;
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}  // Take second number
	b = byte - 48;
	time[5] = (10*a + b); // total scaled time in hours

	memset(txd_msg_buffer5, 0, sizeof(txd_msg_buffer1));  // Clear the buffer

	sprintf((char*)txd_msg_buffer5, "\r\nFor Zone 2 Enter STOP TIME (00-23): ");
	HAL_UART_Transmit(&huart6, txd_msg_buffer5, strlen((char*)txd_msg_buffer5), 1000);
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}  // Take first number
	a = byte - 48;
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}  // Take second number
	b = byte - 48;
	time[6] = (10*a + b); // total scaled time in hours



	// ZONE 3


	sprintf((char*)txd_msg_buffer7, "\r\nFor Zone 3 Enter START TIME (00-23): ");
	HAL_UART_Transmit(&huart6, txd_msg_buffer7, strlen((char*)txd_msg_buffer7), 1000);
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}  // Take first number
	a = byte - 48;
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}  // Take second number
	b = byte - 48;
	time[7] = (10*a + b); // total scaled time in hours

	memset(txd_msg_buffer7, 0, sizeof(txd_msg_buffer1));  // Clear the buffer

	sprintf((char*)txd_msg_buffer7, "\r\nFor Zone 3 Enter STOP TIME (00-23): ");
	HAL_UART_Transmit(&huart6, txd_msg_buffer7, strlen((char*)txd_msg_buffer7), 1000);
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}  // Take first number
	a = byte - 48;
	rcv_intpt_flag = 00;
	HAL_UART_Receive_IT(&huart6,&byte,1);
	while(rcv_intpt_flag == (00)) {}  // Take second number
	b = byte - 48;
	time[8] = (10*a + b); // total scaled time in hours

	memset(txd_msg_buffer7, 0, sizeof(txd_msg_buffer1));  // Clear the buffer

	sprintf((char*)txd_msg_buffer7, "\r\nAwaiting User Input \n ");
	HAL_UART_Transmit(&huart6, txd_msg_buffer7, strlen((char*)txd_msg_buffer7), 1000);

	while (1) {
		HAL_GPIO_WritePin(GPIOA, LD2_Pin, GPIO_PIN_SET); // Turn on LD2 LED
		HAL_Delay(50); // Delay for 50 ms
		HAL_GPIO_WritePin(GPIOA, LD2_Pin, GPIO_PIN_RESET); // Turn off LD2 LED

		if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET) {
			// B1 pin is low (assuming active low configuration)
			break;
		}
		// Add a small delay before checking the B1 pin again
		HAL_Delay(50);
	}
	clock_hours = time[0];
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  TIM3->CCR3 = 0;

	  // Set timer board to 0
	  DIGIT_A_Display(0);
	  DIGIT_B_Display(0);

	  // Turn off all led lights
	  HAL_GPIO_WritePin(GPIOA, RFD_Pin, GPIO_PIN_RESET);
	  HAL_GPIO_WritePin(GPIOA, GRN_Pin, GPIO_PIN_RESET);
	  HAL_GPIO_WritePin(GPIOA, BLU_Pin, GPIO_PIN_RESET);

	  HAL_GPIO_WritePin(GPIOA, LD2_Pin, GPIO_PIN_SET);


	  // RUN MODE
	  sprintf((char*)txd_msg_buffer8, "\r\n----------------RUN MODE----------------");
	  HAL_UART_Transmit(&huart6, txd_msg_buffer8, strlen((char*)txd_msg_buffer8), 1000);

	  // start timer
	  HAL_TIM_Base_Init(&htim5);
	  HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1);
	  HAL_TIM_Base_Start_IT(&htim5);


	  // To deal with first distance being 0

	  if (distance == 0) {
	  distance_sensor();
		memset(txd_msg_buffer10, 0, sizeof(txd_msg_buffer10));  // Clear the buffer


	  sprintf((char*)txd_msg_buffer10, "\r\nTIME: %d INLET MODE -- PWM: %d -- RPM: %d DEPTH: %d", clock_hours,  zones[0], (int) rpm, distance);
	  	HAL_UART_Transmit(&huart6, txd_msg_buffer10, strlen((char*)txd_msg_buffer10), 1000);

	  }


	  // ZONES

	  while (distance <= 650 && distance != 0)
	  {
		  isFull = 0;
		  //sec = 0;
		  // Set color to purple
		  HAL_GPIO_WritePin(GPIOA, RFD_Pin, GPIO_PIN_SET);
		  HAL_GPIO_WritePin(GPIOA, BLU_Pin, GPIO_PIN_SET);

		  // Servo rotate
		  TIM2->CCR1 = 2000;

		  // dc motor runs for user specified time
		  while ((clock_hours >= time[1] && clock_hours <= time[2]) || isFull == 0)
		  {
			  // Timer
			  TIM3->PSC = 160-1;
			  TIM3->ARR = 100-1;
			  TIM3->CCR3 = 0;

			  distance_sensor();

			  // Terminal Display
			  if(clock_hours != pastHour) {
				  sprintf((char*)txd_msg_buffer10, "\r\nTIME: %d INLET MODE -- PWM: %d -- RPM: %d DEPTH: %d", clock_hours,  zones[0], (int) rpm, distance);
				  HAL_UART_Transmit(&huart6, txd_msg_buffer10, strlen((char*)txd_msg_buffer10), 1000);
				  pastHour = clock_hours;
			  }




			  // If distance is less than 50, then resovoir is considered full
			  if (distance >= 50)
			  {
				  digits_set();
				  // dc motor speed
				  if (zones[0] == 0)
				  {
					  manual_mode_INLET();
				  }

				  else
				  {
					  TIM3->CCR1 = zones[0];
				  }

			  }

			  else {

				  if (distance != 0)
				  {
					  isFull = 1;
					  TIM3->CCR1 = 0;
					  DIGIT_A_Display(9);
					  DIGIT_B_Display(9);
					  rpm_tick_count = 0;
				  }

			  }

		  }

		  TIM3->CCR1 = 0;
		  rcv_intpt_flag = 00;

		  // ZONE 1
		  temp = 0;
		  //sec = 0;
		  // Set color to red
		  HAL_GPIO_WritePin(GPIOA, GRN_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(GPIOA, BLU_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(GPIOA, RFD_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(GPIOA, BLU_Pin, GPIO_PIN_SET);

		  // Servo rotate
		  TIM2->CCR1 = 1500;

		  // dc motor runs for user specified time
		  while (time[3] <= clock_hours && clock_hours <= time[4] && temp == 0) {
			  temp = 0;
			  // Timer
			  TIM3->PSC = 160-1;
			  TIM3->ARR = 100-1;
			  TIM3->CCR1 = 0;

			  distance_sensor();

			  // Terminal Display
			  if(clock_hours != pastHour){
				  sprintf((char*)txd_msg_buffer12, "\r\nTIME: %d ZONE 1 -- PWM: %d -- RPM: %d DEPTH: %d", clock_hours,  zones[1], (int) rpm, distance);
				  HAL_UART_Transmit(&huart6, txd_msg_buffer12, strlen((char*)txd_msg_buffer12), 1000);
				  pastHour = clock_hours;
			  }

			  // If distance is less than 50, then resovoir is considered full
			  if (distance >= 50)
			  {
				  digits_set();

				  if (digits[1] == 0 && digits[0] == 0){
					  temp = 1;
					  break;
				  }

				  // dc motor speed
				  if (zones[1] == 0)
				  {
					  manual_mode_ZONES();
				  }

				  else
				  {
					  TIM3->CCR3 = zones[1];
				  }


			  }

//			  else {
//
//				  if (distance != 0){
//					  TIM3->CCR3 = 0;
//					  DIGIT_A_Display(9);
//					  DIGIT_B_Display(9);
//				  }
//
//			  }

		  }

		  if (temp == 1)
		  {
			  break;
		  }

		  TIM3->CCR3 = 0;
		  rcv_intpt_flag = 00;



		  // ZONE 2

		  //sec = 0;

		  // Set color to green
		  HAL_GPIO_WritePin(GPIOA, GRN_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(GPIOA, BLU_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(GPIOA, RFD_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(GPIOA, GRN_Pin, GPIO_PIN_SET);

		  // Servo rotate
		  TIM2->CCR1 = 1000;

		  // dc motor runs for user specified time
		  while (time[5] <= clock_hours && clock_hours <= time[6] && temp == 0)
		  {
			  temp = 0;
			  // Timer
			  TIM3->PSC = 160-1;
			  TIM3->ARR = 100-1;
			  TIM3->CCR1 = 0;

			  distance_sensor();

			  // Terminal Display
			  if(clock_hours != pastHour) {
				  sprintf((char*)txd_msg_buffer13, "\r\nTIME: %d ZONE 2 -- PWM: %d -- RPM: %d DEPTH: %d", clock_hours,  zones[2], (int) rpm, distance);
				  HAL_UART_Transmit(&huart6, txd_msg_buffer13, strlen((char*)txd_msg_buffer13), 1000);
				  pastHour = clock_hours;
			  }


			  // If distance is less than 50, then resovoir is considered full
			  if (distance >= 50)
			  {
				  digits_set();

				  if (digits[1] == 0 && digits[0] == 0)
				  {
					  temp = 1;
					  break;
				  }

				  // dc motor speed
				  if (zones[2] == 0)
				  {
					  manual_mode_ZONES();
				  }

				  else
				  {
					  TIM3->CCR3 = zones[2];
				  }
			  }

//			  else {
//
//				  if (distance != 0){
//					  TIM3->CCR3 = 0;
//					  DIGIT_A_Display(9);
//					  DIGIT_B_Display(9);
//				  }
//
//			  }

		  }

		  if (temp == 1)
		  {
			  break;
		  }

		  TIM3->CCR3 = 0;
		  rcv_intpt_flag = 00;



		  // ZONE 3

		  //sec = 0;
		  // Set color to blue
		  HAL_GPIO_WritePin(GPIOA, GRN_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(GPIOA, BLU_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(GPIOA, RFD_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(GPIOA, RFD_Pin, GPIO_PIN_SET);

		  // Servo rotate
		  TIM2->CCR1 = 500;

		  // dc motor runs for user specified time
		  while (time[7] <= clock_hours && clock_hours <= time[8] && temp == 0)
		  {
			  temp = 0;
			  // Timer
			  TIM3->PSC = 160-1;
			  TIM3->ARR = 100-1;
			  TIM3->CCR1 = 0;

			  distance_sensor();

			  // Terminal Display
			  if(clock_hours != pastHour){
				  sprintf((char*)txd_msg_buffer14, "\r\nTIME: %d ZONE 3 -- PWM: %d -- RPM: %d DEPTH: %d", clock_hours,  zones[3], (int) rpm, distance);
				  HAL_UART_Transmit(&huart6, txd_msg_buffer14, strlen((char*)txd_msg_buffer14), 1000);
				  pastHour = clock_hours;
			  }


			  // If ditance is less than 50, then resovoir is considered full
			  if (distance >= 50)
			  {
				  digits_set();

				  if (digits[1] == 0 && digits[0] == 0)
				  {
					  temp = 1;
					  break;
				  }

				  // dc motor speed
				  if (zones[3] == 0)
				  {
					  manual_mode_ZONES();
				  }

				  else
				  {
					  TIM3->CCR3 = zones[3];
				  }
			  }

//			  else {
//
//				  if (distance != 0){
//					  TIM3->CCR3 = 0;
//					  DIGIT_A_Display(9);
//					  DIGIT_B_Display(9);
//				  }
//
//			  }

		  }

		  if (temp == 1)
		  {
			  break;
		  }
		  if(clock_hours != pastHour){
			  sprintf((char*)txd_msg_buffer14, "\r\nTIME: %d IDLE -- PWM: %d -- RPM: %d DEPTH: %d", clock_hours,  zones[3], (int) rpm, distance);
			  HAL_UART_Transmit(&huart6, txd_msg_buffer14, strlen((char*)txd_msg_buffer14), 1000);
			  pastHour = clock_hours;
		  }
		  distance_sensor();

		  TIM3->CCR3 = 0;
		  rcv_intpt_flag = 00;

	}
	  //sec = 0;

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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_8B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_9;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  htim2.Init.Prescaler = 16-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 20000-1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
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
  sConfigOC.Pulse = 500;
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
  htim3.Init.Prescaler = 160-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 100-1;
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
  sConfigOC.Pulse = 10-1;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.Pulse = 0;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

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

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 16000-1;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 1000-1;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
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
  huart1.Init.BaudRate = 9600;
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
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 9600;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

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
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|BLU_Pin|GRN_Pin|RFD_Pin
                          |DIGIT_A3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, DIGIT_B0_Pin|DIGIT_B3_Pin|DIGIT_B2_Pin|DIGIT_B1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, DIGIT_A2_Pin|DIGIT_A1_Pin|DIGIT_A0_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin BLU_Pin GRN_Pin RFD_Pin
                           DIGIT_A3_Pin */
  GPIO_InitStruct.Pin = LD2_Pin|BLU_Pin|GRN_Pin|RFD_Pin
                          |DIGIT_A3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : DIGIT_B0_Pin DIGIT_B3_Pin DIGIT_B2_Pin DIGIT_B1_Pin */
  GPIO_InitStruct.Pin = DIGIT_B0_Pin|DIGIT_B3_Pin|DIGIT_B2_Pin|DIGIT_B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : DIGIT_A2_Pin DIGIT_A1_Pin DIGIT_A0_Pin */
  GPIO_InitStruct.Pin = DIGIT_A2_Pin|DIGIT_A1_Pin|DIGIT_A0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : RPM_Tick_Pin */
  GPIO_InitStruct.Pin = RPM_Tick_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(RPM_Tick_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){


	if(GPIO_Pin == RPM_Tick_Pin)
	{
		rpm_tick_count += 1;
	}


}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART6)
	{
		HAL_UART_Transmit(&huart6, &byte, 1, 100);
		rcv_intpt_flag = 1;
	}

	if (huart->Instance == USART1)
	{
//		if (distance != 0){
			us100_Rx_flag = 01;
//		}
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM5)
	{
		seconds += 1;
		sec += 1;
		clock_seccs += 1; // for RPM
		clock_secs += 1; // this could be a variable for seconds etc.

		if(sec % 2 == 0) {
			clock_hours++;
		}

		if(clock_hours >= 24) {
			clock_hours = 0;
			sec = 0;
		}

		// RPM Calculation
		if (clock_seccs == 2)
		{
			rpm = rpm_tick_count / 20;
			rpm = (rpm/clock_seccs)*60;
			clock_seccs = 0;
			rpm_tick_count = 0;
		}
	}
}
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
