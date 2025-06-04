#include "rgb.h"

// Inicializar os pinos do LED RGB
void iniciar_rgb() {
  gpio_init(red);
  gpio_init(green);
  gpio_init(blue);

  gpio_set_dir(red, GPIO_OUT);
  gpio_set_dir(green, GPIO_OUT);
  gpio_set_dir(blue, GPIO_OUT);
}

// Função para iniciar os LEDs RGB dos pinos 11, 12 e 13 e configurar o tempo ligado
void state(bool rr, bool gg, bool bb) {
  iniciar_rgb();
  gpio_put(red, rr);
  gpio_put(green, gg);
  gpio_put(blue, bb);
} 