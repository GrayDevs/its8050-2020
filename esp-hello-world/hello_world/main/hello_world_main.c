/* ITS8050 - Embedded Software Workshop (2020 Spring)
   ESP32 development SW installation

   Exercise 1: Hello World world with multi-task
   This code creates two FreeRTOS tasks to do the same thing, outputing two strings of "Hello" and "World" each from each tasks with separate delays

   Authors: Group G (Marco Hanisch, Romain Thollot, Antoine Pinon)
   Note that this implementation is greatly influenced by:
    - demo code from FreeRTOS
    - example code 'hello_world_main_c' from esp-idf
    - A tutorial on creating FreeRTOS Task on ESP32: https://techtutorialsx.com/2017/05/06/esp32-arduino-creating-a-task/
*/

/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>

/* FreeRTOS kernel includes. */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ESP32 includes. */
#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

void vTaskHello( void *pvParameters );
void vTaskWorld( void *pvParameters );

/*-----------------------------------------------------------*/
void app_main(void)
{

    printf("[+] LOG: Entered in the main loop\n");

    // Creating the two tasks with the same priority
    xTaskCreate(
                    vTaskHello,         /* Task function. */
                    "vTaskHello",       /* String with name of task. */
                    10000,              /* Stack size in bytes. */
                    NULL,               /* Parameter passed as input of the task */
                    1,                  /* Priority of the task. */
                    NULL);              /* Task handle. */

    xTaskCreate(
                    vTaskWorld,         /* Task function. */
                    "vTaskWorld",       /* String with name of task. */
                    10000,              /* Stack size in bytes. */
                    NULL,               /* Parameter passed as input of the task */
                    1,                  /* Priority of the task. */
                    NULL);              /* Task handle. */


    /* Start the RTOS scheduler, this function should not return as it causes the
    execution context to change from main() to one of the created tasks. */
    // vTaskStartScheduler();  
    
    /* Intersetingly enough it give this error:
    Guru Meditation Error: Core  0 panic'ed (InstrFetchProhibited). Exception was unhandled.
    The crash occurs while attempting to do a task context switch which is apparently caused by a stack size issue.
    As we do not need the task scheduler for this example we will just continue without it.
    */

    for (int i = 0; i >= 0; i--) {
        vTaskDelay(20000 / portTICK_PERIOD_MS);
        printf("Restarting now.\n");
        fflush(stdout);
        esp_restart();        
    }
}


void vTaskHello( void *pvParameters )
{
    for(int i = 0; i < 4; i++)
    {
        //-- Task application code here. --
        printf("Hello ");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    /* Tasks must not attempt to return from their implementing
    function or otherwise exit. If it is necessary for a task to
    exit then have the task call vTaskDelete( NULL ) to ensure
    its exit is clean. */
    vTaskDelete( NULL );
}


void vTaskWorld( void *pvParameters )
{
    for(int i = 0; i < 4; i++)
    {
        //-- Task application code here. --
        printf("World\n");
        vTaskDelay(1500 / portTICK_PERIOD_MS);
    }

    /* Tasks must not attempt to return from their implementing
    function or otherwise exit. If it is necessary for a task to
    exit then have the task call vTaskDelete( NULL ) to ensure
    its exit is clean. */
    vTaskDelete( NULL );
}