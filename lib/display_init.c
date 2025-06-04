#include "lib/display_init.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

ssd1306_t ssd;
int borda_estado = 0;

// Square center positions
int centro_y = (WIDTH - square_size) / 2;
int centro_x = (HEIGHT - square_size) / 2;

void display() {
    // Initialize I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Initialize display
    ssd1306_init(&ssd, WIDTH, HEIGHT, true, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

void desenhar_borda() {
    switch (borda_estado) {
        case 1:
            ssd1306_rect(&ssd, 3, 3, 122, 60, true, false);
            break;
        case 2:
            ssd1306_rect(&ssd, 3, 3, 122, 60, true, false);
            ssd1306_rect(&ssd, 5, 5, 118, 56, true, false);
            break;
        default:
            break;
    }
}