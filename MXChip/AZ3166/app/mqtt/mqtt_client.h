/* 
 *  Copyright (c) 2025 Eclipse Foundation
 * Copyright (c) 2026 Eclipse ThreadX contributors
 * 
 *  This program and the accompanying materials are made available 
 *  under the terms of the MIT license which is available at
 *  https://opensource.org/license/mit.
 * 
 *  SPDX-License-Identifier: MIT
 * 
 *  Contributors: 
 *     Frédéric Desbiens - Initial version.
 */
// Portions of this file were generated with AI assistance.
#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "tx_api.h"

#define MQTT_CLIENT_STACK_SIZE 5120
#define MQTT_THREAD_PRIORTY 2
#define MQTT_KEEP_ALIVE_TIMER 300
void mqtt_thread_entry(ULONG thread_input);

#endif
