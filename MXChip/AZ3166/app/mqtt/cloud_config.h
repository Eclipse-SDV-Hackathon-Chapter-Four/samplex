/* 
 * Copyright (c) Microsoft
 * Copyright (c) 2024 Eclipse Foundation
 * 
 *  This program and the accompanying materials are made available 
 *  under the terms of the MIT license which is available at
 *  https://opensource.org/license/mit.
 * 
 *  SPDX-License-Identifier: MIT
 * 
 *  Contributors: 
 *     Microsoft         - Initial version
 *     Frédéric Desbiens - 2024 version.
 */

#ifndef _CLOUD_CONFIG_H
#define _CLOUD_CONFIG_H

#include "nx_api.h"

typedef enum
{
    None         = 0,
    WEP          = 1,
    WPA_PSK_TKIP = 2,
    WPA2_PSK_AES = 3
} WiFi_Mode;

// ----------------------------------------------------------------------------
// WiFi connection config
// ----------------------------------------------------------------------------
#define HOSTNAME      "eclipse-threadx"  //Change to unique hostname.
#define WIFI_SSID     "Hackathon-Team-03"
#define WIFI_PASSWORD "SDVTeam-123456" 
#define WIFI_MODE     WPA2_PSK_AES

// ----------------------------------------------------------------------------
// MQTT Config
// ----------------------------------------------------------------------------
#define MQTT_CLIENT_NAME     "ThreadXAZ3166" //Change to unique name.
// imx95-a.local: use its IPv4 address directly; no username or password required.
#define MQTT_LOCAL_BROKER_IP (IP_ADDRESS(192, 168, 103, 191))
#define MQTT_BROKER_PORT      1883
#define MQTT_SUBSCRIBE_TOPIC "vehicle/powertrain/mode"
#define MQTT_PUBLISH_TOPIC   MQTT_CLIENT_NAME "/telemetry" 

// ----------------------------------------------------------------------------
// MQTT Support infrastructure
// ----------------------------------------------------------------------------
// The OLED and telemetry sensors share the same I2C bus.
extern TX_MUTEX mqtt_i2c_mutex;
extern TX_EVENT_FLAGS_GROUP mqtt_app_flag;
#define MQTT_RECEIVE_EVENT 1
#define MQTT_MESSAGE_READY 2
#define MQTT_ALL_EVENTS    3


#endif // _CLOUD_CONFIG_H
