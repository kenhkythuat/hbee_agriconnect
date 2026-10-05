/*
 * common_simcom.c
 *
 *  Created on: Mar 3, 2025
 *      Author: thuanphat
 */
#include "cJSON.h"
#include "config.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <main.h>
#include <stdbool.h>
#include "ph_pump_scheduler.h"
#include "ph_pump_isr.h"

/* Private variables ---------------------------------------------------------*/
extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef huart5;
/* USER CODE BEGIN 0 */
char rx_data_sim[700];
char array_at_command[400];
char array_json[400];

char test_lcd[10]="hello";
ph_ctrl_cfg_t g_cfg_simcom;

int previousTick;
bool is_pb_done = false;
bool is_enable_mqtt = false;
bool is_connected_mqtt = false;
bool is_checked_sim = false;
bool is_subcribed_mqtt = false;
bool is_published_mqtt = false;
bool is_acquiered_mqtt = false;
bool is_updated_status = false;
uint8_t total_errors = 0;

bool is_at_connect_mqtt = false;
bool is_at_acquier_mqtt = false;
bool is_at_subcribe_topic_mqtt = false;
bool is_at_subcribe_mqtt = false;
bool is_at_topic_puplish_mqtt = false;
bool is_at_data_puplish_mqtt = false;
bool is_at_puplish_mqtt = false;
bool is_at_check_dis_mqtt = false;
bool is_at_disconnect_mqtt = false;
bool is_at_rel_mqtt = false;
bool is_at_stop_mqtt = false;
bool is_inital_check = false;
bool to_send_status_to_server = false;
//volatile bool control_mode;

uint16_t count_errors = 0;
int timeout_pb_done = 60000;
int is_connect_simcom = 0;
int rssi = -99;

uint16_t frequency_1hz = 0;


volatile uint8_t motor_ph_plus =false;
volatile uint8_t motor_ph_minus=false;
bool motor_x=false;

uint16_t frequency_1hz_timer2;

bool is_publish_data_lcd = false;

uint8_t check_sensor_ph_error=0;
uint8_t check_sensor_ec_error=0;
uint8_t check_sensor_do_error=0;
/* USER CODE END 0 */

//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
//  if (htim->Instance == htim6.Instance) {
//    if (current_status_simcom == Off) {
//      frequency_1hz++;
//      if (frequency_1hz >= INTERVAL_PUPLISH_DATA) {
//        current_status_simcom = UpdateToServer;
//        to_send_status_to_server = 1;
//        frequency_1hz = 0;
//        data_measured_ph_fuvitech++;
//        printf("Case error log total :%d\r\n", total_errors);
//      }
//    }
////    HAL_TIM_Base_Start_IT(&htim6);
//  }
//  if (htim->Instance == htim3.Instance) {
//	  frequency_1hz_timer2++;
//  }
//
//}




void send_to_simcom_a76xx(char *cmd) {
  printf("STM32 Write: %s", cmd);
  HAL_UART_Transmit(&huart1, (uint8_t *)cmd, strlen(cmd), 1000);
}

void restart_stm32(void) {
  printf("\r\n-----------------Restart STM32------------------\r\n");
  printf("\r\n-----------------GOOD BYE !------------------\r\n");
#if SAVE_LOAD
  write_status_load();
#endif
  NVIC_SystemReset();
}

void enable_simcom(void) {
  printf("Enable SIMCOM\n");
#if SIMCOM_MODEL == a7080
  send_to_simcom_a76xx("AT+CPOWD=1\r\n");
  HAL_Delay(5000);
  HAL_GPIO_WritePin(PWRKEY_SIMCOM_GPIO_Port, PWRKEY_SIMCOM_Pin, GPIO_PIN_SET);
  HAL_Delay(4000);
  HAL_GPIO_WritePin(PWRKEY_SIMCOM_GPIO_Port, PWRKEY_SIMCOM_Pin, GPIO_PIN_RESET);
  HAL_Delay(4000);
#else
  HAL_GPIO_WritePin(GPIOA, ENABLE_SENSOR_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PWRKEY_SIMCOM_GPIO_Port, PWRKEY_SIMCOM_Pin, GPIO_PIN_SET);
  HAL_Delay(3300);
  HAL_GPIO_WritePin(PWRKEY_SIMCOM_GPIO_Port, PWRKEY_SIMCOM_Pin, GPIO_PIN_RESET);
  HAL_Delay(3300);
  HAL_GPIO_WritePin(PWRKEY_SIMCOM_GPIO_Port, PWRKEY_SIMCOM_Pin, GPIO_PIN_SET);
  HAL_Delay(500);
  HAL_GPIO_WritePin(PWRKEY_SIMCOM_GPIO_Port, PWRKEY_SIMCOM_Pin, GPIO_PIN_RESET);
  HAL_Delay(1000);
#endif
}

bool wait_for_pb_done_event(void) {
  HAL_GPIO_WritePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin, RESET);
  previousTick = HAL_GetTick();
  while (is_inital_check == 0 && previousTick + timeout_pb_done > HAL_GetTick()) {
#if SIMCOM_MODEL == a7080
    send_to_simcom_a76xx("AT+CFGRI=1\r\n");
    HAL_Delay(200);
    send_to_simcom_a76xx("ATE0\r\n");
    HAL_Delay(200);
    send_to_simcom_a76xx("AT+CFUN?\r\n");
    HAL_Delay(200);
    send_to_simcom_a76xx("AT+CGDCONT?\r\n");
    HAL_Delay(200);
    send_to_simcom_a76xx("ATI\r\n");
    HAL_Delay(200);
    send_to_simcom_a76xx("AT+CGREG?\r\n");
    HAL_Delay(200);
    send_to_simcom_a76xx("AT+CBAND?\r\n");
    HAL_Delay(200);
    send_to_simcom_a76xx("ATE0\r\n");
    HAL_Delay(200);
    send_to_simcom_a76xx("AT+CCID\r\n");
    HAL_Delay(200);
    send_to_simcom_a76xx("AT+CNSMOD?\r\n");
    HAL_Delay(200);
    send_to_simcom_a76xx("ATI\r\n");
    HAL_Delay(200);
    send_to_simcom_a76xx("AT+CICCID\r\n");
    HAL_Delay(200);
    send_to_simcom_a76xx("AT+CMNB?\r\n");
    HAL_Delay(200);
    send_to_simcom_a76xx("AT+COPS=0\r\n");
    HAL_Delay(200);
    send_to_simcom_a76xx("AT+COPS?\r\n");
    HAL_Delay(200);
    read_signal_quality();
    HAL_Delay(200);
    send_to_simcom_a76xx("AT+CBANDCFG=\"NB-IOT\",1,3,4,5,8,13,18,19,20,25,26,28,66,71,85\r\n");
    HAL_Delay(200);
    send_to_simcom_a76xx("AT+CBANDCFG?\r\n");
    HAL_Delay(200);
    send_to_simcom_a76xx("AT+CGNAPN\r\n");
    HAL_Delay(200);
    send_to_simcom_a76xx("AT+CGATT?\r\n");
    HAL_Delay(500);
    return true;
#elif SIMCOM_MODEL == a7680
    HAL_Delay(12000);
    send_to_simcom_a76xx("ATE0\r\n");
    HAL_Delay(200);
    send_to_simcom_a76xx("AT+CCIOTOPTI?\r\n");
    HAL_Delay(200);
    return true;
//	}
#elif SIMCOM_MODEL == a7677
    if (strstr((char *)rx_data_sim, "EPS PDN ACT 1")) {
      is_pb_done = 1;
      led_status('B');
      HAL_Delay(8000);
      return true;
    }
#else
    {
      if (strstr((char *)rx_data_sim, "PB DONE")) {
        send_to_simcom_a76xx("ATE0\r\n");
        HAL_Delay(200);
        is_connect_simcom =1;
        return true;
      }
    }

#endif
  }
  if (is_connect_simcom == 0) {
    NVIC_SystemReset();
  }
  return false;
}

int read_signal_quality(void) {
  send_to_simcom_a76xx("AT+CSQ\r\n");
  HAL_Delay(200);
  int signal, error;
  char *ptr = strstr(rx_data_sim, "+CSQ:");
  if (ptr && sscanf(ptr, "+CSQ: %d,%d", &signal, &error) == 2) {
    printf("Cuong do tin hieu: %d\n", signal);
    printf("Gia tri loi: %d\n", error);
  } else {
    printf("khong the phan tich chuoi @@ !\n");
  }
  if (signal >= 31) {
    rssi = -51;
  } else if (signal <= 0) {
    printf("not known or not detectable !\r\n");
    rssi = -113;
  } else if (signal == 99) {
    printf("not known or not detectable !\r\n");
    rssi = -113;
  } else
    rssi = (signal * 2 - 113);
  return rssi;
}

bool check_signal_simcom(void) {
  printf("-----------------check_signal_simcom------------------\n");
  read_signal_quality();
  HAL_Delay(200);
  is_connect_simcom = 1;
  HAL_Delay(200);
  send_to_simcom_a76xx("AT+CPIN?\r\n");
  HAL_Delay(200);
  if (strstr((char *)rx_data_sim, "+CPIN: READY")) {
    printf("-----------------SIM OK !------------------\n");
  } else
    return false;
  HAL_Delay(200);
  send_to_simcom_a76xx("AT+CGREG?\r\n");
  HAL_Delay(200);
  send_to_simcom_a76xx("AT+CREG?\r\n");
  HAL_Delay(200);
  send_to_simcom_a76xx("ATI\r\n");
  HAL_Delay(200);
  send_to_simcom_a76xx("AT+CICCID\r\n");
  HAL_Delay(200);
  send_to_simcom_a76xx("AT+CGREG?\r\n");
  HAL_Delay(400);
  if (strstr((char *)rx_data_sim, "+CGREG: 0,1")) {
    printf("-----------------Network registration OK!------------------\n");
  } else
    return false;
  return true;
}

bool enable_mqtt_on_gsm_modem(void) {
  is_enable_mqtt = false;
#if SIMCOM_MODEL == a7080
  send_to_simcom_a76xx("AT+CNACT=0,1\r\n");
  HAL_Delay(400);
  send_to_simcom_a76xx("AT+CNACT?\r\n");
  HAL_Delay(400);
  send_to_simcom_a76xx("AT+SMCONF=\"URL\",\"mqtt.agriconnect.vn\",1883\r\n");
  HAL_Delay(400);
  send_to_simcom_a76xx("AT+SMCONF=\"KEEPTIME\",600\r\n");
  HAL_Delay(400);
  sprintf(array_at_command, "AT+SMCONF=\"CLIENTID\",\"%s\"\r\n", MQTT_CLIENT_ID);
  send_to_simcom_a76xx(array_at_command);
  HAL_Delay(400);
  sprintf(array_at_command, "AT+SMCONF=\"USERNAME\",\"%s\"\r\n", MQTT_USER);
  send_to_simcom_a76xx(array_at_command);
  HAL_Delay(400);
  sprintf(array_at_command, "AT+SMCONF=\"PASSWORD\",\"%s\"\r\n", MQTT_PASS);
  send_to_simcom_a76xx(array_at_command);
  HAL_Delay(400);
  send_to_simcom_a76xx("AT+SMCONF=\"CLEANSS\",1\r\n");
  HAL_Delay(400);
  send_to_simcom_a76xx("AT+SMCONF?\r\n");
  HAL_Delay(400);
  memset(rx_data_sim, '\0', 1500);
  HAL_Delay(400);
  send_to_simcom_a76xx("AT+SMCONN\r\n");
  HAL_Delay(9000);
  if ((strstr((char *)rx_data_sim, "OK") != NULL)) {
    printf("----------Service have started SIMCOM 7080G "
           "successfully------------\n");
    is_connected_mqtt = 1;
    return true;
  }
#else
  memset(rx_data_sim, '\0', 700);
  send_to_simcom_a76xx("AT+CMQTTSTART\r\n");
  HAL_Delay(3000);
  if ((strstr((char *)rx_data_sim, "+CMQTTSTART: 0") != NULL)||(strstr((char *)rx_data_sim, "OK")!= NULL)) {
    printf("----------Service have started successfully------------\n");
    return true;
  } else {
    printf("----------------- Start MQTT service fail------------------\n");
    return false;
  }
#endif
  return false;
}

bool acquire_gsm_mqtt_client(void) {
  is_acquiered_mqtt = false;
  memset(rx_data_sim, '\0', 700);
  printf("-----------------Acquire_gsm_mqtt_client------------------\n");
  sprintf(array_at_command, "+CMQTTACCQ: 0,\"%s\",0\r\n", MQTT_CLIENT_ID);
  send_to_simcom_a76xx("AT+CMQTTACCQ?\r\n");
  HAL_Delay(400);
  if (strstr((char *)rx_data_sim, array_at_command) != NULL) {
    printf("-----------------Had acquired------------------\n");
    return true;
  } else {
    printf("-----------------Haven't got acquire yet------------------\n");
    is_at_acquier_mqtt = false;
  }
  if (is_at_acquier_mqtt == false) {
    sprintf(array_at_command, "AT+CMQTTACCQ=0,\"%s\",0\r\n", MQTT_CLIENT_ID);
    send_to_simcom_a76xx(array_at_command);
    HAL_Delay(300);
    //sprintf(array_at_command, "+CMQTTACCQ: 0,\"%s\",0", MQTT_CLIENT_ID);
    HAL_Delay(300);
    if (strstr((char *)rx_data_sim, "OK") != NULL) {
      printf("-----------------Acquire Successfully------------------\n");
      is_at_acquier_mqtt = true;
      return true;
    } else {
      printf("-----------------Acquire Fail------------------\n");
    }
  }
  return false;
}

bool connect_mqtt_server_by_gsm(void) {
  is_connected_mqtt = false;
  sprintf(array_at_command, "+CMQTTCONNECT: 0,\"%s:%d\",60,1,\"%s\",\"%s\"\r\n", MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS);
  HAL_Delay(200);
  send_to_simcom_a76xx("AT+CMQTTCONNECT?\r\n");
  HAL_Delay(200);
  if (strstr((char *)rx_data_sim, array_at_command) != NULL) {
    printf("-----------------Connected------------------\n");
    is_at_connect_mqtt = true;
    return true;
  } else {
    printf("-----------------Not connect yet !------------------\n");
    is_at_connect_mqtt = false;
  }
  if (is_at_connect_mqtt == false) {
	memset(rx_data_sim, '\0', 700);
    sprintf(array_at_command, "AT+CMQTTCONNECT=0,\"%s:%d\",60,1,\"%s\",\"%s\"\r\n", MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS);
    send_to_simcom_a76xx(array_at_command);
    HAL_Delay(2000);
    if (strstr((char *)rx_data_sim, "+CMQTTCONNECT: 0,0") != NULL || strstr((char *)rx_data_sim, "OK") != NULL) {
      printf("\n-----------------Connected MQTT Success------------------\n");
      return true;
    } else {
      printf("\n-----------------Connect fail------------------\n");
    }
  }
  return false;
}

bool subscribe_mqtt_via_gsm(void) {
#if SIMCOM_MODEL == a7080
  memset(rx_data_sim, '\0', 1500);
  HAL_Delay(400);
  char temp_buffer[200];
  sprintf(temp_buffer, "\"%s/snac/%s/#\"", FARM, SERIAL_NUMBER);
  sprintf(array_at_command, "AT+SMSUB=%s,0\r\n", temp_buffer);
  send_to_simcom_a76xx(array_at_command);
  HAL_Delay(3000);
  if (strstr((char *)rx_data_sim, "OK") != NULL) {
    printf("\n-----------------Subscribe Topic Success------------------\n");
    is_at_subcribe_topic_mqtt = true;
    HAL_GPIO_WritePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin, SET);
    return true;
  } else {
    printf("-----------------Subscribe Fail !------------------\n");
    is_at_subcribe_mqtt = false;
    return false;
  }
#else
  memset(rx_data_sim, '\0', 700);
  sprintf(array_at_command, "%s/snac/%s/#", FARM, SERIAL_NUMBER);
  sprintf(array_at_command, "AT+CMQTTSUBTOPIC=0,%d,1\r\n", (int)strlen(array_at_command));
  send_to_simcom_a76xx(array_at_command);
  HAL_Delay(500);
  sprintf(array_at_command, "%s/snac/%s/#\r\n", FARM, SERIAL_NUMBER);
  send_to_simcom_a76xx(array_at_command);
  HAL_Delay(500);
  if (strstr((char *)rx_data_sim, "OK") != NULL) {
    printf("-----------------Subscribe Topic Success------------------\n");
    is_at_subcribe_topic_mqtt = true;
  } else {
    printf("-----------------Subscribe Topic Fail------------------\n");
    is_at_subcribe_topic_mqtt = false;
  }
  if (is_at_subcribe_topic_mqtt == true) {
    send_to_simcom_a76xx("AT+CMQTTSUB=0\r\n");
    HAL_Delay(1000);
    if (strstr((char *)rx_data_sim, "+CMQTTSUB: 0,0") != NULL) {
      printf("-----------------Subscribe Success !------------------\n");
      is_at_subcribe_mqtt = true;
    } else {
      printf("-----------------Subscribe Fail !------------------\n");
      is_at_subcribe_mqtt = false;
      return false;
    }
  }
  HAL_GPIO_WritePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin, SET);
  return true;
#endif
}

void create_JSON(void) {
  cJSON *json = cJSON_CreateObject();
  rssi = read_signal_quality();
  data_percentage_pin = read_level_pin();
  cJSON_AddNumberToObject(json, "_gsm_signal_strength", rssi);
  cJSON_AddNumberToObject(json, "_battery_level", data_percentage_pin);
  cJSON_AddNumberToObject(json, "control_mode", g_control_mode);
  cJSON_AddStringToObject(json, "version", VERSION_WBEE);


#if ph_fuvitech
  // data PH Fuvitech
//  if(data_measured_ph_fuvitech<1){
//      check_sensor_ph_error++;
//  }
//  else{
//      check_sensor_ph_error=0;
//  }
//  if(check_sensor_ph_error>=3){
//      NVIC_SystemReset();
//  }

  char data_measured_ph_fuvitech_str[16];
  char data_temperature_ph_fuvitech_str[16];
  snprintf(data_measured_ph_fuvitech_str, sizeof(data_measured_ph_fuvitech_str), "%.2f", data_measured_ph_fuvitech);
  snprintf(data_temperature_ph_fuvitech_str, sizeof(data_temperature_ph_fuvitech_str), "%.2f", data_temperature_ph_fuvitech);
  cJSON_AddStringToObject(json, "solPH", data_measured_ph_fuvitech_str);
  cJSON_AddStringToObject(json, "solT", data_temperature_ph_fuvitech_str);
#endif


#if ph_rika500_12
  // data PH Fuvitech
//  if(data_measured_ph_fuvitech<1){
//      check_sensor_ph_error++;
//  }
//  else{
//      check_sensor_ph_error=0;
//  }
//  if(check_sensor_ph_error>=3){
//      NVIC_SystemReset();
//  }

  char data_measured_ph_fuvitech_str[16];
  char data_temperature_ph_fuvitech_str[16];
  snprintf(data_measured_ph_fuvitech_str, sizeof(data_measured_ph_fuvitech_str), "%.2f", data_measured_ph_fuvitech);
  snprintf(data_temperature_ph_fuvitech_str, sizeof(data_temperature_ph_fuvitech_str), "%.2f", data_temperature_ph_fuvitech);
  cJSON_AddStringToObject(json, "solPH", data_measured_ph_fuvitech_str);
  cJSON_AddStringToObject(json, "solT", data_temperature_ph_fuvitech_str);
#endif
#if ec_fuvitech
  if(data_conductivity_ec_fuvitech<1){
      check_sensor_ec_error++;
  }
  else{
      check_sensor_ec_error=0;
  }
  if(check_sensor_ec_error>=3){
      NVIC_SystemReset();
  }
  char data_conductivity_ec_fuvitech_str[16];
  char data_tds_ec_fuvitech_str[16];
  char data_resistivity_ec_fuvitech_str[16];
  char data_salinity_ec_fuvitech_str[16];
  char data_temperature_ec_fuvitech_str[16];
  snprintf(data_conductivity_ec_fuvitech_str, sizeof(data_conductivity_ec_fuvitech_str), "%.2f", data_conductivity_ec_fuvitech);
  snprintf(data_tds_ec_fuvitech_str, sizeof(data_tds_ec_fuvitech_str), "%.2f", data_tds_ec_fuvitech);
  snprintf(data_resistivity_ec_fuvitech_str, sizeof(data_resistivity_ec_fuvitech_str), "%.2f", data_resistivity_ec_fuvitech);
  snprintf(data_salinity_ec_fuvitech_str, sizeof(data_salinity_ec_fuvitech_str), "%.2f", data_salinity_ec_fuvitech);
  snprintf(data_temperature_ec_fuvitech_str, sizeof(data_temperature_ec_fuvitech_str), "%.2f", data_temperateure_ec_fuvitech);
  //   data EC Fuvitech
  cJSON_AddStringToObject(json, "solEC", data_conductivity_ec_fuvitech_str);
  cJSON_AddStringToObject(json, "solTDS", data_tds_ec_fuvitech_str);
  cJSON_AddStringToObject(json, "solRes", data_resistivity_ec_fuvitech_str);
  cJSON_AddStringToObject(json, "solSal", data_salinity_ec_fuvitech_str);
  cJSON_AddStringToObject(json, "solT", data_temperature_ec_fuvitech_str);
#endif
#if ec_rika500_13
//  if(data_conductivity_ec_fuvitech<1){
//      check_sensor_ec_error++;
//  }
//  else{
//      check_sensor_ec_error=0;
//  }
//  if(check_sensor_ec_error>=3){
//      NVIC_SystemReset();
//  }
  char data_conductivity_ec_fuvitech_str[16];
  char data_resistivity_ec_fuvitech_str[16];
  char data_temperature_ec_fuvitech_str[16];
  char data_tds_ec_fuvitech_str[16];
  char data_salinity_ec_fuvitech_str[16];
  snprintf(data_conductivity_ec_fuvitech_str, sizeof(data_conductivity_ec_fuvitech_str), "%.2f", data_conductivity_ec_fuvitech);
  snprintf(data_resistivity_ec_fuvitech_str, sizeof(data_resistivity_ec_fuvitech_str), "%.2f", data_resistivity_ec_fuvitech);
  snprintf(data_temperature_ec_fuvitech_str, sizeof(data_temperature_ec_fuvitech_str), "%.2f", data_temperateure_ec_fuvitech);
  snprintf(data_tds_ec_fuvitech_str, sizeof(data_tds_ec_fuvitech_str), "%.2f", data_tds_ec_fuvitech);
  snprintf(data_salinity_ec_fuvitech_str, sizeof(data_salinity_ec_fuvitech_str), "%.2f", data_salinity_ec_fuvitech);
  //   data EC Fuvitech
  cJSON_AddStringToObject(json, "solEC", data_conductivity_ec_fuvitech_str);
  cJSON_AddStringToObject(json, "solRes", data_resistivity_ec_fuvitech_str);
  cJSON_AddStringToObject(json, "solTDS", data_tds_ec_fuvitech_str);
  cJSON_AddStringToObject(json, "solSal", data_salinity_ec_fuvitech_str);
//  cJSON_AddStringToObject(json, "solT", data_temperature_ec_fuvitech_str);
#endif
#if do_fuvitech
  //   data DO Fuvitech
//  if(data_dissolved_oxygen_fuvitech<1){
//      check_sensor_do_error++;
//  }
//  else{
//      check_sensor_do_error=0;
//  }
//  if(check_sensor_do_error>=3){
//      NVIC_SystemReset();
//  }
  char data_dissolved_oxygen_str[16];
  snprintf(data_dissolved_oxygen_str, sizeof(data_dissolved_oxygen_str), "%.2f", data_dissolved_oxygen_fuvitech);
  cJSON_AddStringToObject(json, "solDO", data_dissolved_oxygen_str);
#endif

#if do_rika500_04

  char data_dissolved_oxygen_str[16];
  snprintf(data_dissolved_oxygen_str, sizeof(data_dissolved_oxygen_str), "%.2f", data_dissolved_oxygen_rika);
  cJSON_AddStringToObject(json, "solDO", data_dissolved_oxygen_str);
#endif

  char *json_string = cJSON_PrintUnformatted(json);
  if (json_string == NULL) {
    printf("New create error JSON\n");
    cJSON_Delete(json);
    return;
  }
  sprintf(array_json, "%s", json_string);
  // decompress memory
  free(json_string);
  cJSON_Delete(json);
}
void create_JSON_LCD(void) {
  cJSON *json = cJSON_CreateObject();
  data_percentage_pin = read_level_pin();
  cJSON_AddNumberToObject(json, "_gsm_signal_strength", rssi);
  cJSON_AddNumberToObject(json, "_battery_level", data_percentage_pin);
  cJSON_AddNumberToObject(json, "control_mode", g_control_mode);


#if ph_fuvitech
  // data PH Fuvitech
//  if(data_measured_ph_fuvitech<1){
//      check_sensor_ph_error++;
//  }
//  else{
//      check_sensor_ph_error=0;
//  }
//  if(check_sensor_ph_error>=3){
//      NVIC_SystemReset();
//  }

  char data_measured_ph_fuvitech_str[16];
  char data_temperature_ph_fuvitech_str[16];
  snprintf(data_measured_ph_fuvitech_str, sizeof(data_measured_ph_fuvitech_str), "%.2f", data_measured_ph_fuvitech);
  snprintf(data_temperature_ph_fuvitech_str, sizeof(data_temperature_ph_fuvitech_str), "%.2f", data_temperature_ph_fuvitech);
  cJSON_AddStringToObject(json, "solPH", data_measured_ph_fuvitech_str);
  cJSON_AddStringToObject(json, "solT", data_temperature_ph_fuvitech_str);
#endif

#if ph_rika500_12
  // data PH Fuvitech
//  if(data_measured_ph_fuvitech<1){
//      check_sensor_ph_error++;
//  }
//  else{
//      check_sensor_ph_error=0;
//  }
//  if(check_sensor_ph_error>=3){
//      NVIC_SystemReset();
//  }

  char data_measured_ph_fuvitech_str[16];
  char data_temperature_ph_fuvitech_str[16];
  snprintf(data_measured_ph_fuvitech_str, sizeof(data_measured_ph_fuvitech_str), "%.2f", data_measured_ph_fuvitech);
  snprintf(data_temperature_ph_fuvitech_str, sizeof(data_temperature_ph_fuvitech_str), "%.2f", data_temperature_ph_fuvitech);
  cJSON_AddStringToObject(json, "solPH", data_measured_ph_fuvitech_str);
  cJSON_AddStringToObject(json, "solT", data_temperature_ph_fuvitech_str);
#endif

#if ec_fuvitech
  if(data_conductivity_ec_fuvitech<1){
      check_sensor_ec_error++;
  }
  else{
      check_sensor_ec_error=0;
  }
  if(check_sensor_ec_error>=3){
      NVIC_SystemReset();
  }
  char data_conductivity_ec_fuvitech_str[16];
  char data_tds_ec_fuvitech_str[16];
  char data_resistivity_ec_fuvitech_str[16];
  char data_salinity_ec_fuvitech_str[16];
  char data_temperature_ec_fuvitech_str[16];
  snprintf(data_conductivity_ec_fuvitech_str, sizeof(data_conductivity_ec_fuvitech_str), "%.2f", data_conductivity_ec_fuvitech);
  snprintf(data_tds_ec_fuvitech_str, sizeof(data_tds_ec_fuvitech_str), "%.2f", data_tds_ec_fuvitech);
  snprintf(data_resistivity_ec_fuvitech_str, sizeof(data_resistivity_ec_fuvitech_str), "%.2f", data_resistivity_ec_fuvitech);
  snprintf(data_salinity_ec_fuvitech_str, sizeof(data_salinity_ec_fuvitech_str), "%.2f", data_salinity_ec_fuvitech);
  snprintf(data_temperature_ec_fuvitech_str, sizeof(data_temperature_ec_fuvitech_str), "%.2f", data_temperateure_ec_fuvitech);
  //   data EC Fuvitech
  cJSON_AddStringToObject(json, "solEC", data_conductivity_ec_fuvitech_str);
  cJSON_AddStringToObject(json, "solTDS", data_tds_ec_fuvitech_str);
  cJSON_AddStringToObject(json, "solRes", data_resistivity_ec_fuvitech_str);
  cJSON_AddStringToObject(json, "solSal", data_salinity_ec_fuvitech_str);
  cJSON_AddStringToObject(json, "solT", data_temperature_ec_fuvitech_str);
#endif
#if ec_rika500_13
//  if(data_conductivity_ec_fuvitech<1){
//      check_sensor_ec_error++;
//  }
//  else{
//      check_sensor_ec_error=0;
//  }
//  if(check_sensor_ec_error>=3){
//      NVIC_SystemReset();
//  }
  char data_conductivity_ec_fuvitech_str[16];
  char data_resistivity_ec_fuvitech_str[16];
  char data_temperature_ec_fuvitech_str[16];
  char data_tds_ec_fuvitech_str[16];
  char data_salinity_ec_fuvitech_str[16];
  snprintf(data_conductivity_ec_fuvitech_str, sizeof(data_conductivity_ec_fuvitech_str), "%.2f", data_conductivity_ec_fuvitech);
  snprintf(data_resistivity_ec_fuvitech_str, sizeof(data_resistivity_ec_fuvitech_str), "%.2f", data_resistivity_ec_fuvitech);
  snprintf(data_temperature_ec_fuvitech_str, sizeof(data_temperature_ec_fuvitech_str), "%.2f", data_temperateure_ec_fuvitech);
  snprintf(data_tds_ec_fuvitech_str, sizeof(data_tds_ec_fuvitech_str), "%.2f", data_tds_ec_fuvitech);
  snprintf(data_salinity_ec_fuvitech_str, sizeof(data_salinity_ec_fuvitech_str), "%.2f", data_salinity_ec_fuvitech);
  //   data EC Fuvitech
  cJSON_AddStringToObject(json, "solEC", data_conductivity_ec_fuvitech_str);
  cJSON_AddStringToObject(json, "solRes", data_resistivity_ec_fuvitech_str);
  cJSON_AddStringToObject(json, "solTDS", data_tds_ec_fuvitech_str);
  cJSON_AddStringToObject(json, "solSal", data_salinity_ec_fuvitech_str);
//  cJSON_AddStringToObject(json, "solT", data_temperature_ec_fuvitech_str);
#endif
#if do_fuvitech
  //   data DO Fuvitech
//  if(data_dissolved_oxygen_fuvitech<1){
//      check_sensor_do_error++;
//  }
//  else{
//      check_sensor_do_error=0;
//  }
//  if(check_sensor_do_error>=3){
//      NVIC_SystemReset();
//  }
  char data_dissolved_oxygen_str[16];
  snprintf(data_dissolved_oxygen_str, sizeof(data_dissolved_oxygen_str), "%.2f", data_dissolved_oxygen_fuvitech);
  cJSON_AddStringToObject(json, "solDO", data_dissolved_oxygen_str);
#endif


#if do_rika500_04

  char data_dissolved_oxygen_str[16];
  snprintf(data_dissolved_oxygen_str, sizeof(data_dissolved_oxygen_str), "%.2f", data_dissolved_oxygen_rika);
  cJSON_AddStringToObject(json, "solDO", data_dissolved_oxygen_str);
#endif

  char *json_string = cJSON_PrintUnformatted(json);
  if (json_string == NULL) {
    printf("New create error JSON\n");
    cJSON_Delete(json);
    return;
  }
  sprintf(array_json, "%s", json_string);
  // decompress memory
  free(json_string);
  cJSON_Delete(json);
}

void create_Json_status_motor(void)
{
	  cJSON *json = cJSON_CreateObject();
	  cJSON_AddNumberToObject(json, "1", motor_ph_plus);
	  cJSON_AddNumberToObject(json, "2", motor_ph_minus);
	  cJSON_AddNumberToObject(json, "3", motor_x);

	  char *json_string = cJSON_PrintUnformatted(json);
	  if (json_string == NULL) {
	    printf("New create error JSON\n");
	    cJSON_Delete(json);
	    return;
	  }
	  sprintf(array_json, "%s", json_string);
	  // decompress memory
	  free(json_string);
	  cJSON_Delete(json);
}
bool publish_mqtt_via_gsm(void) {
  //  is used to input the topic of a publish message
  create_JSON();
#if SIMCOM_MODEL == a7080
  send_to_simcom_a76xx("AT+SMSTATE?\r\n");
  HAL_Delay(200);
  if (strstr((char *)rx_data_sim, "+SMSTATE: 1") != NULL) {
    printf("-----------------Expression MQTT on-line state!------------------\n");
    //		  return true;
  } else {
    printf("-----------------Expression MQTT off-line state @@ "
           "!------------------\n");
    return false;
  }
  send_to_simcom_a76xx("AT+CGREG?\r\n");
  HAL_Delay(200);
  sprintf(array_at_command, "AT+SMPUB=\"%s\",%d,0,0\r\n", MQTT_TOPIC_ACTUATOR_STATUS, strlen(array_json));
  send_to_simcom_a76xx(array_at_command);
  HAL_Delay(300);
  sprintf(array_at_command, "%s\r\n", array_json);
  send_to_simcom_a76xx(array_at_command);
  HAL_Delay(1000);
  if (strstr((char *)rx_data_sim, "OK") != NULL) {
    printf("-----------------Publish Success to server Agriconnect "
           "!------------------\n");
    led_status('G');
    HAL_Delay(1000);
    return true;
  } else
    printf("-----------------Publish fail !------------------\n");
#else
  sprintf(array_at_command, "AT+CMQTTTOPIC=0,%d\r\n", strlen(MQTT_TOPIC_ACTUATOR_STATUS));
  send_to_simcom_a76xx(array_at_command);
  HAL_Delay(400);
  sprintf(array_at_command, "%s\r\n", MQTT_TOPIC_ACTUATOR_STATUS);
  send_to_simcom_a76xx(array_at_command);
  HAL_Delay(400);
  if (strstr((char *)rx_data_sim, "OK") != NULL) {
    printf("\r\n----- Sent input the topic of a publish message success ! ---------\n");
    is_at_topic_puplish_mqtt = true;
  } else {
    printf("\r\n----------------- Sent input the topic of a publish message fail "
           "!------------------\n");
    is_at_topic_puplish_mqtt = false;
  }
  if (is_at_topic_puplish_mqtt) {
    // is used to input the message body of a publish message.
    int length_array = strlen(array_json);
    sprintf(array_at_command, "AT+CMQTTPAYLOAD=0,%d\r\n", length_array);
    send_to_simcom_a76xx(array_at_command);
    HAL_Delay(400);
    send_to_simcom_a76xx(array_json);
    HAL_Delay(600);
    if (strstr((char *)rx_data_sim, "OK") != NULL) {
      printf("\r\n----------------- Sent input the message body of a publish "
             "message ! ------------------\n");
      is_at_data_puplish_mqtt = true;
    } else {
      printf("\r\n--- Sent input the message body of a publish fail! "
             "--------\n");
      is_at_data_puplish_mqtt = false;
    }

    if (is_at_data_puplish_mqtt) {
      send_to_simcom_a76xx("AT+CMQTTPUB=0,1,60\r\n");
      HAL_Delay(2000);
      if (strstr((char *)rx_data_sim, "+CMQTTPUB: 0,0") != NULL) {
        printf("-----------------Publish Success !------------------\n");
        is_at_puplish_mqtt = true;
        return true;
      } else {
        printf("-----------------Publish fail !------------------\n");
        is_at_puplish_mqtt = false;
      }
    }
  }
#endif
  return false;
}

bool publish_mqtt_motor_status (void)
{
		create_Json_status_motor();
	  sprintf(array_at_command, "AT+CMQTTTOPIC=0,%d\r\n", strlen(MQTT_TOPIC_MOTOR_STATUS));
	  send_to_simcom_a76xx(array_at_command);
	  HAL_Delay(400);
	  sprintf(array_at_command, "%s\r\n", MQTT_TOPIC_MOTOR_STATUS);
	  send_to_simcom_a76xx(array_at_command);
	  HAL_Delay(400);
	  if (strstr((char *)rx_data_sim, "OK") != NULL) {
	    printf("\r\n----- Sent input the topic of a publish message success ! ---------\n");
	    is_at_topic_puplish_mqtt = true;
	  } else {
	    printf("\r\n----------------- Sent input the topic of a publish message fail "
	           "!------------------\n");
	    is_at_topic_puplish_mqtt = false;
	  }
	  if (is_at_topic_puplish_mqtt) {
	    // is used to input the message body of a publish message.
	    int length_array = strlen(array_json);
	    sprintf(array_at_command, "AT+CMQTTPAYLOAD=0,%d\r\n", length_array);
	    send_to_simcom_a76xx(array_at_command);
	    HAL_Delay(400);
	    send_to_simcom_a76xx(array_json);
	    HAL_Delay(600);
	    if (strstr((char *)rx_data_sim, "OK") != NULL) {
	      printf("\r\n----------------- Sent input the message body of a publish "
	             "message ! ------------------\n");
	      is_at_data_puplish_mqtt = true;
	    } else {
	      printf("\r\n--- Sent input the message body of a publish fail! "
	             "--------\n");
	      is_at_data_puplish_mqtt = false;
	    }
	    if (is_at_data_puplish_mqtt) {
	      send_to_simcom_a76xx("AT+CMQTTPUB=0,1,60\r\n");
	      HAL_Delay(2000);
	      if (strstr((char *)rx_data_sim, "+CMQTTPUB: 0,0") != NULL) {
	        printf("-----------------Publish Success !------------------\n");
	        is_at_puplish_mqtt = true;
	        return true;
	      } else {
	        printf("-----------------Publish fail !------------------\n");
	        is_at_puplish_mqtt = false;
	      }
	    }
	  }
	  return false;
}
bool stop_mqtt_via_gsm(void) {
#if SIMCOM_MODEL == a7080
  send_to_simcom_a76xx("AT+SMDISC\r\n");
  if (strstr((char *)rx_data_sim, "OK") != NULL) {
    printf("-------- Disconnect MQTT successfully------------\n");
    return true;
  } else
    return false;
#else
  send_to_simcom_a76xx("AT+CMQTTDISC?\r\n");
  HAL_Delay(500);
  if (strstr((char *)rx_data_sim, "+CMQTTDISC: 0,0") != NULL) {
    printf("----------------- Connection! ------------------\n");
    is_at_check_dis_mqtt = true;
  } else {
    printf("----------------- Disconnect! ------------------\n");
    is_at_check_dis_mqtt = false;
    is_at_disconnect_mqtt = true;
  }
  if (is_at_check_dis_mqtt) {
    send_to_simcom_a76xx("AT+CMQTTDISC=0,120\r\n");
    HAL_Delay(500);
    if (strstr((char *)rx_data_sim, "+CMQTTDISC: 0,0") != NULL) {
      printf("----------------- Disconnect successfully! ------------------\n");
      is_at_disconnect_mqtt = true;
    } else
      return false;
  }
  if (is_at_disconnect_mqtt) {
    send_to_simcom_a76xx("AT+CMQTTREL=0\r\n");
    HAL_Delay(500);
    if (strstr((char *)rx_data_sim, "OK") != NULL) {
      printf("--------------- Release a MQTT client successfully! "
             "-----------------\n");
      is_at_rel_mqtt = true;
    } else
      return false;
  }
  if (is_at_rel_mqtt) {
    send_to_simcom_a76xx("AT+CMQTTSTOP\r\n");
    HAL_Delay(500);
    if (strstr((char *)rx_data_sim, "OK") != NULL) {
      printf("----------------- Stop MQTT service successfully! "
             "------------------\n");
      is_at_stop_mqtt = true;
      return true;
    } else
      return false;
  }
  return false;
#endif
}

bool send_payload_signal_to_server(void) {
  for (int i = 1; i <= 3; i++) {
    is_published_mqtt = publish_mqtt_via_gsm();
    if (is_published_mqtt) {
      return true;
    }
  }
  if (!is_published_mqtt) {
    return false;
  }
  return false;
}

void sleep_stm32(void) {
  printf("begin sleep mode STM32");
  HAL_GPIO_WritePin(PWRKEY_SIMCOM_GPIO_Port, PWRKEY_SIMCOM_Pin, GPIO_PIN_SET);
  HAL_Delay(3000);
  HAL_GPIO_WritePin(PWRKEY_SIMCOM_GPIO_Port, PWRKEY_SIMCOM_Pin, GPIO_PIN_RESET);
  HAL_Delay(6000);
  HAL_GPIO_WritePin(GPIOB, LED_STATUS_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, ENABLE_SENSOR_Pin, GPIO_PIN_RESET);
  printf("--------GOOD BYE !-------");
  HAL_TIM_Base_Stop_IT(&htim6);
    HAL_ADC_Stop_DMA(&hadc1);
  HAL_TIM_Base_Start_IT(&htim3);
  HAL_SuspendTick();
  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFE);
  HAL_ResumeTick();
  NVIC_SystemReset();
}

bool update_data_to_sreen(uint8_t *data){
	  printf("update data to screen \r\n");
	  HAL_UART_Transmit(&huart5, data, strlen((const char *)data), 200);
	  return 1;
	return 0;
}

void check_handle_state(enum GmsModemState status) {
  switch (status) {
  case Off: {
    enable_simcom();
    IWDG->KR = 0xAAAA;
    is_pb_done = wait_for_pb_done_event();
    if (is_pb_done) {
      current_status_simcom = On;
      printf("Current status SIMCOM On \r\n");
    } else {
      NVIC_SystemReset();
    }
    break;
  }
  case On: {
    IWDG->KR = 0xAAAA;
    is_checked_sim = check_signal_simcom();
    if (is_checked_sim) {
      total_errors = 0;
      current_status_simcom = InternetReady;
      printf("SIMCOM current status is Internet ready\r\n");
    } else {
      total_errors++;
    }
    if (total_errors > 10) {
      restart_stm32();
    }
    break;
  }
  case InternetReady: {
    IWDG->KR = 0xAAAA;
    is_enable_mqtt = enable_mqtt_on_gsm_modem();
#if SIMCOM_MODEL != a7080
    if (is_enable_mqtt) {
      is_acquiered_mqtt = acquire_gsm_mqtt_client();
    } else {
      NVIC_SystemReset();
    }
    if (is_acquiered_mqtt) {
      is_connected_mqtt = connect_mqtt_server_by_gsm();
    }
#endif
    if (is_connected_mqtt) {
      total_errors = 0;
      current_status_simcom = MqttReady;
      printf("SIMCOM current status is MQTT ready\r\n");
    } else
      total_errors++;
    if (total_errors > 3) {
      restart_stm32();
    }
    break;
  }
  case MqttReady: {
	IWDG->KR = 0xAAAA;
    is_subcribed_mqtt = subscribe_mqtt_via_gsm();
    if (is_subcribed_mqtt) {
      total_errors = 0;
      current_status_simcom = Subscribed;
      printf("Current status Simcom subscribed \r\n");
    } else
      total_errors++;
    if (total_errors > 5) {
      restart_stm32();
    }
    break;
  }
  case Subscribed: {
#if INTERVAL_PUPLISH_DATA < 60
	  if (to_send_status_to_server) {
//      read_sensor();
		  	//process_uart_rx();
      IWDG->KR = 0xAAAA;
      is_updated_status = send_payload_signal_to_server();
//      is_publish_data_lcd = update_data_to_sreen((uint8_t *)array_json);
      is_updated_status= publish_mqtt_motor_status();

      if (is_updated_status) {
        to_send_status_to_server = 0;
        IWDG->KR = 0xAAAA;
        total_errors = 0;
      } else {
        total_errors++;
        if (total_errors > 5) {
          stop_mqtt_via_gsm();
          current_status_simcom = On;
        }
      }
  }

#else
      read_sensor();
      is_updated_status = send_payload_signal_to_server();
      if (is_updated_status) {
        to_send_status_to_server = 0;
        IWDG->KR = 0xAAAA;
        total_errors = 0;
    current_status_simcom = SleepStm32;
      } else {
        total_errors++;
        if (total_errors > 3) {
          stop_mqtt_via_gsm();
          current_status_simcom = On;
        }
      }

#endif
	  break;
  }

  case ControlModeOffline: {

	  break;
  }
#if INTERVAL_PUPLISH_DATA >= 60
  case SleepStm32: {
	  sleep_stm32();
	  break;
  }
#endif
  default:
    printf("Case cannot be determined !\r\n");
  }
}
