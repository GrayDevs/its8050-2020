/*  INTEGRATION

    This module provide integration of the two previous project:
    - esp-mqtt-echo
    - esp-contacless-access

    Authors: Group G (Marco Hanisch, Romain Thollot, Antoine Pinon)
    Last Reviewed: 30/04/2020

    TODO: Maybe use header instead of having all the functions in the same place

    References:
    @see MQTT with ESP-IDF framework: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mqtt.html

    Part of this code is in the Public Domain (or CC0 licensed, at your option.)
    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/


// Libs
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
// #include "tcpip_adapter.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"


// CONSTANT
#define PRECISION 5 /* defines the interval where the button will be considered to be pressed */
#define MAX_INACTIVITY_DELAY 5 /* delay of inactivity to close the system (in s) */
static const char *TAG = "MQTT_EXAMPLE";

// Global Vars
int without_magnet = 100;
int with_magnet = 0;
bool access_system_state;
bool current_button_state;
bool previous_button_state;
bool previous_button_state_2;


// Signature
void calibrate(void);
void read_sens_task(void);
void update_button_state(bool current_bs);
int get_average_hall(int nb_of_measures, int ms_delay);

void connecting_to_wifi(void);
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event);
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void mqtt_app_start();



/************************
 *  Contactless Sensor  *
 ************************/


/**
 * Keeps track of the button state by updating n-2, n-1 and n values
 * @param: current_bs <bool> - the current button state based on the hall value
 */
void update_button_state(bool current_bs){
    previous_button_state_2 = previous_button_state;
    previous_button_state = current_button_state;
    current_button_state = current_bs;
}

/** 
 * Manage everything about contactless sensor 
 */
void read_sens_task(void){
    int current_hall = without_magnet;
    int timer_inactivity = 0;
    access_system_state = false;
    current_button_state = false;
    previous_button_state = false;
    previous_button_state_2 = false;

    while(true){
        current_hall = get_average_hall(1000, 8);
        printf("ESP32 Hall sensor average = %d mT\n", current_hall);

        if(current_hall <= (with_magnet + PRECISION)){
            update_button_state(true);
            timer_inactivity = 0;
            printf("++ Button state: Pressed\n");
            if(!access_system_state){
                access_system_state = true;
                printf("++ System state: Open\n");
                printf("[i] A user openned the system.\n");
            }
            else if(previous_button_state_2 && !previous_button_state){
                access_system_state = false;
                printf("-- System state: Closed\n");
                printf("[i] A user closed the system.\n");
            } 
            else {
                printf("++ System state: Open\n");
            }
        } 
        else {
            update_button_state(false);
            printf("-- Button state: Not Pressed\n");
            if(access_system_state){
                if(timer_inactivity == MAX_INACTIVITY_DELAY){
                    access_system_state = false;
                    timer_inactivity = 0;
                    printf("-- System state: Closed\n");
                    printf("[!] System automatically closed due to inactivity\n");
                } 
                else {
                    printf("++ System state: Open\n");
                    timer_inactivity++;
                }
            }
            else {
                printf("-- System state: Closed\n");
            }
        }
    }
}


/**
 * This function role is to leverage noise impact and get more stable values
 * @param: nb_of_measures, how many measures to take, = 100
 * @param: delay between each measure (in ms)         = 10 (up to 100reads/s)
 * @return: average hall effect value                 
*/
int get_average_hall(int nb_of_measures, int delay){
    int sum=0;
    for(int i=0; i < nb_of_measures; i++){
        sum += hall_sensor_read();
        vTaskDelay(delay / portTICK_PERIOD_MS);
    }
    return sum / nb_of_measures;
}

/**
 * This function manage the calibration of the magnet
 * 1. It first get an average hall value in normal environment
 * 2. Then get the value the perturbation caused by a with magnet
 * Note: This last value then is used as a treshold for controlling the access system.
 */
void calibrate(void){
    printf("[i] Gathering data without magnet...\r\n");
    without_magnet = get_average_hall(1000, 5);
    printf(".\r\n[+] done!\r\n");

    printf("[i] Put the magnets near the chips and wait calibration is over\r\n");
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    printf("[i] Gathering data with magnet...\r\n");
    with_magnet = get_average_hall(1000, 5); // -10 for the purpose of the tests
    printf(".\r\n[+] Calibration done!\r\n");
}


/************************
 *         MQTT         *
 ************************/

/**
 * This function manage the different MQTT event
 * Most of it's code come from esp-idf/examples/protocols/mqtt/tcp/main/mqtt_tcp
 * 
 * Modifications have been made to make the ESP32 subscribe to a specific topic right after connecting
 * Then publishing data on this topic 
 */
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            // On connect, client subscribe to /iot/access-system/status
            msg_id = esp_mqtt_client_subscribe(client, "/iot/access-system/status", 2);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA: // "equivalent" of on_message for python paho
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);

			// button and system state toString()
			char* message = "-- Button: Not Pressed | -- System: Closed";
			if(current_button_state && access_system_state) {
				message = "++ Button: Pressed | ++ System: Open";
			}
			else if(current_button_state && !access_system_state) {
				message = "++ Button: Pressed | -- System: Closed";
			}
            else if(!current_button_state && access_system_state) {
				message = "-- Button: Not Pressed | ++ System: Open";
			}

            // publishing data to the topic /iot/access-system/status
            msg_id = esp_mqtt_client_publish(client, "/iot/access-system/status", message, 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}


void mqtt_app_start()
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,  /* @see Kconfig.projbuild file */
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}


/************************
 *         WIFI         *
 ************************/

void connecting_to_wifi(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    // tcpip_adapter_init();  // deprecated
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    ESP_LOGI(TAG, "[+] Connection to Wifi established");

}


/************************
 *         MAIN         *
 ************************/

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);    
    
    connecting_to_wifi();
    mqtt_app_start();

    printf("[i] Beginning sensor calibration...\r\n");
    calibrate();
    printf("[+] Calibration sucessfull:\n");
    printf("\t> Standard Value    = %d;\n", without_magnet);
    printf("\t> Value with Magnet =%d;\n\n", with_magnet);

    printf("[i] Starting main programm...\n");
    read_sens_task();
}