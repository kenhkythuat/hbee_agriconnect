/*
 * config.h
 *
 *  Created on: AUG 1, 2024
 *      Author: thuanphat7
 */
#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_

#include "stdio.h"
#include <string.h>
#include <main.h>

// Codename of the farm, where we deploy this node to.

#define VERSION_WBEE "2.1"

#define a7672s 1  // 4G
#define a7670c 2  // 4G
#define a7670sa 3 // 4G
#define a7080 4  // NB-IOT
#define a7680 5  // 4G

#define SIMCOM_MODEL a7680 // #default is a7670 if you use model other please choose enter your model
#define SAVE_LOAD false
#define INTERVAL_PUPLISH_DATA 10 // the time the device sends data to the server, If the sending time is over 60 seconds, the sensor will go into deep sleep.

// Serial number. Must be lower case.
#ifndef SERIAL_NUMBER
  #define SERIAL_NUMBER "hb000999"
#endif

#define true 1
#define false 0

#define ph_fuvitech false
#define ec_fuvitech false
#define do_fuvitech false

#define ph_rika500_12 true
#define ec_rika500_13 false
#define do_rika500_04 false

#define duty_cycles_ph 50
#define duty_cycles_ec 50
#define duty_cycles_x 50



//#define FARM "demox"
//#define MQTT_USER "node" 		// User - connect to MQTT broker
//#define MQTT_PASS "654321"		// Password - connect to MQTT broker

#define FARM "gateway-agriconnect"
#define MQTT_USER "mqttnode"       // User - connect to MQTT broker
#define MQTT_PASS "congamo"		// Password - connect to MQTT broker

#define MQTT_TOPIC_ACTUATOR_STATUS FARM "/sn/" SERIAL_NUMBER
#define MQTT_TOPIC_MOTOR_STATUS FARM "/sn/" SERIAL_NUMBER "/as/"
// MQTT topic to subscribe and get command to switch on/off actuator
#define MQTT_TOPIC_ACTUATOR_CONTROL FARM "/snac/" SERIAL_NUMBER "/"
/** MQTT
 * Global broker: mqtt.agriconnect.vn
 */
#define MQTT_HOST "tcp://mqtt.agriconnect.vn"           		// MQTT broker

#define MQTT_CLIENT_ID  SERIAL_NUMBER
#define MQTT_PORT 1883

#define TIME_PERIOD ((2000000*INTERVAL_PUPLISH_DATA)/60000)-1


#endif /* INC_CONFIG_H_ */
