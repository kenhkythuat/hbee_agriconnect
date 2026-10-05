/*
 * sensor.c
 *
 *  Created on: Mar 3, 2025
 *      Author: thuanphat
 */
/* USER CODE BEGIN Includes */
#include "config.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <math.h>
#include <main.h>
#include <stdbool.h>
/* USER CODE END Includes */
/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart4;
/* USER CODE END EM */
// data PH Fuvitech
float data_ph_fuvitech;
float data_measured_ph_fuvitech=0;
float data_temperature_ph_fuvitech;
// data EC Fuvitech
float data_common_sensor_fuvitech;
float data_conductivity_ec_fuvitech;
float data_resistivity_ec_fuvitech;
float data_temperateure_ec_fuvitech;
float data_tds_ec_fuvitech;
float data_salinity_ec_fuvitech;

// data DO Fuvitech
float data_do_fuvitech;
float data_dissolved_oxygen_fuvitech;
float data_temperature_do_fuvitech;
uint8_t is_init_setup_do=0;

// data PH Rika500_12
float data_ph_rika500_12;
float data_measured_rika500_12;
float data_temperature_rika500_12;


// data EC Fuvitech
float data_common_sensor_fuvitech;
float data_conductivity_ec_rika500_13;
float data_resistivity_ec_rika500_13;
float data_temperateure_ec_rika500_13;
float data_tds_ec_rika500_13;
float data_salinity_ec_rika500_13;

float data_do_saturation;
float data_dissolved_oxygen_rika;
float data_temperature_do_rika;

bool sensor_ph_valid = false;
bool sensor_ph_temperature_valid = false;
bool sensor_ec_conductivity_valid = false;
bool sensor_ec_resistivity_valid = false;
bool sensor_ec_temperature_valid = false;
bool sensor_ec_tds_valid = false;
bool sensor_ec_salinity_valid = false;
bool sensor_do_valid = false;
volatile uint16_t sensor_rx_length = 0U;
volatile bool sensor_rx_received = false;

static bool last_sensor_read_valid = false;

static uint16_t modbus_crc16(const uint8_t *data, uint16_t length) {
  uint16_t crc = 0xFFFFU;

  for (uint16_t i = 0U; i < length; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0U; bit < 8U; bit++) {
      if ((crc & 1U) != 0U) {
        crc = (uint16_t)((crc >> 1U) ^ 0xA001U);
      } else {
        crc >>= 1U;
      }
    }
  }
  return crc;
}

static bool modbus_response_valid(const uint8_t *request) {
  const uint8_t *response = (const uint8_t *)rx_buffer_fuvitech;
  uint16_t length = sensor_rx_length;
  uint16_t expected_length;
  uint16_t crc;

  if (!sensor_rx_received || request == NULL || length < 5U ||
      response[0] != request[0] || response[1] != request[1]) {
    return false;
  }

  expected_length = (uint16_t)response[2] + 5U;
  if (length != expected_length || expected_length > sizeof(rx_buffer_fuvitech)) {
    return false;
  }

  crc = modbus_crc16(response, (uint16_t)(length - 2U));
  return response[length - 2U] == (uint8_t)(crc & 0xFFU) &&
         response[length - 1U] == (uint8_t)(crc >> 8U);
}


float ieee754_to_float(unsigned char *bytes) {
  uint32_t int_representation = 0;
  // Chuyển mảng byte thành số nguyên 32-bit
  int_representation = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
  // Ép kiểu về float
  float result;
  memcpy(&result, &int_representation, sizeof(result));
  return result;
}

float bytes_to_float(unsigned char *byte_array) {
  unsigned int temp = 0;

  // Chuyển dãy byte thành số 32-bit (Big Endian)
  temp |= byte_array[0] << 24;
  temp |= byte_array[1] << 16;
  temp |= byte_array[2] << 8;
  temp |= byte_array[3]; // byte cuối cùng (Big Endian)

  // Sử dụng union để ép kiểu 32-bit sang float
  union {
    unsigned int i;
    float f;
  } data;

  data.i = temp; // Gán giá trị 32-bit cho i
  return data.f; // Trả về giá trị float
}

#if ph_fuvitech
unsigned char measured_command_ph_fuvitech[8] = {0x02, 0x03, 0x00, 0x01, 0x00, 0x02, 0x95, 0xF8};
unsigned char temperature_command_ph_fuvitech[8] = {0x02, 0x03, 0x00, 0x03, 0x00, 0x02, 0x34, 0x38};
#endif
#if ec_fuvitech
unsigned char conductivity_command_ec_fuvitech[8] = {0x03, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC5, 0xE9};
unsigned char resistivity_command_ec_fuvitech[8] = {0x03, 0x03, 0x00, 0x02, 0x00, 0x02, 0x64, 0x29};
unsigned char temperateure_command_ec_fuvitech[8] = {0x03, 0x03, 0x00, 0x04, 0x00, 0x02, 0x84, 0x28};
unsigned char tds_command_ec_fuvitech[8] = {0x03, 0x03, 0x00, 0x06, 0x00, 0x02, 0x25, 0xE8};
unsigned char salinity_command_ec_fuvitech[8] = {0x03, 0x03, 0x00, 0x08, 0x00, 0x02, 0x44, 0x2B};
#endif
#if do_fuvitech
unsigned char _command_do_fuvitech[8] = {0x01, 0x03, 0x00, 0x10, 0x00, 0x08, 0x45, 0xC9};
unsigned char _command_setup_do_fuvitech[13] = {0x01, 0x10, 0x00, 0x00, 0x00, 0x02, 0x04, 0x00, 0x05, 0x00, 0x1E, 0x63, 0xA6};
#endif
#if ph_rika500_12
unsigned char measured_command_ph_rika500_12[8] = {0x05, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC5, 0x8F};
unsigned char temperature_command_ph_rika500_12[8] = {0x05, 0x03, 0x00, 0x04, 0x00, 0x02, 0x84, 0x4E};
#endif
#if ec_rika500_13
unsigned char conductivity_command_ec_rika500_13[8] = {0x04, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x5E};
unsigned char resistivity_command_ec_rika500_13[8] = {0x04, 0x03, 0x00, 0x02, 0x00, 0x02, 0x65, 0x9E};
unsigned char temperateure_command_ec_rika500_13[8] = {0x04, 0x03, 0x00, 0x04, 0x00, 0x02, 0x85, 0x9F};
unsigned char tds_command_ec_rika500_13[8] = {0x04, 0x03, 0x00, 0x06, 0x00, 0x02, 0x24, 0x5F};
unsigned char salinity_command_ec_rika500_13[8] = {0x04, 0x03, 0x00, 0x08, 0x00, 0x02, 0x45, 0x9C};
#endif

#if do_rika500_04
unsigned char measured_command_do_rika500_04[8] = {0x0A, 0x03, 0x00, 0x00, 0x00, 0x06, 0xC4, 0xB3};
unsigned char temperateure_command_do_rika500_04[8] = {0x0A, 0x03, 0x00, 0x00, 0x00, 0x06, 0xC4, 0xB3};
#endif

float read_ph_fuvitech(uint8_t *data, char * data_log) {
  printf("Read measured form %s\r\n",data_log);
  HAL_UART_Transmit(&huart4, data, 8, 1000);
  HAL_Delay(700);
  uint8_t reordered_data[4] = {rx_buffer_fuvitech[5], rx_buffer_fuvitech[6], rx_buffer_fuvitech[3], rx_buffer_fuvitech[4]};
  data_ph_fuvitech = ieee754_to_float(reordered_data);
  return data_ph_fuvitech;
}

float read_ph_rika500_12(uint8_t *data, char * data_log) {
  printf("Read measured form %s\r\n",data_log);
  HAL_UART_Transmit(&huart4, data, 8, 1000);
  HAL_Delay(700);
  uint8_t reordered_data[4] = {rx_buffer_fuvitech[5], rx_buffer_fuvitech[6], rx_buffer_fuvitech[3], rx_buffer_fuvitech[4]};
  data_ph_fuvitech = ieee754_to_float(reordered_data);
  return data_ph_fuvitech;
}

float read_sensor_fuvitech(uint8_t *data, char * data_log) {
  printf("Read sensor %s\r\n",data_log);
  data_common_sensor_fuvitech=0;
  last_sensor_read_valid = false;
  sensor_rx_received = false;
  sensor_rx_length = 0U;
  memset(rx_buffer_fuvitech, '\0', 100);
  HAL_Delay(100);
  HAL_UART_Transmit(&huart2, data, 8, 500);
  HAL_Delay(500);
  bool frame_valid = modbus_response_valid(data);
#if ph_fuvitech
  if(frame_valid&&rx_buffer_fuvitech[0]==2&&rx_buffer_fuvitech[1]==3&&rx_buffer_fuvitech[2]==4){
	  uint8_t reordered_data[4] = {rx_buffer_fuvitech[5], rx_buffer_fuvitech[6], rx_buffer_fuvitech[3], rx_buffer_fuvitech[4]};
	  data_common_sensor_fuvitech = (ieee754_to_float(reordered_data));
  }
#endif
#if ph_rika500_12
  if(frame_valid&&rx_buffer_fuvitech[0]==5&&rx_buffer_fuvitech[1]==3&&rx_buffer_fuvitech[2]==4){
	  uint8_t reordered_data[4] = {rx_buffer_fuvitech[3], rx_buffer_fuvitech[4], rx_buffer_fuvitech[5], rx_buffer_fuvitech[6]};
	  data_common_sensor_fuvitech = (ieee754_to_float(reordered_data));
  }
#endif
#if ec_fuvitech
  if(frame_valid&&rx_buffer_fuvitech[0]==3&&rx_buffer_fuvitech[1]==3&&rx_buffer_fuvitech[2]==4){
	  uint8_t reordered_data[4] = {rx_buffer_fuvitech[5], rx_buffer_fuvitech[6], rx_buffer_fuvitech[3], rx_buffer_fuvitech[4]};
	  data_common_sensor_fuvitech = ieee754_to_float(reordered_data);
  }
#endif
#if ec_rika500_13
  if(frame_valid&&rx_buffer_fuvitech[0]==4&&rx_buffer_fuvitech[1]==3&&rx_buffer_fuvitech[2]==4){
	  uint8_t reordered_data[4] = {rx_buffer_fuvitech[3], rx_buffer_fuvitech[4], rx_buffer_fuvitech[5], rx_buffer_fuvitech[6]};
	  data_common_sensor_fuvitech = (ieee754_to_float(reordered_data));
  }
#endif
#if do_fuvitech
  if(frame_valid&&rx_buffer_fuvitech[0]==1&&rx_buffer_fuvitech[1]==3){
	  printf("Parse rx data DO\r\n");
	  data_common_sensor_fuvitech = (rx_buffer_fuvitech[11]<<8| rx_buffer_fuvitech[12]);
  }
#endif
#if do_rika500_04
  if(frame_valid&&rx_buffer_fuvitech[0]==10&&rx_buffer_fuvitech[1]==3&&rx_buffer_fuvitech[2]==12){
	  uint8_t reordered_data[4] = {rx_buffer_fuvitech[3], rx_buffer_fuvitech[4], rx_buffer_fuvitech[5], rx_buffer_fuvitech[6]};
	  uint8_t saturation_data[4] = {rx_buffer_fuvitech[7], rx_buffer_fuvitech[8], rx_buffer_fuvitech[9], rx_buffer_fuvitech[10]};
	  data_do_saturation= (ieee754_to_float(saturation_data));
	  data_common_sensor_fuvitech = (ieee754_to_float(reordered_data));
  }
#endif

  last_sensor_read_valid = frame_valid && isfinite(data_common_sensor_fuvitech);
  if (!last_sensor_read_valid) {
    printf("Sensor %s: no valid Modbus response\r\n", data_log);
  }

  return data_common_sensor_fuvitech;
}

float read_do_fuvitech(uint8_t *data) {
  printf("Read measured form sensor EC\r\n");
  HAL_UART_Transmit(&huart2, data, 8, 1000);
  HAL_Delay(700);
  data_do_fuvitech = (rx_buffer_fuvitech[43]<<8| rx_buffer_fuvitech[44]);
  memset(rx_buffer_fuvitech, '\0', 100);
  return data_do_fuvitech;
}

static void update_sensor_value(uint8_t *command, const char *label,
                                float scale, float *destination,
                                bool *valid) {
  float value;

  if (*valid) {
    return;
  }
  value = read_sensor_fuvitech(command, (char *)label);
  if (last_sensor_read_valid) {
    *destination = value * scale;
    *valid = true;
  }
}

void read_sensor(void) {
  sensor_ph_valid = false;
  sensor_ph_temperature_valid = false;
  sensor_ec_conductivity_valid = false;
  sensor_ec_resistivity_valid = false;
  sensor_ec_temperature_valid = false;
  sensor_ec_tds_valid = false;
  sensor_ec_salinity_valid = false;
  sensor_do_valid = false;

  /* Two attempts are retained, but successful fields are not read twice. */
  for (uint8_t attempt = 0U; attempt < 2U; attempt++) {
#if ph_fuvitech
    update_sensor_value(measured_command_ph_fuvitech, "PH", 1.0f,
                        &data_measured_ph_fuvitech, &sensor_ph_valid);
    update_sensor_value(temperature_command_ph_fuvitech, "PH temperature",
                        1.0f, &data_temperature_ph_fuvitech,
                        &sensor_ph_temperature_valid);
#endif
#if ph_rika500_12
    update_sensor_value(measured_command_ph_rika500_12, "PH", 1.0f,
                        &data_measured_ph_fuvitech, &sensor_ph_valid);
    update_sensor_value(temperature_command_ph_rika500_12, "PH temperature",
                        1.0f, &data_temperature_ph_fuvitech,
                        &sensor_ph_temperature_valid);
#endif
#if ec_fuvitech
    update_sensor_value(conductivity_command_ec_fuvitech, "EC conductivity",
                        1000.0f, &data_conductivity_ec_fuvitech,
                        &sensor_ec_conductivity_valid);
    update_sensor_value(resistivity_command_ec_fuvitech, "EC resistivity",
                        1.0f, &data_resistivity_ec_fuvitech,
                        &sensor_ec_resistivity_valid);
    update_sensor_value(temperateure_command_ec_fuvitech, "EC temperature",
                        1.0f, &data_temperateure_ec_fuvitech,
                        &sensor_ec_temperature_valid);
    update_sensor_value(tds_command_ec_fuvitech, "EC TDS", 1.0f,
                        &data_tds_ec_fuvitech, &sensor_ec_tds_valid);
    update_sensor_value(tds_command_ec_fuvitech, "EC salinity",
                        (1.0f / 640.0f), &data_salinity_ec_fuvitech,
                        &sensor_ec_salinity_valid);
#endif
#if ec_rika500_13
    update_sensor_value(conductivity_command_ec_rika500_13,
                        "EC conductivity", 1000.0f,
                        &data_conductivity_ec_fuvitech,
                        &sensor_ec_conductivity_valid);
    update_sensor_value(resistivity_command_ec_rika500_13, "EC resistivity",
                        1.0f, &data_resistivity_ec_fuvitech,
                        &sensor_ec_resistivity_valid);
    update_sensor_value(temperateure_command_ec_rika500_13, "EC temperature",
                        1.0f, &data_temperateure_ec_fuvitech,
                        &sensor_ec_temperature_valid);
    update_sensor_value(salinity_command_ec_rika500_13, "EC salinity",
                        (1.0f / 640.0f), &data_salinity_ec_fuvitech,
                        &sensor_ec_salinity_valid);
#endif
#if do_fuvitech
    if (!is_init_setup_do) {
      (void)read_sensor_fuvitech(_command_setup_do_fuvitech, "DO setup");
      is_init_setup_do = 1U;
      HAL_Delay(500);
    }
    update_sensor_value(_command_do_fuvitech, "DO", (1.0f / 100.0f),
                        &data_dissolved_oxygen_fuvitech, &sensor_do_valid);
#endif
#if do_rika500_04
    update_sensor_value(measured_command_do_rika500_04, "DO Rika", 1.0f,
                        &data_dissolved_oxygen_rika, &sensor_do_valid);
#endif
  }
}
