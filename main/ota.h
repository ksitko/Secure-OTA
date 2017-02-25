// Copyright (c) 2017 Krzysztof Sitko
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#ifndef OTA_H
#define OTA_H

#include "esp_partition.h"

#define TASKDELAY 4096
#define BUFFSIZE 4096
#define REQUEST_SIZE 64
#define WIFI_WAIT_BIT BIT0
#define PARTITION_SIZE 0x00100000

typedef struct
{
    esp_partition_t current_partition;
    esp_partition_t new_partition;
	char *status;
}
ota_contex;

/*
 * Select which partition would be flashed with new firmware as well as
 * currently used partition.
 */
bool ota_init(ota_contex *ota_ctx);

/*
 * Initialize context variable. Select which partition would be flashed with new
 * firmware as well as currently used partition.
 */
int ota_write(char* text, int buff_len, int binary_file_length);

/*
 * Main ota thread. Initialize, wait for wifi CONNECTED_BIT (BIT0). Start a TLS
 * connection to the S3 Bucket holding binary. Extract binary length and MD5sum
 * (ETag) from header. Calculate the currently used parition's MD5sum and compare
 * it to that of the remote. If there is a mismatch, read and write the binary
 * length of the remote binary's into the new partition. Calculate the MD5sum of
 * the new partition. If it matches, set new partion as boot partition and restart.
 */
void ota_task(void *pvParameterm);

/*
 * Return status of OTA process
 */
char* ota_status(ota_contex* ota_ctx);

void __attribute__((noreturn)) task_fatal_error();

#endif
/*
How To use:
initialise_wifi();
xTaskCreate(&ota_task, "ota_task", 16384, NULL, 5, NULL);
*/
