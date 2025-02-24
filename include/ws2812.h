#include "ws2812.pio.h"

#ifndef WS2812_H
#define WS2812_H

#define MATRIX_WIDTH 5
#define MATRIX_HEIGHT 5
#define MATRIX_FLIPPED 1 // Defina como 1 se a matriz estiver de cabeça para baixo, 0 caso contrário

uint32_t led_matrix[MATRIX_HEIGHT][MATRIX_WIDTH];

uint32_t adjust_brightness(uint32_t color, float brightness) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    r = (uint8_t)(r * brightness);
    g = (uint8_t)(g * brightness);
    b = (uint8_t)(b * brightness);

    return (r << 16) | (g << 8) | b;
}

// Função para definir a cor de um pixel na matriz de LEDs WS2812
void ws2812_set_pixel(uint8_t x, uint8_t y, uint32_t color) {
    if (x < MATRIX_WIDTH && y < MATRIX_HEIGHT) {
        led_matrix[y][x] = color;
    }
}

// Função para limpar a matriz de LEDs WS2812
void ws2812_clear() {
    for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
        for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
            ws2812_set_pixel(x, y, 0x000000); // Apaga o pixel
        }
    }
}

// Função para atualizar a matriz de LEDs WS2812
void ws2812_draw() {
    float brightness = 0.01; // Ajuste o valor de 0.0 a 1.0 para definir o brilho
    for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
        for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
            // Calcular a posição correta considerando a orientação da matriz
            uint8_t draw_y = MATRIX_FLIPPED ? (MATRIX_HEIGHT - 1 - y) : y;        

            uint32_t color = led_matrix[draw_y][x];
            color = adjust_brightness(color, brightness);

            // Enviar a cor ajustada para o LED correspondente
            pio_sm_put_blocking(pio0, 0, color << 8u);
        }
    }
}

#define MIN_NUMBER 0
#define MAX_NUMBER 9

// Função para exibir o número na matriz de LEDs WS2812
void display_number(uint8_t num) {
    // Cores para cada número de 0 a 9
    // Seguindo a ordem do espectro visível de luz
    uint32_t colors[] = {
        0xFF0000, // Vermelho para 0
        0xFF7F00, // Laranja para 1
        0xFFFF00, // Amarelo para 2
        0x00FF00, // Verde para 3
        0x00FF7F, // Verde claro para 4
        0x00FFFF, // Ciano para 5
        0x007FFF, // Azul claro para 6
        0x0000FF, // Azul para 7
        0x4B0082, // Anil para 8
        0x8B00FF  // Violeta para 9
    };

    // Padrões de LEDs para cada número de 0 a 9
    uint8_t patterns[10][MATRIX_HEIGHT][MATRIX_WIDTH] = {
        // 0
        {
            {1, 1, 1, 1, 1},
            {1, 0, 0, 0, 1},
            {1, 0, 0, 0, 1},
            {1, 0, 0, 0, 1},
            {1, 1, 1, 1, 1}
        },
        // 1
        {
            {0, 0, 1, 0, 0},
            {0, 1, 1, 0, 0},
            {0, 0, 1, 0, 1},
            {0, 0, 1, 0, 0},
            {1, 1, 1, 1, 1}
        },
        // 2
        {
            {1, 1, 1, 1, 1},
            {0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1},
            {1, 0, 0, 0, 0},
            {1, 1, 1, 1, 1}
        },
        // 3
        {
            {1, 1, 1, 1, 1},
            {0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1},
            {0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1}
        },
        // 4
        {
            {1, 0, 0, 0, 1},
            {1, 0, 0, 0, 1},
            {1, 1, 1, 1, 1},
            {0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0}
        },
        // 5
        {
            {1, 1, 1, 1, 1},
            {1, 0, 0, 0, 0},
            {1, 1, 1, 1, 1},
            {0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1}
        },
        // 6
        {
            {1, 1, 1, 1, 1},
            {1, 0, 0, 0, 0},
            {1, 1, 1, 1, 1},
            {1, 0, 0, 0, 1},
            {1, 1, 1, 1, 1}
        },
        // 7
        {
            {1, 1, 1, 1, 1},
            {0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0},
            {0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0}
        },
        // 8
        {
            {1, 1, 1, 1, 1},
            {1, 0, 0, 0, 1},
            {1, 1, 1, 1, 1},
            {1, 0, 0, 0, 1},
            {1, 1, 1, 1, 1}
        },
        // 9
        {
            {1, 1, 1, 1, 1},
            {1, 0, 0, 0, 1},
            {1, 1, 1, 1, 1},
            {0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1}
        }
    };

    uint32_t color = colors[num];

    // Limpa a matriz
    ws2812_clear();

    // Define os LEDs para o número
    for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
        for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
            if (patterns[num][y][x] == 1) {
                ws2812_set_pixel(x, y, color);
            }
        }
    }

    // Atualiza a matriz
    ws2812_draw();
}
#endif