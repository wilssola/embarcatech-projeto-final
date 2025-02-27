#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "pico/multicore.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

#include "ws2812.pio.h"

#include "include/ssd1306_font.h"

#include "include/const.h"
#include "include/led.h"
#include "include/button.h"
#include "include/ws2812.h"
#include "include/ssd1306_i2c.h"

typedef enum {
    MODE_WAVEFORM = 0,
    MODE_SPECTRUM,
    MODE_VU_METER,
    MODE_RADAR,
    MODE_LENGTH
} display_mode_t;

typedef enum {
    MENU = 0,
    VIEWER,
    ALARM,
    LENGTH
} app_mode_t;

volatile uint8_t number = MIN_NUMBER;

volatile uint8_t led_color = MIN_LED;

volatile bool both_buttons_pressed = false;

volatile bool led_green_state = false, led_blue_state = false;

uint16_t level_red = 0, level_green = 0, level_blue = 0;

uint slice_num_red, slice_num_green, slice_num_blue;

uint16_t vrx, vry;

uint16_t adjusted_volume = 0;

uint8_t square_x = (WIDTH - 8) / 2;
uint8_t square_y = (HEIGHT - 8) / 2;

bool led_pwm_enabled = true;

bool border_style = false;
uint8_t border_style_index = 0;

display_mode_t mode = MODE_WAVEFORM, previous_mode = MODE_WAVEFORM;

app_mode_t app_mode = MENU;

// Função para inicializar o LED RGB
void led_init() {    
    gpio_init(LED_RGB_RED_PIN);
    gpio_set_dir(LED_RGB_RED_PIN, GPIO_OUT);

    gpio_init(LED_RGB_GREEN_PIN);
    gpio_set_dir(LED_RGB_GREEN_PIN, GPIO_OUT);

    gpio_init(LED_RGB_BLUE_PIN);
    gpio_set_dir(LED_RGB_BLUE_PIN, GPIO_OUT);
}

void led_pwm_setup(uint led_pin, uint *slice_num, uint16_t led_level) {
    gpio_set_function(led_pin, GPIO_FUNC_PWM);

    *slice_num = pwm_gpio_to_slice_num(led_pin);

    pwm_set_clkdiv(*slice_num, PWM_DIVIDER);
    pwm_set_wrap(*slice_num, PWM_PERIOD);    

    pwm_set_gpio_level(led_pin, led_level);

    pwm_set_enabled(*slice_num, true);
}

void led_pwm_init() {
    led_pwm_setup(LED_RGB_RED_PIN, &slice_num_red, 0);

    led_pwm_setup(LED_RGB_GREEN_PIN, &slice_num_green, 0);

    led_pwm_setup(LED_RGB_BLUE_PIN, &slice_num_blue, 0);
}

// Função para inicializar os botões
void button_init() {
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);

    // Configura interrupções para os botões
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_button_callback);
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_button_callback);
}

// Função para inicializar o controlador WS2812
void ws2812_init() {
    // Inicializa o controlador WS2812
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);
}

void joystick_init() {
    adc_init();
    adc_gpio_init(JOYSTICK_VRX_PIN);
    adc_gpio_init(JOYSTICK_VRY_PIN);

    gpio_init(JOYSTICK_SW_PIN);
    gpio_set_dir(JOYSTICK_SW_PIN, GPIO_IN);
    gpio_pull_up(JOYSTICK_SW_PIN);

    gpio_set_irq_enabled_with_callback(JOYSTICK_SW_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_button_callback);
}

void buzzer_init() {
    gpio_init(BUZZER_A_PIN);
    gpio_set_dir(BUZZER_A_PIN, GPIO_OUT);
    gpio_put(BUZZER_A_PIN, false);

    gpio_init(BUZZER_B_PIN);
    gpio_set_dir(BUZZER_B_PIN, GPIO_OUT);
    gpio_put(BUZZER_B_PIN, false);
}

void core1_entry() {
    blink_led();
}

void switch_led() {
    if (led_color < MAX_LED) {
        led_color++;
    } else {
        led_color = MIN_LED;
    }
    
    // Reiniciar o núcleo 1 para atualizar a cor do LED
    multicore_reset_core1();

    if (!both_buttons_pressed) {
        // Iniciar o núcleo 1 para piscar o LED com a nova cor
        multicore_launch_core1(core1_entry);
    } else {
        // Desligar os LED RGB
        off_led();
    }

    both_buttons_pressed = !both_buttons_pressed;
}

// Função para verificar se ambos os botões foram pressionados
void check_both_buttons() {   
    if (button_a_pressed && button_b_pressed) {
        button_a_pressed = false;
        button_b_pressed = false;
        
        number = 0;

        switch_led();

        ws2812_clear();
        ws2812_draw();
    }

    sleep_ms(20);
}

// Função para exibir o símbolo na matriz WS2812
void display_symbol(uint8_t number) {
    ws2812_clear();

    display_number(number);

    printf("Número %d\n", number);
}

// Função para limpar o display
void display_clean(ssd1306_t *ssd) {
    ssd1306_fill(ssd, false);
    ssd1306_send_data(ssd);
}

void toggle_green_led(ssd1306_t *ssd) {
    gpio_put(LED_RGB_BLUE_PIN, 0);

    led_green_state = !led_green_state;
    gpio_put(LED_RGB_GREEN_PIN, led_green_state);

    char message[32];
    snprintf(message, sizeof(message), "LED Verde %s", led_green_state ? "Ligado" : "Desligado");

    display_clean(ssd);

    ssd1306_draw_string(ssd, message, 0, 0);
    ssd1306_send_data(ssd);

    printf("%s\n", message);
}

void toggle_blue_led(ssd1306_t *ssd) {            
    gpio_put(LED_RGB_GREEN_PIN, 0);

    led_blue_state = !led_blue_state;
    gpio_put(LED_RGB_BLUE_PIN, led_blue_state);

    char message[32];
    snprintf(message, sizeof(message), "LED Azul %s", led_blue_state ? "Ligado" : "Desligado");

    display_clean(ssd);

    ssd1306_draw_string(ssd, message, 0, 0);
    ssd1306_send_data(ssd);

    printf("%s\n", message);
}

void read_joystick(uint16_t *vrx, uint16_t *vry) {
    adc_select_input(ADC_CHANNEL_1);
    sleep_us(1);
    *vrx = adc_read();
    
    adc_select_input(ADC_CHANNEL_0);
    sleep_us(1);
    *vry = adc_read();

    // Atualiza o brilho do LED com base na posição do joystick
    if (led_pwm_enabled) {
        uint16_t red_level = abs((int16_t)(*vrx) - 2048) * 2;
        uint16_t blue_level = abs((int16_t)(*vry) - 2048) * 2;

        // Apaga o LED se o joystick estiver na posição central
        if (abs((int16_t)(*vrx) - 2048) < 128) {
            red_level = 0;
        }
        if (abs((int16_t)(*vry) - 2048) < 128) {
            blue_level = 0;
        }

        pwm_set_gpio_level(LED_RGB_RED_PIN, red_level);
        pwm_set_gpio_level(LED_RGB_BLUE_PIN, blue_level);
    }
}

void draw_border(ssd1306_t *ssd) {
    if (border_style) {
        switch (border_style_index) {
            case 1:
                ssd1306_rect(ssd, 0, 0, WIDTH, HEIGHT, true, false);
                break;
            case 0:
                ssd1306_rect(ssd, 32, 64, WIDTH, HEIGHT, true, false);
                break;
        }
    } else {
        ssd1306_rect(ssd, 0, 0, WIDTH, HEIGHT, false, false);
    }
}

// Função para inicializar o microfone
void mic_init() {
    adc_init();
    adc_gpio_init(MIC_PIN);
}

// Função para ler o nível de áudio do microfone
uint16_t read_mic() {    
    adc_select_input(ADC_CHANNEL_2);
    return adc_read();
}

void display_waveform(ssd1306_t *ssd, uint16_t mic_level) {
    if (previous_mode != MODE_WAVEFORM) {
        display_clean(ssd);
        previous_mode = MODE_WAVEFORM;
    }
    
    static uint8_t x = 0;
    static uint8_t prev_y = HEIGHT / 2;

    uint8_t y = HEIGHT / 2 + (mic_level - 2048) / 16;

    ssd1306_line(ssd, x, prev_y, x + 1, y, true);
    ssd1306_send_data(ssd);

    prev_y = y;
    x = (x + 1) % WIDTH;

    if (x == 0) {
        display_clean(ssd);
    }
}

void display_spectrum(ssd1306_t *ssd, uint16_t mic_level) {
    static uint8_t bar_width = WIDTH / 16;
    static uint8_t bar_heights[16] = {0};
    static uint8_t mic_levels[16] = {0};
    static uint8_t index = 0;

    // Adiciona o valor do microfone ao buffer circular
    mic_levels[index] = mic_level;
    index = (index + 1) % 16;

    // Calcula a altura das barras com base nos valores do microfone
    for (uint8_t i = 0; i < 16; ++i) {
        uint8_t value = mic_levels[(index + i) % 16];
        bar_heights[i] = (value % 256) / 4;
    }

    display_clean(ssd);

    // Desenha as barras no display
    for (uint8_t i = 0; i < 16; ++i) {
        ssd1306_rect(ssd, HEIGHT - bar_heights[i], i * bar_width, bar_width - 1, bar_heights[i], true, true);
    }

    ssd1306_send_data(ssd);
}

void display_vu_meter(ssd1306_t *ssd, uint16_t mic_level) {
    display_clean(ssd);

    uint8_t bar_length = (mic_level * WIDTH) / MIC_LIMIAR_2;

    ssd1306_rect(ssd, HEIGHT / 2 - 4, 0, bar_length, 8, true, true);

    ssd1306_send_data(ssd);
}

void display_radar(ssd1306_t *ssd, uint16_t mic_level) {
    display_clean(ssd);

    // Desenha o círculo do radar
    ssd1306_circle(ssd, WIDTH / 2, HEIGHT / 2, HEIGHT / 2 - 1, true);

    // Desenha a linha do radar
    uint8_t angle = ((mic_level * 360) / MIC_LIMIAR_2) % 360;
    uint8_t x = WIDTH / 2 + (HEIGHT / 2 - 1) * cos(angle * M_PI / 180);
    uint8_t y = HEIGHT / 2 + (HEIGHT / 2 - 1) * -sin(angle * M_PI / 180);
    ssd1306_line(ssd, WIDTH / 2, HEIGHT / 2, x, y, true);

    ssd1306_send_data(ssd);
}

// Função para exibir o menu
void display_menu(ssd1306_t *ssd) {
    display_clean(ssd);
    ssd1306_draw_string(ssd, "BitSound", 0, 0);
    ssd1306_draw_string(ssd, "A Visualizar", 0, 16);
    ssd1306_draw_string(ssd, "B Armar Alarme", 0, 32);
    ssd1306_send_data(ssd);
}

void display_alarm_message(ssd1306_t *ssd, uint16_t mic_level) {
    display_clean(ssd);
    ssd1306_draw_string(ssd, "Alarme Disparado", 0, 0);
    
    char mic_level_str[16];
    snprintf(mic_level_str, sizeof(mic_level_str), "%u", mic_level);
    ssd1306_draw_string(ssd, mic_level_str, 0, 16);
    
    ssd1306_send_data(ssd);
}

void display_alarm_activated_message(ssd1306_t *ssd, uint16_t mic_level) {
    display_clean(ssd);
    ssd1306_draw_string(ssd, "Alarme Armado", 0, 0);
    
    char mic_level_str[16];
    snprintf(mic_level_str, sizeof(mic_level_str), "%u", mic_level);
    ssd1306_draw_string(ssd, mic_level_str, 0, 16);
    
    ssd1306_send_data(ssd);
}

void play_sound(uint16_t mic_level, uint16_t volume) {
    // Define a frequência do som com base no valor do microfone
    uint16_t frequency = (mic_level * 1024) / MIC_LIMIAR_2;

    // Ajusta o volume do som
    if (volume < 128) { // Verifica se o joystick está na posição inferior
        adjusted_volume = 0;
    } else {
        adjusted_volume = (volume * 255) / 4096;
    }

    // Ativa os buzzers com o volume ajustado
    gpio_put(BUZZER_A_PIN, true);
    gpio_put(BUZZER_B_PIN, true);

    // Aguarda um curto período de tempo
    sleep_us(1000000 / frequency / 2 * adjusted_volume);

    // Desativa os buzzers
    gpio_put(BUZZER_A_PIN, false);
    gpio_put(BUZZER_B_PIN, false);
}

void update_led_rgb(uint16_t mic_level) {
    if (mic_level > MIC_LIMIAR_2) {
        level_red = ((mic_level * 255) / (2 * MIC_LIMIAR_2)) % 255;
        level_blue = 0;
        level_green = 0;
    } else if (mic_level > MIC_LIMIAR_1 && mic_level < MIC_LIMIAR_2) {
        level_red = ((mic_level * 255) / MIC_LIMIAR_2) % 255;
        level_blue = 0;
        level_green = ((mic_level * 255) / MIC_LIMIAR_2) % 255;
    } else {
        level_green = ((mic_level * 255) / MIC_LIMIAR_1) % 255;
        level_red = 0;
        level_blue = 0;
    }
    
    pwm_set_gpio_level(LED_RGB_RED_PIN, level_red);
    pwm_set_gpio_level(LED_RGB_BLUE_PIN, level_blue);
    pwm_set_gpio_level(LED_RGB_GREEN_PIN, level_green);
}

void update_led_matrix_progressive(uint16_t mic_level) {
    // Limpa a matriz de LEDs
    ws2812_clear();

    // Calcula o número de LEDs a serem acesos com base no nível do microfone
    uint8_t num_leds = (mic_level * 25) / MIC_LIMIAR_2;
    if (num_leds > 25) {
        num_leds = 25;
    }

    // Acende os LEDs progressivamente com cores diferentes
    for (uint8_t i = 0; i < num_leds; i++) {
        uint32_t color;
        if (i < 8) {
            color = 0xFF0000; // Vermelho
        } else if (i < 17) {
            color = 0xFFFF00; // Amarelo
        } else {
            color = 0x00FF00; // Verde
        }

        uint8_t x = i % MATRIX_WIDTH;
        uint8_t y = MATRIX_HEIGHT - 1 - (i / MATRIX_WIDTH); // Alteração para acender de baixo para cima
        ws2812_set_pixel(x, y, color);
    }

    // Atualiza a matriz de LEDs
    ws2812_draw();
}

void activate_alarm() {
    while (true) {
        gpio_put(BUZZER_A_PIN, true);
        gpio_put(BUZZER_B_PIN, true);
        sleep_ms(100);
        gpio_put(BUZZER_A_PIN, false);
        gpio_put(BUZZER_B_PIN, false);
        sleep_ms(100);

        if (button_b_pressed) {
            button_b_pressed = false;
            break;
        }
    }
}

void deactivate_alarm() {
    gpio_put(BUZZER_A_PIN, false);
    gpio_put(BUZZER_B_PIN, false);
}

int main() {
    stdio_init_all();

    // Inicializa os LEDs RGB
    led_init();

    // Inicializa os LEDs RGB usando PWM
    led_pwm_init();

    // Inicializa os pinos dos botões com pull-up
    button_init();

    // Inicializa o controlador WS2812
    ws2812_init();

    // Inicializa o joystick
    joystick_init();

    // Inicializa o microfone
    mic_init();
    
    // Inicializa os buzzers
    buzzer_init();

    // Inicializa o I2C
    i2c_init(I2C_PORT, I2C_BAUDRATE);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // Inicializa e configura o display SSD1306
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, I2C_ADDR, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    display_clean(&ssd);
    
    ws2812_clear();
    ws2812_draw();

    uint64_t interval = 1000000 / MIC_SAMPLE_RATE;

    while (true) {
        // Obter o nível de áudio do microfone
        uint16_t mic_level = read_mic();

        switch (app_mode) {
            case MENU:            
                display_menu(&ssd);
                if (button_a_pressed) {
                    display_clean(&ssd);

                    app_mode = VIEWER;

                    button_a_pressed = false;

                    continue;
                } 
                
                if (button_b_pressed) {
                    display_clean(&ssd);

                    app_mode = ALARM;

                    button_b_pressed = false;

                    continue;
                }
                break;
            case VIEWER:
                if (button_a_pressed) {
                    previous_mode = mode;
                    mode = (mode + 1) % MODE_LENGTH;

                    button_a_pressed = false;

                    continue;
                }

                if (joystick_pressed) {
                    ws2812_clear();
                    ws2812_draw();

                    display_clean(&ssd);

                    app_mode = MENU;
                    joystick_pressed = false;

                    continue;
                }

                update_led_rgb(mic_level);

                // Atualiza a matriz de LEDs WS2812 com base no nível do microfone
                update_led_matrix_progressive(mic_level);

                // Lê o valor do eixo Y do joystick para ajustar o volume
                uint16_t vrx, vry;
                read_joystick(&vrx, &vry);

                // Chamada para a função play_sound
                play_sound(mic_level, vry);

                switch (mode) {
                    case MODE_WAVEFORM:
                        display_waveform(&ssd, mic_level);
                        break;
                    case MODE_SPECTRUM:
                        display_spectrum(&ssd, mic_level);
                        break;
                    case MODE_VU_METER:
                        display_vu_meter(&ssd, mic_level);
                        break;
                    case MODE_RADAR:
                        display_radar(&ssd, mic_level);
                        break;
                }

                sleep_us(interval);
                break;
            case ALARM:
                if (joystick_pressed) {
                    ws2812_clear();
                    ws2812_draw();

                    display_clean(&ssd);

                    deactivate_alarm();

                    app_mode = MENU;
                    joystick_pressed = false;

                    continue;
                }

                if (mic_level > MIC_LIMIAR_2) {
                    display_alarm_message(&ssd, mic_level);
                    activate_alarm();
                } else {
                    display_alarm_activated_message(&ssd, mic_level);
                    deactivate_alarm();
                }

                sleep_us(interval);
                break;
        }
    }

    return 0;
}