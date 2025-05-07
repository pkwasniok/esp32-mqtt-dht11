#pragma once
#include "driver/gpio.h"

#define DHT_SUCCESS 0
#define DHT_EIO     1

typedef enum {
    DHT_DHT11
} dht_model_t;

typedef struct {
    dht_model_t dht_model;
    gpio_num_t gpio_num;
} dht_t;

typedef int8_t dht_temperature_t;
typedef int8_t dht_humidity_t;

typedef struct {
    dht_temperature_t temperature;
    dht_humidity_t humidity;
} dht_reading_t;

int dht_init(dht_t* dht, dht_model_t dht_model, gpio_num_t gpio_num);

int dht_read(dht_t* dht, dht_reading_t* reading);

