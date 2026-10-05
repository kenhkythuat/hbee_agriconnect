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
#include <stdbool.h>

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#define RX_IDLE_BUF_SZ   128
#define RX_LINE_MAX      256

#define OFFLINE      0
#define ONLINE      1

/*
 * 0: disable automatic pH pump schedule from g_control_mode/config.
 *    Pumps are controlled only by MQTT command received on UART1.
 * 1: enable the old automatic schedule again.
 */
#define PH_PUMP_SCHEDULE_ENABLE 0

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern TIM_HandleTypeDef htim2;
extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
typedef enum {
    PH_ZONE_OK = 0,     // trong ngưỡng: cả 2 OFF
    PH_ZONE_LOW,        // pH < ph_low : bazo chạy theo lịch
    PH_ZONE_HIGH        // pH > ph_high: acid chạy theo lịch
} ph_zone_t;

typedef struct {
    float    ph_high;
    float    ph_low;
    uint32_t time_off;
    uint32_t time_on;
    uint32_t pump_power;
    uint32_t control_mode;
} ph_ctrl_cfg_t;
enum GmsModemState
{
	Off,
	On,
	InternetReady,
	MqttReady,
	Subscribed,
	UpdateToServer,
	DisconnectMqtt,
	SleepStm32,
	ControlModeOffline
};
extern enum GmsModemState current_status_simcom;
extern char rx_buffer[700];
extern char rx_data_sim[700];
extern char array_json[400];
extern char rx_buffer_ec[20];
extern char rx_buffer_ph[20];
extern char rx_buffer_do[100];;
extern char rx_buffer_fuvitech[100];
extern char tx5_status_pump[50];
extern char data_status_pump[50];
extern float data_ph_fuvitech;
extern uint16_t adc_pin_valve;
extern float data_percentage_pin;
extern uint16_t frequency_1hz;
extern bool to_send_status_to_server;
extern volatile uint32_t g_control_mode;
//extern volatile bool control_mode;
extern uint8_t total_errors;
extern volatile bool to_start_ota;
// data EC Fuvitech
extern float data_measured_ph_fuvitech;
extern float data_temperature_ph_fuvitech;
// data EC Fuvitech
extern float data_common_sensor_fuvitech;
extern float data_conductivity_ec_fuvitech;
extern float data_resistivity_ec_fuvitech;
extern float data_temperateure_ec_fuvitech;
extern float data_tds_ec_fuvitech;
extern float data_salinity_ec_fuvitech;
extern float data_dissolved_oxygen_fuvitech;

extern float data_dissolved_oxygen_rika;
extern float data_temperature_do_rika;


extern bool is_pb_done;
extern volatile uint8_t motor_ph_plus;
extern volatile uint8_t motor_ph_minus;
extern bool motor_x;

extern bool is_publish_data_lcd;

extern uint8_t check_sensor_ph_error;
extern uint8_t check_sensor_ec_error;
extern uint8_t check_sensor_do_error;
extern uint8_t is_init_setup_do;

extern uint8_t  rx_idle_buf[RX_IDLE_BUF_SZ];

extern volatile uint16_t g_rx_len;
extern volatile uint8_t  g_rx_flag;      // cờ báo có dữ liệu mới
extern char g_rx_line[RX_LINE_MAX];          // chuỗi nhận được (null-terminated)

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
extern void check_handle_state(enum GmsModemState status);
bool wait_for_pb_done_event(void);
bool check_signal_simcom(void);
float read_level_pin(void);
void read_sensor(void);
extern bool update_data_to_sreen(uint8_t *data);
extern void uart5_rx_start_to_idle(void);
extern void create_JSON(void);
extern void create_JSON_LCD(void);
extern void process_uart_rx(void);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ENABLE_SENSOR_Pin GPIO_PIN_4
#define ENABLE_SENSOR_GPIO_Port GPIOA
#define LED_STATUS_Pin GPIO_PIN_12
#define LED_STATUS_GPIO_Port GPIOB
#define PWRKEY_SIMCOM_Pin GPIO_PIN_11
#define PWRKEY_SIMCOM_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
