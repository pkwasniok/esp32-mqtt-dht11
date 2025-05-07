#include "mqtt.h"
#include "config.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define TAG "MQTT"

#define EVENT_CONNECTED    BIT0
#define EVENT_DISCONNECTED BIT1
#define EVENT_DATA         BIT2

EventGroupHandle_t event_group_mqtt;

char topic[255];
char data[255];

void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    esp_mqtt_event_handle_t mqtt_event = event_data;
    esp_mqtt_event_id_t mqtt_event_id = event_id;

    switch (mqtt_event_id) {
        case MQTT_EVENT_CONNECTED:
            xEventGroupSetBits(event_group_mqtt, EVENT_CONNECTED);
            break;

        case MQTT_EVENT_DISCONNECTED:
            xEventGroupSetBits(event_group_mqtt, EVENT_DISCONNECTED);
            break;

        case MQTT_EVENT_DATA:
            memcpy(topic, mqtt_event->topic, mqtt_event->topic_len);
            topic[mqtt_event->topic_len] = '\0';

            memcpy(data, mqtt_event->data, mqtt_event->data_len);
            data[mqtt_event->data_len] = '\0';

            xEventGroupSetBits(event_group_mqtt, EVENT_DATA);
            break;

        default:
            break;
    }
}

void mqtt_task(void* pvParameters) {
    ESP_LOGI(TAG, "Started MQTT task");

    event_group_mqtt = xEventGroupCreate();

    esp_mqtt_client_config_t mqtt_config = {
        .broker.address.uri = CONFIG_MQTT_BROKER,
    };

    esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, &mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);

    while (1) {
        EventBits_t event_bits = xEventGroupWaitBits(event_group_mqtt, EVENT_DISCONNECTED | EVENT_CONNECTED | EVENT_DATA, pdTRUE, pdFALSE, portMAX_DELAY);

        if (event_bits & EVENT_CONNECTED) {
            ESP_LOGI(TAG, "Connected to broker");
        }

        if (event_bits & EVENT_DISCONNECTED) {
            ESP_LOGE(TAG, "Disconnected from broker");
            return;
        }

        if (event_bits & EVENT_DATA) {
            ESP_LOGI(TAG, "Received \"%s\" \"%s\"", topic, data);
        }
    }
}

