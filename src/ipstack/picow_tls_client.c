#include <mbedtls/debug.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#if 1
#define WIFI_SSID "iPhone (Lore)"
#define WIFI_PASSWORD "moromoro123"

#define TS_API_KEY "CMP4T8OD4I7JIO0E"
#define TB_API_KEY "C7BZYN6EG1OWPVS1"
#define TB_ID      "53310"

#define TLS_CLIENT_SERVER        "api.thingspeak.com"
#define TLS_CLIENT_HTTP_SEND_DATA  ("GET /update?api_key="TS_API_KEY"&field1=100&field2=60&field3=24&field4=40&field5=150 HTTP/1.1\r\n" \
                                  "Host: api.thingspeak.com\r\n" \
                                  "User-Agent: PicoW\r\n" \
                                  "Accept: */*\r\n" \
                                  "Content-Length: 0\r\n" \
                                  "Content-Type: application/x-www-form-urlencoded\r\n" \
                                  "\r\n")
#define TLS_CLIENT_HTTP_RECEIVE_DATA  "GET /channels/2684345/fields/5.json?api_key=CRM2H2J47Q1DGW7B&results=2 HTTP/1.1\r\n" \
                                  "Host: api.thingspeak.com\r\n" \
                                  "User-Agent: PicoW\r\n" \
                                  "Accept: */*\r\n" \
                                  "Content-Length: 0\r\n" \
                                  "Content-Type: application/x-www-form-urlencoded\r\n" \
                                  "\r\n"
#define TLS_CLIENT_TB_QUEUE_READ ("POST /talkbacks/"TB_ID"/commands/execute.json HTTP/1.1\r\n" \
                                  "Host: api.thingspeak.com\r\n" \
                                  "Content-Length: 24\r\n" \
                                  "Content-Type: application/x-www-form-urlencoded\r\n" \
                                  "\r\n" \
                                  "api_key="TB_API_KEY)
#define TLS_CLIENT_TIMEOUT_SECS  15
#endif
#define TS_ROOT_CERT "-----BEGIN CERTIFICATE-----\n\
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n\
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n\
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n\
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n\
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n\
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n\
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n\
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n\
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n\
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n\
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n\
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n\
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n\
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n\
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n\
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n\
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n\
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n\
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n\
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n\
-----END CERTIFICATE-----\n"

extern bool run_tls_client_test(const uint8_t *cert, size_t cert_len, const char *server, const char *request, int timeout);


void tls_test(int send_or_receive) {
#if 0
    struct timeval now;
    now.tv_sec = 1725920831;
    now.tv_usec = 0;
    settimeofday(&now, NULL);
#endif
    char ssid[] = WIFI_SSID;
    char pwd[] = WIFI_PASSWORD;
    printf("SSID: %s\nPWD: %s\n", ssid, pwd);//WIFI_SSID, WIFI_PASSWORD);
    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return;
    }


    printf("Connecting....\n");
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect\n");
        return;
    } else printf("Connected to the WIFI.\n");

    const uint8_t ts_root_cert[] = TS_ROOT_CERT;

    printf("Trying to send HTTPS message.\n");
    bool pass;
    if (send_or_receive == 1)pass = run_tls_client_test(ts_root_cert, sizeof(ts_root_cert), TLS_CLIENT_SERVER, TLS_CLIENT_HTTP_SEND_DATA, TLS_CLIENT_TIMEOUT_SECS);
    if (send_or_receive == 0)pass = run_tls_client_test(ts_root_cert, sizeof(ts_root_cert), TLS_CLIENT_SERVER, TLS_CLIENT_TB_QUEUE_READ, TLS_CLIENT_TIMEOUT_SECS);

    if (pass) {
        printf("Test passed\n");
    } else {
        printf("Test failed\n");
    }
    /* sleep a bit to let usb stdio write out any buffer to host */
    sleep_ms(100);

    cyw43_arch_deinit();
    printf("All done\n");
    return;
}


