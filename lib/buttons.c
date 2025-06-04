#include "lib/buttons.h"

volatile uint32_t last_irq_time_A = 0;
volatile uint32_t last_irq_time_B = 0;
volatile uint32_t last_irq_time_J = 0;
bool estado_LED_A = false;
bool estado_LED_B = false;

void iniciar_botoes() {
    gpio_init(BOTAO_A);
    gpio_init(BOTAO_B);
    gpio_init(BOTAO_J);

    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_set_dir(BOTAO_J, GPIO_IN);

    gpio_pull_up(BOTAO_A);
    gpio_pull_up(BOTAO_B);
    gpio_pull_up(BOTAO_J);

    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &botao_callback);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &botao_callback);
    gpio_set_irq_enabled_with_callback(BOTAO_J, GPIO_IRQ_EDGE_FALL, true, &botao_callback);
}
