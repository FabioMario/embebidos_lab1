#include <stdio.h>
#include "delay.h"
#include "rgb_led.h"

// Tiempo de parpadeo en milisegundos
#define BLINK_PERIOD_MS 500

// Variables del Ejercicio 3
#define ARRAY_SIZE 10
int exampleData;
char exampleArray[ARRAY_SIZE];

void app_main(void)
{
    void *led = init_led();    
    if (led == NULL) {
        printf("No se pudo inicializar el LED\n");
        return;
    }


    while (1)
    {
        printf("Color: ROJO\n");
        switch_red(led);
        delay_ms(BLINK_PERIOD_MS);

        printf("Color: VERDE\n");
        switch_green(led);
        delay_ms(BLINK_PERIOD_MS);

        printf("Color: AZUL\n");
        switch_blue(led);
        delay_ms(BLINK_PERIOD_MS);

        printf("Color: APAGADO\n");
        switch_off(led);
        delay_ms(BLINK_PERIOD_MS);
    }

    free(led);
}