#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_partition.h"
#include "esp_log.h"
#include "mbedtls/md5.h"
#include "rom/spi_flash.h"

/**
 * \brief          MD5 context structure
 */
typedef struct
{
    int length;          /*!< number of bytes processed  */
    char local[32];   /*!< data block being processed */
    char remote[32];   /*!< data block being processed */
}
ota_header_context;

void validate_init(ota_header_context *context);
bool validate_bin(char *text);
bool remote_get_size(ota_header_context *context, char *header);
bool remote_get_hash(ota_header_context *context, char *header);
bool validate_remote_headers(ota_header_context *context, char *header);
bool local_get_hash(ota_header_context *context, const esp_partition_t* partition);
bool validate_hashes(ota_header_context *context, const esp_partition_t* partition);
int bin_length(ota_header_context *context);
/*
//How to use:
ota_header_context ctx;
validate_init(&ctx);
validate_remote_headers(&ctx,buffer_containing_header); //after pulling binary header
validate_bin(buffer_containing_bin); //confirm first bin is valid via first 2 bytes
bin_length(&ctx); //write bin_length number of bytes
validate_hashes(&ctx, &partition); //after writing binary to partition

*/