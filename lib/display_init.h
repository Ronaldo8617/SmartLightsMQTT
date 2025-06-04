#ifndef DISPLAY_INIT_H
#define DISPLAY_INIT_H

#include "hardware/i2c.h"
#include "lib/ssd1306.h"

// Global display object
extern ssd1306_t ssd;

// Display configuration
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C

// Square configuration
#define square_size 8
extern int centro_y;
extern int centro_x;

extern int borda_estado;

// Display functions
void display();
void desenhar_borda();

#endif // DISPLAY_H