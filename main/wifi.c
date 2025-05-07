#include "wifi.h"
#include "config.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

EventGroupHandle_t event_group;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupSetBits(event_group, BIT1);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(event_group, BIT0);
    }
}

int wifi_setup(void) {
    event_group = xEventGroupCreate();

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();

    if (esp_wifi_init(&init_config) != ESP_OK) {
        return WIFI_ERROR;
    }

    esp_event_handler_instance_t instance_any_id;
    if (esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id) != ESP_OK) {
        return WIFI_ERROR;
    }

    esp_event_handler_instance_t instance_got_ip;
    if (esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip) != ESP_OK) {
        return WIFI_ERROR;
    }

    wifi_config_t config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_WPA3_PSK,
        }
    };

    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) {
        return WIFI_ERROR;
    }

    if (esp_wifi_set_config(WIFI_IF_STA, &config) != ESP_OK) {
        return WIFI_ERROR;
    }

    if (esp_wifi_start() != ESP_OK) {
        return WIFI_ERROR;
    }

    EventBits_t event_bits = xEventGroupWaitBits(event_group, BIT0 | BIT1, pdFALSE, pdFALSE, portMAX_DELAY);

    if (event_bits & BIT0) {
        return WIFI_SUCCESS;
    } else if (event_bits & BIT1) {
        return WIFI_ERROR;
    } else {
        return WIFI_ERROR;
    }

    return WIFI_SUCCESS;
}

