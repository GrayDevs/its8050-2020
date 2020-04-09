/* ITS8050 - Embedded Software Workshop (2020 Spring)
   ESP32 development SW installation

   Exercise 2: Hello World world with event loop
   This code creates two mix up task and event loops to output two strings of "Hello" and "World" with separate delays.

   Authors: Group G (Marco Hanisch, Romain Thollot, Antoine Pinon)
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

void vTaskFunction( void *pvParameters );
/*-----------------------------------------------------------*/


void app_main(void)
{
    xTaskCreate(vTaskFunction, "vTaskFunction", 10000, NULL, 1, NULL); 
    while(true){
        //-- Task application code here. --
        printf("World!\n");
        vTaskDelay(1500 / portTICK_PERIOD_MS);
    }
}


void vTaskFunction( void *pvParameters )
{
    while(true){
        printf("Hello ");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete( NULL );
}