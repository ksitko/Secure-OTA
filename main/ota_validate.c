/* resolve read the responding header from the GET request and find the size of the binary
 * return true if found
 * otherwise return false
 * */
#include "ota_validate.h"

static const char *VAL_TAG = "ota_validate";

void validate_init(ota_header_context *context){
    memset( context, 0, sizeof(ota_header_context));
}

bool validate_bin(char *text)
{
    if (text[0] == 0xE9 && text[1] == 0x08) {
        ESP_LOGI(VAL_TAG, "OTA Write Header format Check OK. first byte is %02x ,second byte is %02x", text[0], text[1]);
        return true;
    } else {
        ESP_LOGE(VAL_TAG, "OTA Write Header format Check Failed! first byte is %02x ,second byte is %02x", text[0], text[1]);
        return false;
    }
}

bool remote_get_size(ota_header_context *context, char *header)
{
    /*Find ETag Key*/
    char *binlen_start;
    binlen_start = strstr(header,"Content-Length");
    if (binlen_start == NULL) {
        ESP_LOGE(VAL_TAG, "Cannot find binary length key!");
        return false;
    } else {
        ESP_LOGI(VAL_TAG, "Found binary length key");
    } 

    /*Find start of ETag Value*/
    int bin_length = 0;
    sscanf(binlen_start, "%*s %d", &bin_length);
    if (bin_length == 0) {
        ESP_LOGE(VAL_TAG, "Invalid binary length!");
        return false;
    } else {
        context->length = bin_length;
        ESP_LOGI(VAL_TAG, "Binary is %d bytes", bin_length);
        return true;
    }     
}

bool remote_get_hash(ota_header_context *context, char *header)
{
    /*Find ETag Key*/
    char *ETag_start;
    char *ETag_end;
    ETag_start = strstr(header,"ETag");
    if (ETag_start == NULL) {
        ESP_LOGE(VAL_TAG, "Cannot find ETag key!");
        return false;
    } else {
        ESP_LOGI(VAL_TAG, "Found ETag key");
    } 

    /*Find start of ETag Value*/
    ETag_start = strstr(ETag_start,"\"")+1;//Find first char after "
    if (ETag_start == NULL) {
        ESP_LOGE(VAL_TAG, "Cannot find start of ETag value!");
        return false;
    } else {
        ESP_LOGI(VAL_TAG, "Found start of ETag value");
    }   

    /*Find end of ETag Value*/
    ETag_end = strstr(ETag_start,"\"");
    if (ETag_end == NULL) {
        ESP_LOGE(VAL_TAG, "Cannot find end of ETag value!");
        return false;
    } else {
        ESP_LOGI(VAL_TAG, "Found end of ETag value");
    }   

    /*Find ETag Length*/
    int ETag_length=(int)ETag_end-(int)ETag_start;
    //Only Accepting ETag's which have the same length as MD5
    if (ETag_length != 32) {
        ESP_LOGE(VAL_TAG, "Invalid ETag value length!");
        return false;
    } else {
        ESP_LOGI(VAL_TAG, "ETag value length is %d", ETag_length);
    }   
    snprintf(context->remote,32,ETag_start);
    return true;
}

bool validate_remote_headers(ota_header_context *context, char *header){
    if (context== NULL){
        return false;
    }
    bool ret = remote_get_hash(context, header);
    ret &= remote_get_size(context, header);
    return ret;
}

bool local_get_hash(ota_header_context *context, const esp_partition_t* partition){
    unsigned char *partition_address = (unsigned char *)partition->address;
    unsigned char local_byte_array[16]={0};
    ESP_LOGI(VAL_TAG, "Calc MD5 Sum");
    mbedtls_md5_context ctx;
    mbedtls_md5_init( &ctx );
    mbedtls_md5_starts( &ctx );
    int ret=-1;
    int buffer_len=1;
    uint8_t buffer[buffer_len];
    for(int i=0;i<context->length/buffer_len;i++){
        ret=spi_flash_read((size_t)(partition_address+i*buffer_len), buffer, sizeof(buffer));
        mbedtls_md5_update( &ctx, buffer, (size_t)buffer_len);
    }
    mbedtls_md5_finish( &ctx, local_byte_array );
    mbedtls_md5_free( &ctx );

    if(ret!=ESP_OK){
        ESP_LOGI(VAL_TAG, "SPI Flash Read Fail!");  
        return false; 
    }
    char local_ETag_array[32]={0};
    char local_buff[2];
    for(int i=0; i<16;i++){
        sprintf(local_buff,"%02x",local_byte_array[i]);
        strcat(local_ETag_array,local_buff);
    }
    snprintf(context->local,32,local_ETag_array);
    return true;
}

bool validate_hashes(ota_header_context *context, const esp_partition_t* partition){
    if(local_get_hash(context, partition) == false ){
        return false;
    }
    if(strncmp (context->local,context->remote,32)==0){
        ESP_LOGI(VAL_TAG, "MD5 Checksum match!");
        return true;
    }else{
       ESP_LOGE(VAL_TAG, "Checksum Mismatch!"); 
        return false;
    }
}

int bin_length(ota_header_context *context){
    return context->length;
}