#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event_loop.h"
#include "esp_log.h"
/*Mbed TLS*/
#include "mbedtls/platform.h"
#include "mbedtls/net.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "ota_tls.h"

mbedtls_ssl_context ssl;
mbedtls_ssl_config conf;
mbedtls_net_context server_fd;
mbedtls_entropy_context entropy;
mbedtls_ctr_drbg_context ctr_drbg;
mbedtls_x509_crt cacert;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
// static EventGroupHandle_t wifi_event_group;
#define OTA_TCP_PORT "443"
#define OTA_SERVER   CONFIG_SERVER
#define OTA_FILENAME CONFIG_FILENAME

static const char *TAG = "ota-tls";

// static const char *TAG = "example";

static const char *REQUEST = "GET " OTA_FILENAME " HTTP/1.1\n"
    "Host: "OTA_SERVER"\n"
    "User-Agent: esp-idf/1.0 esp32\n"
    "\n";


/*mbedTLS*/
/* Root cert for howsmyssl.com, taken from server_root_cert.pem
   The PEM file was extracted from the output of this command:
   openssl s_client -showcerts -connect www.howsmyssl.com:443 </dev/null
   openssl s_client -showcerts -connect s3.amazonaws.com:443 </dev/null
   The CA root cert is the last cert given in the chain of certs.
   To embed it in the app binary, the PEM file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/

extern const uint8_t server_root_cert_pem_start[] asm("_binary_s3amazonawscom_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_s3amazonawscom_pem_end");


#ifdef MBEDTLS_DEBUG_C

#define MBEDTLS_DEBUG_LEVEL 4
/* mbedtls debug function that translates mbedTLS debug output
   to ESP_LOGx debug output.
   MBEDTLS_DEBUG_LEVEL 4 means all mbedTLS debug output gets sent here,
   and then filtered to the ESP logging mechanism.
*/
static void mbedtls_debug(void *ctx, int level,
                     const char *file, int line,
                     const char *str)
{
    const char *MBTAG = "mbedtls";
    char *file_sep;
    /* Shorten 'file' from the whole file path to just the filename
       This is a bit wasteful because the macros are compiled in with
       the full _FILE_ path in each case.
    */
    file_sep = rindex(file, '/');
    if(file_sep)
        file = file_sep+1;

    level = 4;
    switch(level) {
    case 1:
        ESP_LOGI(MBTAG, "%s:%d %s", file, line, str);
        break;
    case 2:
    case 3:
        ESP_LOGD(MBTAG, "%s:%d %s", file, line, str);
    case 4:
        ESP_LOGV(MBTAG, "%s:%d %s", file, line, str);
        break;
    default:
        ESP_LOGE(MBTAG, "Unexpected log level %d: %s", level, str);
        break;
    }
}
#endif
/**/

void ota_tls_exit(){
  ESP_LOGI(TAG, "mbedtls_ssl_close_notify");
  mbedtls_ssl_close_notify(&ssl);

  ESP_LOGI(TAG, "mbedtls_net_free");
  mbedtls_net_free(&server_fd);

  ESP_LOGD(TAG,"mbedtls_x509_crt_free ");
  mbedtls_x509_crt_free(&cacert);

  ESP_LOGD(TAG,"mbedtls_entropy_free");
  mbedtls_entropy_free(&entropy);

  ESP_LOGD(TAG,"mbedtls_ctr_drbg_free");
  mbedtls_ctr_drbg_free(&ctr_drbg);

  ESP_LOGD(TAG,"mbedtls_ssl_config_free");
  mbedtls_ssl_config_free(&conf);

  ESP_LOGD(TAG,"mbedtls_ssl_free");
  mbedtls_ssl_free(&ssl);
}

void ota_tls_init()
{
    int ret, flags;
    char buf[512];

    mbedtls_ssl_init(&ssl);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    ESP_LOGI(TAG, "Seeding the random number generator");

    mbedtls_ssl_config_init(&conf);

    mbedtls_entropy_init(&entropy);
    if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                    NULL, 0)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
        abort();
    }

    ESP_LOGI(TAG, "Loading the CA root certificate...");
    ret = mbedtls_x509_crt_parse(&cacert, server_root_cert_pem_start,
                                 server_root_cert_pem_end-server_root_cert_pem_start);
    if(ret < 0)
    {
        ESP_LOGE(TAG, "mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
        abort();
    }

    ESP_LOGI(TAG, "Setting hostname for TLS session...");

     /* Hostname set here should match CN in server certificate */
    if((ret = mbedtls_ssl_set_hostname(&ssl, OTA_SERVER)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
        abort();
    }

    ESP_LOGI(TAG, "Setting up the SSL/TLS structure...");
    if((ret = mbedtls_ssl_config_defaults(&conf,
                                          MBEDTLS_SSL_IS_CLIENT,
                                          MBEDTLS_SSL_TRANSPORT_STREAM,
                                          MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ssl_config_defaults returned %d", ret);
        ota_tls_exit();
    }

    /* MBEDTLS_SSL_VERIFY_OPTIONAL is bad for security, in this example it will print
       a warning if CA verification fails but it will continue to connect.
       You should consider using MBEDTLS_SSL_VERIFY_REQUIRED in your own code.
    */
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
#ifdef MBEDTLS_DEBUG_C
    // mbedtls_debug_set_threshold(MBEDTLS_DEBUG_LEVEL);
    mbedtls_ssl_conf_dbg(&conf, mbedtls_debug, NULL);
#endif

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
        ota_tls_exit();
    }

    mbedtls_net_init(&server_fd);

    ESP_LOGI(TAG, "Connecting to %s:%s...", OTA_SERVER, OTA_TCP_PORT);

    if ((ret = mbedtls_net_connect(&server_fd, OTA_SERVER,
                                  OTA_TCP_PORT, MBEDTLS_NET_PROTO_TCP)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_net_connect returned -%x", -ret);
        ota_tls_exit();
    }

    ESP_LOGI(TAG, "Connected.");

    mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    ESP_LOGI(TAG, "Performing the SSL/TLS handshake...");

    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0)
    {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            ESP_LOGE(TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
            ota_tls_exit();
        }
    }

    ESP_LOGI(TAG, "Verifying peer X.509 certificate...");

    if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0)
    {
        /* In real life, we probably want to close connection if ret != 0 */
        ESP_LOGW(TAG, "Failed to verify peer certificate!");
        bzero(buf, sizeof(buf));
        mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);
        ESP_LOGW(TAG, "verification info: %s", buf);
    }
    else {
        ESP_LOGI(TAG, "Certificate verified.");
    }

    ESP_LOGI(TAG, "Writing HTTP request...");

    while((ret = mbedtls_ssl_write(&ssl, (const unsigned char *)REQUEST, strlen(REQUEST))) <= 0)
    {
        if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            ESP_LOGE(TAG, "mbedtls_ssl_write returned -0x%x", -ret);
            ota_tls_exit();
        }
    }
}

int ota_tls_read(unsigned char *output, int buffer){
  return mbedtls_ssl_read(&ssl, (unsigned char *)output, buffer);
}
