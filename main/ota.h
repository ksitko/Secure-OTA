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
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

#include "nvs_flash.h"


#include "ota_validate.h"
#include "wifi.h"

// OPEN SSL Client inclution
#include "openssl/ssl.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"


typedef struct
{
    esp_partition_t current_partition;   /*!< data block being processed */
    esp_partition_t new_partition;   /*!< data block being processed */
	char *status;
}
ota_contex;

bool ota_openssl();
bool ota_init(ota_contex *ota_ctx);
int ota_write(char* text, int buff_len, int binary_file_length);
void ota_task(void *pvParameterm);
char* ota_status(ota_contex* ota_ctx);
void __attribute__((noreturn)) task_fatal_error();