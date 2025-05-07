#include "setup.h"
#include "config.h"
#include "led.h"
#include "wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"

#define TAG "SETUP"

#define SUCCESS 0
#define ERROR   1

#define STATE_SETUP_IO    0
#define STATE_SETUP_NVS   1
#define STATE_SETUP_NETIF 2
#define STATE_SETUP_WIFI  3
#define STATE_SUCCESS     4
#define STATE_ERROR       5

/*
 * Non-volatile storage (NVS) library is designed to store key-value pairs in flash.
*/
int nvs_setup(void) {
    esp_err_t err;

    err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        if (nvs_flash_erase() == ESP_OK) {
            err = nvs_flash_init();
        }
    }

    if (err != ESP_OK) {
        return ERROR;
    }

    return SUCCESS;
}

/*
 * ESP-NETIF provides an abstraction layer for the application on top of the TCP/IP stack.
*/
int netif_setup(void) {
    if (esp_netif_init() != ESP_OK) {
        return ERROR;
    }

    if (esp_event_loop_create_default() != ESP_OK) {
        return ERROR;
    }

    esp_netif_t* netif_wifi_sta = esp_netif_create_default_wifi_sta();

    if (esp_netif_set_hostname(netif_wifi_sta, CONFIG_HOSTNAME) != ESP_OK) {
        return ERROR;
    }

    return SUCCESS;
}

int app_setup(void) {
    int state = STATE_SETUP_IO;

    while (1) {
        switch (state) {

            case STATE_SETUP_IO:
                led_setup();
                ESP_LOGI(TAG, "Finished IO setup");
                state = STATE_SETUP_NVS;
                break;

            case STATE_SETUP_NVS:
                if (nvs_setup() == SUCCESS) {
                    ESP_LOGI(TAG, "Finished NVS setup");
                    state = STATE_SETUP_NETIF;
                } else {
                    ESP_LOGE(TAG, "Error occured during NVS setup");
                    state = STATE_ERROR;
                }
                break;

            case STATE_SETUP_NETIF:
                if (netif_setup() == SUCCESS) {
                    ESP_LOGI(TAG, "Finished NETIF setup");
                    state = STATE_SETUP_WIFI;
                } else {
                    ESP_LOGE(TAG, "Error occured during NETIF setup");
                    state = STATE_ERROR;
                }
                break;

            case STATE_SETUP_WIFI:
                if (wifi_setup() == SUCCESS) {
                    ESP_LOGI(TAG, "Finished WIFI setup");
                    state = STATE_SUCCESS;
                } else {
                    ESP_LOGE(TAG, "Error occured during WIFI setup");
                    state = STATE_ERROR;
                }
                break;

            case STATE_SUCCESS:
                return SETUP_SUCCESS;
                break;

            case STATE_ERROR:
                return SETUP_ERROR;
                break;

        }
    }
}

