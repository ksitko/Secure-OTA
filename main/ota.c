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

#include "nvs_flash.h"

#include "ota_validate.h"
#include "ota_tls.h"
#include "wifi.h"

#include "ota.h"

static const char *TAG = "ota";
/*an packet receive buffer*/
char text[BUFFSIZE + 1] = { 0 };
/*socket id*/
int socket_id = -1;
char https_request[REQUEST_SIZE] = {0};
/* operate handle : uninitialized value is zero ,every ota begin would exponential growth*/
esp_ota_handle_t out_handle = 0;

/*****************/

bool ota_init(ota_contex *ota_ctx)
{
    esp_err_t err;
    memset(ota_ctx, 0, sizeof(ota_contex));

    const esp_partition_t *esp_current_partition = esp_ota_get_boot_partition();
    if (esp_current_partition->type != ESP_PARTITION_TYPE_APP) {
        ESP_LOGE(TAG, "Error： esp_current_partition->type != ESP_PARTITION_TYPE_APP");
        return false;
    }

    esp_partition_t find_partition;
    /*choose which OTA image should we write to*/
    switch (esp_current_partition->subtype) {
    case ESP_PARTITION_SUBTYPE_APP_FACTORY:
        find_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
        break;
    case  ESP_PARTITION_SUBTYPE_APP_OTA_0:
        find_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_1;
        break;
    case ESP_PARTITION_SUBTYPE_APP_OTA_1:
        find_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
        break;
    default:
        break;
    }
    find_partition.type = ESP_PARTITION_TYPE_APP;

    const esp_partition_t *partition = esp_partition_find_first(find_partition.type, find_partition.subtype, NULL);
    assert(partition != NULL);

    ESP_LOGI(TAG, "esp_ota_begin...");
    err = esp_ota_begin(partition, OTA_SIZE_UNKNOWN, &out_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed err=0x%x!", err);
        return false;
    } else {
        memcpy(&ota_ctx->new_partition, partition, sizeof(esp_partition_t));
        memcpy(&ota_ctx->current_partition, esp_current_partition, sizeof(esp_partition_t));
        ota_ctx->status="Partition's Found";
        ESP_LOGI(TAG, "esp_ota_begin init OK");
        return true;
    }
    return false;
}

void __attribute__((noreturn)) task_fatal_error()
{
    ESP_LOGE(TAG, "Exiting task due to fatal error...");
    close(socket_id);
    (void)vTaskDelete(NULL);

    while (1) {
        ;
    }
}

int ota_write(char* text, int buff_len, int binary_file_length){
    char ota_write_data[BUFFSIZE + 1] = { 0 };
    memset(ota_write_data, 0, BUFFSIZE);
    memcpy(ota_write_data, text, buff_len);
    esp_err_t err = esp_ota_write( out_handle, (const void *)ota_write_data, buff_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error: esp_ota_write failed! err=%x", err);
        task_fatal_error();
    }
    binary_file_length += buff_len;
    ESP_LOGI(TAG, "Have written image length %d", binary_file_length);
    ESP_LOGI(TAG, "buff_len: %d", buff_len);
    return binary_file_length;
}

void ota_task(void *pvParameter)
{
    while(1){
    ota_contex ota_ctx;
    esp_err_t err;

    ESP_LOGI(TAG, "Starting OTA example...");
    /* Wait for the callback to set the WIFI_WAIT_BIT in the
       event group.
    */
    ESP_LOGI(TAG, "OTA Init...");
    if ( ota_init(&ota_ctx) ) {
        ESP_LOGI(TAG, "OTA Init succeeded");
    } else {
        ESP_LOGE(TAG, "OTA Init failed");
        task_fatal_error();
    }
    ota_ctx.status="Firmware update initializing";
    bool pkg_body_start = false, flag = true;

    ESP_LOGI(TAG, "Wait for Wifi....");
    xEventGroupWaitBits(get_wifi_status(), WIFI_WAIT_BIT,
                        false, true, portMAX_DELAY);

    ESP_LOGI(TAG, "Connect to Wifi ! Start to Connect to Server....");
    ota_tls_init();

    ESP_LOGI(TAG, "Reading HTTPS response...");
    // if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
    //     continue;
    //
    // if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
    //     ret = 0;
    //     break;
    // }
    //
    // if(ret < 0)
    // {
    //     ESP_LOGE(TAG, "mbedtls_ssl_read returned -0x%x", -ret);
    //     break;
    // }
    //
    // if(ret == 0)
    // {
    //     ESP_LOGI(TAG, "connection closed");
    //     break;
    // }

    /*Check Header of remote Bin, compare to local*/
    int buff_len = ota_tls_read((unsigned char *)text, BUFFSIZE);

    ota_header_context ctx;
    validate_init(&ctx);
    if(validate_remote_headers(&ctx,text)!=true){
        ESP_LOGI(TAG, "Remote Binary cannot be confirmed.");
        ota_ctx.status="Remote Binary cannot be confirmed";
        ota_tls_exit();
    }else if(validate_hashes(&ctx, &ota_ctx.current_partition)){
        ESP_LOGI(TAG, "Current bin is same as remote bin");
        ota_ctx.status="Firmware is up to date";
        ota_tls_exit();
    }else{
    /* an image total length*/
    int binary_file_length = 0;
    ota_ctx.status="New Firmware is available; update is starting";

    /*Erase Partion*/
    /*to do: Find better way of getting partition size*/
    err = spi_flash_erase_range(ota_ctx.new_partition.address, PARTITION_SIZE);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Error: erase new partition failed! err=0x%x", err);
        task_fatal_error();
    }

    while (flag) {
        memset(text, 0, BUFFSIZE);
        buff_len = ota_tls_read((unsigned char *)text, BUFFSIZE);
        if (buff_len < 0) { /*receive error*/
            ESP_LOGE(TAG, "Error: receive data error! errno=%d", errno);
            task_fatal_error();
        } else if (buff_len > 0 && !pkg_body_start) { /*deal with packet header*/
            pkg_body_start = validate_bin(text);
            if (pkg_body_start)
            {
                binary_file_length=ota_write(text, buff_len, binary_file_length);
            }else{
                ESP_LOGE(TAG, "Error: Invalid Binary Start Bytes!");
                task_fatal_error();
            }
        } else if (buff_len > 0 && pkg_body_start) { /*deal with packet body*/
            binary_file_length=ota_write(text, buff_len, binary_file_length);
            if (binary_file_length==bin_length(&ctx)){
                ESP_LOGI(TAG, "Reached end of binary");
                flag = false;
                if(validate_hashes(&ctx, &ota_ctx.new_partition)){
                    ESP_LOGI(TAG, "Connection closed, all packets received");
                    ota_ctx.status="Firmware has updated and passed checksum";
                    close(socket_id);
                }else{
                    ota_ctx.status="Firmware has failed checksum";
                    task_fatal_error();
                }
            }
        } else if (buff_len == 0) {
            flag = false;
            ESP_LOGI(TAG, "Connection closed, all packets received");
            close(socket_id);
        } else {
            ESP_LOGE(TAG, "Unexpected recv result");
        }
    }

    ESP_LOGI(TAG, "Total Write binary data length : %d", binary_file_length);

    if (esp_ota_end(out_handle) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed!");
        task_fatal_error();
    }
    err = esp_ota_set_boot_partition(&ota_ctx.new_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed! err=0x%x", err);
        task_fatal_error();
    }
    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();
    return ;
    }
    ota_ctx.status="OTA is idle";
    vTaskDelay( TASKDELAY / portTICK_PERIOD_MS);
    }
}

char *ota_status(ota_contex* ota_ctx){
    return ota_ctx->status;
}
