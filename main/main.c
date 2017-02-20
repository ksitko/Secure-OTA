/* OTA example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "wifi.h"

#include "ota.h"

void hello_task(void *pvParameter)
{
    while(1){
    printf("Hello world! This is Secure ESP OTA 0.91\n");
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    }
}

void app_main()
{
    nvs_flash_init();
    initialise_wifi();
    xTaskCreate(&ota_task, "ota_task", 16384, NULL, 5, NULL);
    xTaskCreate(&hello_task, "hello_task", 2048, NULL, 2, NULL);
}
