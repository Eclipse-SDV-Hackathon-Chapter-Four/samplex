/* SPDX-License-Identifier: MIT */
#ifndef POWERTRAIN_DISPLAY_H
#define POWERTRAIN_DISPLAY_H

#include <stdbool.h>
#include <stddef.h>

/* The caller holds mqtt_i2c_mutex while updating the display. */
void powertrain_display_waiting(void);

/* Display the top-level "Mode" string in a powertrain MQTT JSON payload.
 * Invalid messages leave the last displayed mode intact. */
bool powertrain_display_update(const char *payload, size_t payload_length);

#endif
