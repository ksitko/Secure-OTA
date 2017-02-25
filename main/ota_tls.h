/*
*
* Adapted from the https_request example in esp-idf.
* Adapted from the ssl_client1 example in mbedtls.
*
* Original Copyright (C) 2006-2016, ARM Limited, All Rights Reserved, Apache 2.0 License.
* Additions Copyright (C) Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD, Apache 2.0 License.
* Additions Copyright (C) Copyright 2017 Krzysztof Sitko, Apache 2.0 License.
*
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef OTA_TLS_H
#define OTA_TLS_H

/*
 * Free all of the mbedtls context variables
 */
void ota_tls_exit();

/*
 * mbedtls connection creat. NEEDS TO BE CLEANED UP.
 */
void ota_tls_init();

/*
 * Read from TLS connection.
 */
int ota_tls_read(unsigned char *output, int buffer);

#endif

/*
How To use:
ota_tls_init();
int buff_len = ota_tls_read((unsigned char *)text, BUFFSIZE);
ota_tls_exit();
*/
