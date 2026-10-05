/*
 * convert_data_uart.c
 *
 *  Created on: Mar 3, 2025
 *      Author: thuanphat
 */
#include "cJSON.h"
#include "config.h"
#include "main.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <math.h>
#include "ota_update.h"
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart4;
extern UART_HandleTypeDef huart5;
extern bool is_publish_data_lcd;
char rx_buffer_ec[20];
char rx_buffer_ph[20];
char rx_buffer_do[100];
char rx_buffer_fuvitech[100];
char data_status_pump[50]="{\"deviceID\":\"%s\",\"1\":%d,\"2\":%d,\"3\":%d}\r\n";
char tx5_status_pump[50];
int temp_cycle=duty_cycles_ph;

uint8_t payLoadPin;


 uint8_t  rx_idle_buf[RX_IDLE_BUF_SZ];

 volatile uint16_t g_rx_len  = 0;
 volatile uint8_t  g_rx_flag = 0;      // cờ báo có dữ liệu mới
 char g_rx_line[RX_LINE_MAX];          // chuỗi nhận được (null-terminated)

void uart5_rx_start_to_idle(void)
{
    HAL_UARTEx_ReceiveToIdle_IT(&huart5, rx_idle_buf, RX_IDLE_BUF_SZ);

    // hay dùng để bỏ half-transfer (nếu RX dùng DMA)
    if (huart5.hdmarx) {
        __HAL_DMA_DISABLE_IT(huart5.hdmarx, DMA_IT_HT);
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
  if (huart->Instance == USART1) {
    uint16_t text_size = Size;

    /* Preserve raw modem bytes for HTTPREAD before this shared buffer is
     * cleared/re-armed below. */
    if (ota_uart_rx_capture((const uint8_t *)rx_buffer, Size)) {
      memset(rx_buffer, '\0', sizeof(rx_buffer));
      HAL_UARTEx_ReceiveToIdle_IT(&huart1, (uint8_t *)rx_buffer,
                                 sizeof(rx_buffer));
      return;
    }
    if (text_size >= sizeof(rx_buffer)) {
      text_size = sizeof(rx_buffer) - 1U;
    }
    rx_buffer[text_size] = '\0';
    memcpy(rx_data_sim, rx_buffer, text_size + 1U);
//    printf("\r\nSIMCOM Response:");
//    printf(rx_buffer);

#if INTERVAL_PUPLISH_DATA >= 60
    {
    	snprintf(rx_data_sim, sizeof(rx_buffer), "%s", rx_buffer);
    }
#else
    for (int i = 0; i < (int)text_size - 31; i++) {
    if ((char)rx_buffer[i] == (char)SERIAL_NUMBER[5] && (char)rx_buffer[i + 1] == (char)SERIAL_NUMBER[6] &&
        (char)rx_buffer[i + 2] == (char)SERIAL_NUMBER[7]) {
      payLoadPin = (rx_buffer[i + 4] - 48);
#if SIMCOM_MODEL == a7672s
      if (rx_buffer[(i + 29)] == 49 && is_pb_done == true)
#elif SIMCOM_MODEL == a7670c
      if (rx_buffer[(i + 31)] == 49 && is_pb_done == true)
#elif SIMCOM_MODEL == a7670sa
      if (rx_buffer[(i + 29)] == 49 && is_pb_done == true)
#elif SIMCOM_MODEL == a7680
      if (rx_buffer[(i + 29)] == 49 && is_pb_done == true)
#endif
      {
        printf("-----------ON RELAY %d -----------\r\n", payLoadPin);
        if(payLoadPin==1)
        {
        	temp_cycle=temp_cycle+10;
        	if(temp_cycle>100){
        		temp_cycle=100;
        	}
			 HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
			__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_4, duty_cycles_ph);
			motor_ph_plus=1;
        }
        if(payLoadPin==2)
        {
			 HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
			__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_3, duty_cycles_ec);
			motor_ph_minus=1;
        }
        if(payLoadPin==3)
        {
        	 HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
        	__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_1, duty_cycles_x);
        	motor_x=1;
        }
        sprintf(tx5_status_pump,data_status_pump,SERIAL_NUMBER,motor_ph_plus,motor_ph_minus,motor_x);
        is_publish_data_lcd = update_data_to_sreen((uint8_t *)tx5_status_pump);
      }

#if SIMCOM_MODEL == a7672s
      if (rx_buffer[(i + 29)] == 48 && is_pb_done == true)
#elif SIMCOM_MODEL == a7670c
      if (rx_buffer[(i + 31)] == 48 && is_pb_done == true)
#elif SIMCOM_MODEL == a7670sa
      if (rx_buffer[(i + 29)] == 48 && is_pb_done == true)
#elif SIMCOM_MODEL == a7680
      if (rx_buffer[(i + 29)] == 48 && is_pb_done == true)
#endif
      {
        printf("-----------OFF RELAY %d -----------\r\n", payLoadPin);
        if(payLoadPin==1){
        	HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_4);
        	motor_ph_plus=0;
        }
        if(payLoadPin==2){
        	HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
        	motor_ph_minus=0;
        }
        if(payLoadPin==3){
        	HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
        	motor_x=0;
        }
        sprintf(tx5_status_pump,data_status_pump,SERIAL_NUMBER,motor_ph_plus,motor_ph_minus,motor_x);
        is_publish_data_lcd = update_data_to_sreen((uint8_t *)tx5_status_pump);
      }

    }
  }
#endif
    if ((strstr((char *)rx_buffer, "+CMQTTCONNLOST") != NULL) && is_pb_done == true) {
      printf("--------------Client Disconnect passively!---------------\n");
      current_status_simcom = On;
    }
    if (((strstr((char *)rx_buffer, "ota_check") != NULL) ||
         (strstr((char *)rx_buffer, "ota_update") != NULL)) &&
        is_pb_done == true) {
      printf("--------------OTA request received---------------\r\n");
      to_start_ota = true;
    }
  }
  else if (huart->Instance == UART5)
  {
      // ===== THÊM MỚI CHO UART5 (ESP32 -> STM32) =====
	  printf("-----------UART IT 5 -----------\r\n");
      if (Size >= RX_LINE_MAX) Size = RX_LINE_MAX - 1;

      memcpy(g_rx_line, rx_idle_buf, Size);
      g_rx_line[Size] = '\0';

      g_rx_len  = Size;
      g_rx_flag = 1;

      uart5_rx_start_to_idle(); // re-arm
  }
  else if (huart->Instance == USART2) {
        HAL_UARTEx_ReceiveToIdle_IT(&huart2, (uint8_t *)rx_buffer_fuvitech,100);
  }
  else if (huart->Instance == UART4) {
    // sensor PH
    HAL_UARTEx_ReceiveToIdle_IT(&huart4, (uint8_t *)rx_buffer_ph, 20);
  }
  memset(rx_buffer, '\0', 700);
  HAL_UARTEx_ReceiveToIdle_IT(&huart1, (uint8_t *)rx_buffer, 700);
}
