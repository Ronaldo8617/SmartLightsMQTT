#ifndef RGB_H
#define RGB_H

#include <stdio.h>
#include "pico/stdlib.h"
#include <stdbool.h>

#define blue 12       // Pino PWM para o LED azul
#define red 13        // Pino PWM para o LED vermelho
#define green 11

void iniciar_rgb(void);
void state(bool rr, bool gg, bool bb);

#endif