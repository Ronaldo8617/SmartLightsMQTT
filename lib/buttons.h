// botoes.h
#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"

#define DEBOUNCE_DELAY 300 // Tempo de debounce (em milissegundos)
#define BOTAO_A 5         // Pino do Botão A
#define BOTAO_B 6        // Pino do Botão B
#define BOTAO_J 22      // Pino do Botão Joystick

extern volatile uint32_t last_irq_time_A;
extern volatile uint32_t last_irq_time_B;
extern volatile uint32_t last_irq_time_J;
extern bool estado_LED_A;
extern bool estado_LED_B;

void botao_callback(uint gpio, uint32_t eventos);
void iniciar_botoes();

#endif // BOTOES_H