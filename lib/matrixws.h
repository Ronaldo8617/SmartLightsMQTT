#ifndef MATRIXWS_H
#define MATRIXWS_H

#include "pico/stdlib.h"

// Definição de constantes
#define PINO_MATRIZ 7       // Pino de controle da matriz de LEDs
#define NUM_LEDS 25         // Número total de LEDs na matriz
#define BRILHO_MAX 255      // Brilho máximo (0-255)
#define BRILHO_PADRAO 30    // Brilho padrão (30/255 ≈ 12%)

// Definição da estrutura de cor para cada LED
typedef struct {
    uint8_t R, G, B;       // Componentes de cor: vermelho, verde e azul
} npLED_t;

extern npLED_t leds[NUM_LEDS];  // Vetor que armazena as cores de todos os LEDs
extern uint8_t brilho_global;   // Variável global para controle de brilho

// Funções
void controle(uint pino);       // Função de controle inicial da matriz de LEDs
void bf();                     // Função para atualizar os LEDs da matriz
void cores(const uint indice, const uint8_t r, const uint8_t g, const uint8_t b);  // Função para configurar a cor de um LED
void desliga();                // Função para desligar todos os LEDs
void desenhaMatriz(int mat[5][5][3]);  // Função para desenhar a matriz de LEDs
void set_brilho(uint8_t brilho);       // Função para ajustar o brilho global
void sequencia_rgb();          // Função para testar sequência RGB


int getIndex(int x, int y);    

#endif
