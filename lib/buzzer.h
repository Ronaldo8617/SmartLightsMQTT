#ifndef BUZZER_H
#define BUZZER_H

#include "pico/stdlib.h"

#define buzzer 21
extern volatile bool buzzer_play_A;
extern volatile bool buzzer_play_B;
extern volatile bool buzzer_play_J;

void buzzer_init(uint gpio, uint freq_hz);
void buzzer_set_freq(uint gpio, uint freq_hz);
void buzzer_stop(uint gpio);

#endif
