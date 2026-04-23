#include <stdio.h>
#include "rgb_led.h"
#include "led_strip.h"

void *init_led() {
    led_strip_t *strip;
    if (led_rgb_init(&strip) != ESP_OK || strip == NULL) // Esto hace una allocation
    {
        return NULL;
    }
    return strip;
}

void switch_red(void *led) {
    led_strip_t *strip = (led_strip_t*) led;
    strip->set_pixel(led, 0, 255, 0, 0);
    strip->refresh(led, 100);
}

void switch_green(void *led) {
    led_strip_t *strip = (led_strip_t*) led;
    strip->set_pixel(strip, 0, 0, 255, 0);
    strip->refresh(strip, 100);
}

void switch_blue(void *led) {
    led_strip_t *strip = (led_strip_t*) led;
    strip->set_pixel(strip, 0, 0, 0, 255);
    strip->refresh(strip, 100);
}

void switch_off(void *led) {
    led_strip_t *strip = (led_strip_t*) led;
    strip->clear(strip, 100);
}