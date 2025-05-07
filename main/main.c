#include "led.h"
#include "setup.h"
#include "mqtt.h"
#include "dht11.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#define TAG "APP"

#define STATE_SETUP   0
#define STATE_RUNNING 1
#define STATE_ERROR   2

void app_main(void)
{
    dht_t dht;

    int state = STATE_SETUP;
    while (1) {
        switch (state) {

            case STATE_SETUP:
                ESP_LOGI(TAG, "Entering setup state");

                if (app_setup(&dht) == SETUP_SUCCESS) {
                    state = STATE_RUNNING; 
                } else {
                    state = STATE_ERROR;
                }

                break;

            case STATE_RUNNING:
                ESP_LOGI(TAG, "Entering running state");
                led_enable();
                mqtt_task(&dht);

                state = STATE_ERROR;
                led_disable();

                break;

            case STATE_ERROR:
                ESP_LOGI(TAG, "Entering error state");

                ESP_LOGE(TAG, "Restarting in 5 seconds...");
                vTaskDelay(5000 / portTICK_PERIOD_MS);
                esp_restart();

                break;

        }
    }
}

