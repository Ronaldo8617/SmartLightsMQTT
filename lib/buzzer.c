#include "lib/buzzer.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"

volatile bool buzzer_play_A = false;
volatile bool buzzer_play_B = false;
volatile bool buzzer_play_J = false;

void buzzer_init(uint gpio, uint freq_hz) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);

    uint32_t clock_freq = 125000000; // Clock padrão da PWM (125 MHz)
    uint32_t divider16 = clock_freq / freq_hz / 4096 + 1;
    if (divider16 < 16) divider16 = 16;
    uint32_t wrap = clock_freq * 16 / divider16 / freq_hz - 1;

    pwm_set_clkdiv_int_frac(slice, divider16 / 16, divider16 & 0xF);
    pwm_set_wrap(slice, wrap);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(gpio), wrap / 2); // 50% duty cycle
    pwm_set_enabled(slice, true);
}

void buzzer_set_freq(uint gpio, uint freq_hz) {
    uint slice = pwm_gpio_to_slice_num(gpio);

    uint32_t clock_freq = 125000000;
    uint32_t divider16 = clock_freq / freq_hz / 4096 + 1;
    if (divider16 < 16) divider16 = 16;
    uint32_t wrap = clock_freq * 16 / divider16 / freq_hz - 1;

    pwm_set_clkdiv_int_frac(slice, divider16 / 16, divider16 & 0xF);
    pwm_set_wrap(slice, wrap);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(gpio), wrap / 2);
}

void buzzer_stop(uint gpio) {
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(gpio), 0); // Duty cycle 0%
}
