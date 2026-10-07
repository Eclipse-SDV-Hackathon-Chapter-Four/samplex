/* 
 *  Copyright (c) 2025 Eclipse Foundation
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
#include "cloud_config.h"
#include "mqtt_client.h"
#include "nx_api.h"
#include "powertrain_display.h"
#include "telemetry.h"
#include "wwd_networking.h"

#include <stdint.h>
#include <string.h>

// Helper function.
#define STRLEN(p) (sizeof(p) - 1)

/* Declare the MQTT thread stack space. */
static ULONG mqtt_client_stack[MQTT_CLIENT_STACK_SIZE / sizeof(ULONG)];

/* Declare buffers to hold message and topic. */
static UCHAR message_buffer[NXD_MQTT_MAX_MESSAGE_LENGTH + 1];
static UCHAR topic_buffer[NXD_MQTT_MAX_TOPIC_NAME_LENGTH + 1];


/* Declare the MQTT client control block. */
static NXD_MQTT_CLIENT mqtt_client;


/* Declare the disconnect notify function. */
static VOID client_disconnect_func(NXD_MQTT_CLIENT *client_ptr)
{
    NX_PARAMETER_NOT_USED(client_ptr);
    printf("client disconnected from broker.\r\n");
}

static void send_message(){
    UINT status;
    
    /* Publish a message with QoS Level 1. */
    char buffer[TELEMETRY_BUFFER_SIZE] = {0};
    get_current_telemetry_string(buffer);
    //printf("%s", buffer);

    status = nxd_mqtt_client_publish(&mqtt_client, MQTT_PUBLISH_TOPIC, STRLEN(MQTT_PUBLISH_TOPIC),
                                        (CHAR *)buffer, strlen(buffer), 0, QOS1, NX_WAIT_FOREVER);

    if (status != NXD_MQTT_SUCCESS){
        printf("Publish failed with code: %d\r\n", status);
    }
    else{
        printf("Published message.\r\n");
    }
}

static void receive_message(){
    UINT status;
    UINT topic_length, message_length;

    /* Notifications may cover several messages; drain them all. Leave room
     * for terminators when receiving a maximum-length topic or payload. */
    while (1){
        status = nxd_mqtt_client_message_get(&mqtt_client, topic_buffer, sizeof(topic_buffer) - 1, &topic_length,
                                            message_buffer, sizeof(message_buffer) - 1, &message_length);
        if (status == NXD_MQTT_NO_MESSAGE){
            return;
        }
        if (status != NXD_MQTT_SUCCESS){
            printf("MQTT receive failed with code: %d\r\n", status);
            return;
        }
        topic_buffer[topic_length] = '\0';
        message_buffer[message_length] = '\0';
        printf("Topic: %s, Message: %s\r\n", topic_buffer, message_buffer);

        if (topic_length == STRLEN(MQTT_SUBSCRIBE_TOPIC) &&
            memcmp(topic_buffer, MQTT_SUBSCRIBE_TOPIC, topic_length) == 0){
            tx_mutex_get(&mqtt_i2c_mutex, TX_WAIT_FOREVER);
            bool displayed = powertrain_display_update((const char *)message_buffer, message_length);
            tx_mutex_put(&mqtt_i2c_mutex);
            if (!displayed){
                printf("Ignoring powertrain message without a valid Mode string\r\n");
            }
        }
    }
}

static VOID client_notify_func(NXD_MQTT_CLIENT *client_ptr, UINT number_of_messages)
{
    NX_PARAMETER_NOT_USED(client_ptr);
    NX_PARAMETER_NOT_USED(number_of_messages);
    tx_event_flags_set(&mqtt_app_flag, MQTT_RECEIVE_EVENT, TX_OR);
    return;
}

static void mqtt_thread_work(NX_IP *ip_ptr, NX_PACKET_POOL *pool_ptr){
    UINT status;
    NXD_ADDRESS server_ip;
    ULONG events;

    printf("Creating MQTT client\r\n");
    /* Create MQTT client instance. */
    status = nxd_mqtt_client_create(&mqtt_client, MQTT_CLIENT_NAME, MQTT_CLIENT_NAME, STRLEN(MQTT_CLIENT_NAME),
                                    ip_ptr, pool_ptr, (VOID *)mqtt_client_stack, sizeof(mqtt_client_stack),
                                    MQTT_THREAD_PRIORTY, NX_NULL, 0);

    if (status){
        printf("Error in creating MQTT client: 0x%02x\n", status);
        return;
    }

    printf(" MQTT client created\r\n");

    /* Register the disconnect notification function. */
    nxd_mqtt_client_disconnect_notify_set(&mqtt_client, client_disconnect_func);

    /* Install the callback before subscribing: retained messages can arrive
     * as soon as the broker receives the subscription. */
    status = nxd_mqtt_client_receive_notify_set(&mqtt_client, client_notify_func);
    if (status != NXD_MQTT_SUCCESS){
        printf("MQTT receive notify setup failed with code: %d\r\n", status);
        return;
    }

    server_ip.nxd_ip_version = 4;
    server_ip.nxd_ip_address.v4 = MQTT_LOCAL_BROKER_IP;

    /* Connect over plain MQTT without setting a username or password. */

    status = nxd_mqtt_client_connect(&mqtt_client, &server_ip, MQTT_BROKER_PORT,
                                     MQTT_KEEP_ALIVE_TIMER, 0, NX_WAIT_FOREVER);
    if (status != NXD_MQTT_SUCCESS){
                printf("MQTT connect failed with code: %d\r\n", status);
                return;
    }
    else{
        printf("MQTT Client connected.\r\n");
    }

    /* Subscribe to the topic with QoS level 0. */
    status = nxd_mqtt_client_subscribe(&mqtt_client, MQTT_SUBSCRIBE_TOPIC, STRLEN(MQTT_SUBSCRIBE_TOPIC), QOS0);
    if (status != NXD_MQTT_SUCCESS){
                printf("MQTT subscribe failed with code: %d\r\n", status);
                return;
    }
    else{
        printf("Subscribed to topic %s.\r\n", MQTT_SUBSCRIBE_TOPIC);
    }

    /* Now wait for the broker to publish the message. */
    printf("Waiting for messages\r\n");

    while (1){
        tx_event_flags_get(&mqtt_app_flag, MQTT_ALL_EVENTS, TX_OR_CLEAR, &events, TX_WAIT_FOREVER);
        if (events & MQTT_RECEIVE_EVENT){
            receive_message();

        }
        if (events & MQTT_MESSAGE_READY){
            send_message();
        }
    }

    /* Cleanup. Release resources. */
    nxd_mqtt_client_unsubscribe(&mqtt_client, MQTT_SUBSCRIBE_TOPIC, STRLEN(MQTT_SUBSCRIBE_TOPIC));
    nxd_mqtt_client_disconnect(&mqtt_client);
    nxd_mqtt_client_delete(&mqtt_client);

    return;
}

void mqtt_thread_entry(ULONG parameter){

    UINT status;

    printf("Starting Eclipse ThreadX MQTT thread\r\n\r\n");

    tx_mutex_get(&mqtt_i2c_mutex, TX_WAIT_FOREVER);
    powertrain_display_waiting();
    tx_mutex_put(&mqtt_i2c_mutex);

    // Initialize the network
    if ((status = wwd_network_init(WIFI_SSID, WIFI_PASSWORD, WIFI_MODE))){
        printf("ERROR: Failed to initialize the network (0x%08x)\r\n", status);
    }

    wwd_network_connect();

    mqtt_thread_work(&nx_ip, nx_pool);
}
