/*  ITS8050 - Embedded Software Workshop (2020 Spring)
    Contact Free Access Sensor on ESP32

    @see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html
    @see https://sensing.honeywell.com/hallbook.pdf

    Tasks:
    // A) measuring the internal Hall Effect Sensor output with an ADC on ESP32
    // Output should indicate:
    // - State of the access system ("Open" or "Closed")
    // - State of the press button ("Pressed" or "Not Pressed")

    // To open the access system, one "press" event is required (switch access from closed -> open)
    // To close the access system, two press events are required ("Pressed" -> "Not Pressed" -> "Pressed"; open -> closed)

    // When magnet no close, access system state should not change, button state = "Not Pressed"

    // sample averaging, debouncing, hysteresis

    // extra: provide sensor output with their units (hall effect = mT (Tesla), Temperature = C°)


    Authors: Group G (Marco Hanisch, Romain Thollot, Antoine Pinon)
*/

// libs
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include <driver/adc.h>


/* ESP32 board Sensors test
uint8_t temprature_sens_read();
uint32_t hall_sens_read();
printf("ESP32 onchip Temperature = %d\n", temprature_sens_read());
printf("ESP32 onchip Hall sensor = %d\n", hall_sens_read());
*/

// CONSTANT
#define PRECISION 5 /* defines the interval where the button will be considered to be pressed */
#define MAX_INACTIVITY_DELAY 5 /* delay of inactivity to close the system (in s) */

// Global Vars
int without_magnet = 100;
int with_magnet = 0;
bool access_system_state;
bool current_button_state;
bool previous_button_state;
bool previous_button_state_2;

// Signature
void calibrate(void);
void press_enter(void);
//void read_sens_task(void *pvParameters);
void read_sens_task(void);
void update_button_state(bool current_bs);
int get_average_hall(int nb_of_measures, int ms_delay);


void app_main(void){

    printf("[i] LOG: Entered the ESP32 'Thing' Monitoring\r\n");
    printf("[i] Try to read the sensors inside the ESP32 chip... \r\n");
    adc1_config_width(ADC_WIDTH_BIT_12);
    //TODO: add sensor reading test

    printf("[i] Beginning sensor calibration...\r\n");
    calibrate();
    printf("[+] Calibration sucessfull:\n");
    printf("\t> Standard Value    = %d;\n", without_magnet);
    printf("\t> Value with Magnet =%d;\n\n", with_magnet);

    printf("[i] Starting main programm...\n");
    read_sens_task();
    //xTaskCreatePinnedToCore(&read_sens_task,  /* Task function. */
    //                        "read_sens_task", /* String with name of task. */
    //                        10000,             /* Stack size in bytes. */
    //                        NULL,             /* Parameter passed as input of the task */
    //                        5,                /* Priority of the task. */
    //                        NULL,             /* Task handle. */
    //                        0);
    

}


void calibrate(void){
    printf("[i] Gathering data without magnet...\r\n");
    without_magnet = get_average_hall(1000, 5);
    printf(".\r\n[+] done!\r\n");

    printf("[i] Put the magnets near the chips for 5s, press 'Enter' when you are ready... \r\n");
    press_enter();

    printf("[i] Gathering data with magnet...\r\n");
    with_magnet = get_average_hall(1000, 5); // -10 for the purpose of the tests
    printf(".\r\n[+] Calibration done!\r\n");
}


//void read_sens_task(void *pvParameters){
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


        /* button is pressed, magnet is close to the hall sensor */
        if(current_hall <= (with_magnet + PRECISION)){
            update_button_state(true);
            timer_inactivity = 0;
            printf("++ Button state: Pressed\n");

            // system is closed
            if(!access_system_state){
                access_system_state = true;
                printf("++ System state: Open\n");
                printf("[i] A user openned the system.\n");
            }
            // system is open
            else if(previous_button_state_2 && !previous_button_state){
                access_system_state = false;
                printf("-- System state: Closed\n");
                printf("[i] A user closed the system.\n");
            } 
            else {
                printf("++ System state: Open\n");
            }
        /* button is not pressed, magnets is far from the hall sensor */
        } 
        else {
            update_button_state(false);
            printf("-- Button state: Not Pressed\n");

            /* button is not pressed, but system is open */
            if(access_system_state){
                // close the system after a certain amount of time (a number of loop, 1 round ~ 1s)
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
        // update_button_state(false); // forced button reset
    }
}// task end, should never reach


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

void update_button_state(bool current_bs){
    previous_button_state_2 = previous_button_state;
    previous_button_state = current_button_state;
    current_button_state = current_bs;
}

void press_enter(void){
    while( getchar() != '\n' ){
        // Task watchdog got triggered. The following tasks did not reset the watchdog in time:
        // task_wdt:  - IDLE0 (CPU 0)
        // esp_err_t esp_task_wdt_reset();
    }
}