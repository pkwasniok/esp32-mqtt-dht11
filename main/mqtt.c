#include "mqtt.h"
#include "config.h"
#include "mqtt_client.h"
#include "dht11.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"

#define TAG "MQTT"

#define EVENT_CONNECTED    BIT0
#define EVENT_DISCONNECTED BIT1
#define EVENT_SAMPLE       BIT2

#define TOPIC_TEMPERATURE CONFIG_MQTT_TOPIC_BASE "/" "temperature"
#define TOPIC_HUMIDITY    CONFIG_MQTT_TOPIC_BASE "/" "humidity"

EventGroupHandle_t event_group_mqtt;
TimerHandle_t  timer_publish;

char topic[255];
char data[255];

void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    esp_mqtt_event_id_t mqtt_event_id = event_id;

    switch (mqtt_event_id) {
        case MQTT_EVENT_CONNECTED:
            xEventGroupSetBits(event_group_mqtt, EVENT_CONNECTED);
            break;

        case MQTT_EVENT_DISCONNECTED:
            xEventGroupSetBits(event_group_mqtt, EVENT_DISCONNECTED);
            break;

        default:
            break;
    }
}

void timer_publish_callback(TimerHandle_t timer) {
    xEventGroupSetBits(event_group_mqtt, EVENT_SAMPLE);
}

void mqtt_task(dht_t* dht) {
    ESP_LOGI(TAG, "Started MQTT task");

    event_group_mqtt = xEventGroupCreate();

    timer_publish = xTimerCreate("timer_publish", CONFIG_SAMPLING_PERIOD_MS / portTICK_PERIOD_MS, pdTRUE, (void*) 0, timer_publish_callback);

    esp_mqtt_client_config_t mqtt_config = {
        .broker.address.uri = CONFIG_MQTT_BROKER,
    };

    esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, &mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);

    while (1) {
        EventBits_t event_bits = xEventGroupWaitBits(event_group_mqtt, EVENT_DISCONNECTED | EVENT_CONNECTED | EVENT_SAMPLE, pdTRUE, pdFALSE, portMAX_DELAY);

        if (event_bits & EVENT_CONNECTED) {
            ESP_LOGI(TAG, "Connected to broker");
            xTimerStart(timer_publish, 0);
        }

        if (event_bits & EVENT_DISCONNECTED) {
            ESP_LOGE(TAG, "Disconnected from broker");
            xTimerStop(timer_publish, 0);
            return;
        }

        if (event_bits & EVENT_SAMPLE) {
            ESP_LOGI(TAG, "Sampling and publishing sensor data");

            dht_reading_t reading;

            if (dht_read(dht, &reading) == DHT_SUCCESS) {
                char data_temperature[255];
                sprintf(data_temperature, "%d", reading.temperature);

                char data_humidity[255];
                sprintf(data_humidity, "%d", reading.humidity);

                esp_mqtt_client_publish(mqtt_client, TOPIC_TEMPERATURE, data_temperature, strlen(data_temperature), 0, 1);
                esp_mqtt_client_publish(mqtt_client, TOPIC_HUMIDITY, data_humidity, strlen(data_humidity), 0, 1);
            }
        }
    }
}

