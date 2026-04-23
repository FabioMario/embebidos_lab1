#include <stdio.h>
#include "led_strip.h"
#include "delay.h"

// Tiempo de parpadeo en milisegundos
#define BLINK_PERIOD_MS 500

// Variables del Ejercicio 3
#define ARRAY_SIZE 10
int exampleData;
char exampleArray[ARRAY_SIZE];

void app_main(void)
{
    led_strip_t *strip = NULL;
    if (led_rgb_init(&strip) != ESP_OK || strip == NULL)
    {
        printf("No se pudo inicializar el LED\n");
        return;
    }

    while (1)
    {
        printf("Color: ROJO\n");
        strip->set_pixel(strip, 0, 255, 0, 0);
        strip->refresh(strip, 100);
        delay_ms(BLINK_PERIOD_MS);

        printf("Color: VERDE\n");
        strip->set_pixel(strip, 0, 0, 255, 0);
        strip->refresh(strip, 100);
        delay_ms(BLINK_PERIOD_MS);

        printf("Color: AZUL\n");
        strip->set_pixel(strip, 0, 0, 0, 255);
        strip->refresh(strip, 100);
        delay_ms(BLINK_PERIOD_MS);

        printf("Color: APAGADO\n");
        strip->clear(strip, 100);
        delay_ms(BLINK_PERIOD_MS);
    }
}