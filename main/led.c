#include "led.h"
#include "config.h"
#include "driver/gpio.h"

#define GPIO_LED CONFIG_GPIO_LED

void led_setup(void) {
    gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT);
    led_disable();
}

void led_enable(void) {
    gpio_set_level(GPIO_LED, 0);
}

void led_disable(void) {
    gpio_set_level(GPIO_LED, 1);
}

