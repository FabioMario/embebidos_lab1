#pragma once
#include <stdint.h>

/**
* Bloquea la ejecución durante el tiempo especificado en milisegundos.
* @param ms: milisegundos a esperar.
*/
void delay_ms(uint32_t ms);

/**
* Bloquea la ejecución durante el tiempo especificado en segundos.
* @param s: segundos a esperar.
*/
void delay_s(uint32_t s);