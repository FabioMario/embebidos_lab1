#pragma once
#include "led_strip.h"

/** 
* Inicializa el LED RGB y devuelve un puntero para controlarlo.
* Liberar el puntero cuando ya no sea necesario.
* @returns el puntero al LED RGB o NULL si la inicialización falla.
*/
void *init_led();

/**
* Cambia el color del LED a rojo.
* @param led: controlador del LED RGB.
*/
void switch_red(void *led);

/**
* Cambia el color del LED a verde.
* @param led: controlador del LED RGB.
*/
void switch_green(void *led);

/**
* Cambia el color del LED a azul.
* @param led: controlador del LED RGB.
*/
void switch_blue(void *led);

/**
* Apaga el LED.
* @param led: controlador del LED RGB.
*/
void switch_off(void *led);