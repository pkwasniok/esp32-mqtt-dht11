#include "dht11.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ==================
// ===== MACROS =====
// ==================

#define _DHT_FRAME_LENGTH 5

#define _DHT_RET_EIO_ON_ERR(expression) if (expression != DHT_SUCCESS) return DHT_EIO

// =======================
// ===== PRIVATE API =====
// =======================

void _dht_send_request(dht_t* dht) {
    gpio_set_level(dht->gpio_num, 0);
    vTaskDelay(20 / portTICK_PERIOD_MS);
    gpio_set_level(dht->gpio_num, 1);
}

int _dht_get_pulse_width(dht_t* dht, int level, int64_t timeout_us) {
    int64_t time_start = esp_timer_get_time();

    while (gpio_get_level(dht->gpio_num) == level) {
        if (esp_timer_get_time() - time_start >= timeout_us) {
            return timeout_us;
        }
    }

    return esp_timer_get_time() - time_start;
}

int _dht_wait_pulse(dht_t* dht, int level, int64_t timeout_us) {
    int64_t pulse_width = _dht_get_pulse_width(dht, level, timeout_us);

    if (pulse_width == timeout_us) {
        return DHT_EIO;
    }

    return DHT_SUCCESS;
}

int _dht_verify_checksum(uint8_t* buffer) {
    uint8_t frame_checksum = buffer[_DHT_FRAME_LENGTH - 1];

    uint8_t calculated_checksum = 0;
    for(int i = 0; i < _DHT_FRAME_LENGTH - 1; i++) {
        calculated_checksum = (calculated_checksum + buffer[i]) % 256;
    }

    if (frame_checksum != calculated_checksum) {
        return DHT_EIO;
    }

    return DHT_SUCCESS;
}

int _dht_read(dht_t* dht, uint8_t* buffer) {
    _dht_send_request(dht);

    _DHT_RET_EIO_ON_ERR(_dht_wait_pulse(dht, 1, 55));

    _DHT_RET_EIO_ON_ERR(_dht_wait_pulse(dht, 0, 95));

    _DHT_RET_EIO_ON_ERR(_dht_wait_pulse(dht, 1, 95));

    for (int i = 0; i < _DHT_FRAME_LENGTH; i++) {
        buffer[i] = 0;

        for (int j = 0; j < 8; j++) {
            _DHT_RET_EIO_ON_ERR(_dht_wait_pulse(dht, 0, 65));

            int64_t pulse_width = _dht_get_pulse_width(dht, 1, 85);
            int bit = pulse_width > 50;

            buffer[i] = buffer[i] << 1 | bit;
        }
    }

    _DHT_RET_EIO_ON_ERR(_dht_wait_pulse(dht, 0, 65));

    _DHT_RET_EIO_ON_ERR(_dht_verify_checksum(buffer));

    return DHT_SUCCESS;
}

dht_temperature_t _dht_decode_temperature(uint8_t* buffer) {
    return buffer[2];
}

dht_humidity_t _dht_decode_humidity(uint8_t* buffer) {
    return buffer[0];
}

// ======================
// ===== PUBLIC API =====
// ======================

int dht_init(dht_t *dht, dht_model_t dht_model, gpio_num_t gpio_num) {
    dht->dht_model = dht_model;
    dht->gpio_num = gpio_num;

    gpio_set_direction(gpio_num, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_level(gpio_num, 1);

    return DHT_SUCCESS;
}

int dht_read(dht_t* dht, dht_reading_t* dht_reading) {
    uint8_t buffer[_DHT_FRAME_LENGTH];

    if (_dht_read(dht, buffer)) {
        return DHT_EIO;
    }

    dht_reading->temperature = _dht_decode_temperature(buffer);
    dht_reading->humidity = _dht_decode_humidity(buffer);

    return DHT_SUCCESS;
}

