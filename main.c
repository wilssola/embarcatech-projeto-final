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

volatile uint8_t number = MIN_NUMBER;

volatile uint8_t led_color = MIN_LED;

volatile bool both_buttons_pressed = false;

volatile bool led_green_state = false, led_blue_state = false;

uint16_t level_red = 0, level_green = 0, level_blue = 0;

uint slice_num_red, slice_num_green, slice_num_blue;

uint16_t vrx, vry;

uint8_t square_x = (WIDTH - 8) / 2;
uint8_t square_y = (HEIGHT - 8) / 2;

bool led_pwm_enabled = true;

bool border_style = false;
uint8_t border_style_index = 0;

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

void display_waveform(ssd1306_t *ssd, uint16_t mic_value) {
    static uint8_t x = 0;
    static uint8_t prev_y = HEIGHT / 2;

    uint8_t y = HEIGHT / 2 + (mic_value - 2048) / 16;

    ssd1306_line(ssd, x, prev_y, x + 1, y, true);
    ssd1306_send_data(ssd);

    prev_y = y;
    x = (x + 1) % WIDTH;

    if (x == 0) {
        display_clean(ssd);
    }
}

void display_spectrum(ssd1306_t *ssd, uint16_t mic_value) {
    static uint8_t bar_width = WIDTH / 16;
    static uint8_t bar_heights[16] = {0};

    uint8_t bar_index = (mic_value / 256) % 16;
    uint8_t bar_height = (mic_value % 256) / 4;

    bar_heights[bar_index] = bar_height;

    display_clean(ssd);

    for (uint8_t i = 0; i < 16; ++i) {
        ssd1306_rect(ssd, HEIGHT - bar_heights[i], i * bar_width, bar_width - 1, bar_heights[i], true, true);
    }

    ssd1306_send_data(ssd);
}

void display_vu_meter(ssd1306_t *ssd, uint16_t mic_value) {
    uint8_t bar_length = (mic_value * WIDTH) / 4096;

    display_clean(ssd);

    ssd1306_rect(ssd, HEIGHT / 2 - 4, 0, bar_length, 8, true, true);

    ssd1306_send_data(ssd);
}

void display_radar(ssd1306_t *ssd, uint16_t mic_value) {
    display_clean(ssd);

    // Desenha o círculo do radar
    ssd1306_circle(ssd, WIDTH / 2, HEIGHT / 2, HEIGHT / 2 - 1, true);

    // Desenha a linha do radar
    uint8_t angle = ((mic_value * 360) / MIC_LIMIAR_2) % 360;
    uint8_t x = WIDTH / 2 + (HEIGHT / 2 - 1) * cos(angle * M_PI / 180);
    uint8_t y = HEIGHT / 2 + (HEIGHT / 2 - 1) * -sin(angle * M_PI / 180);
    ssd1306_line(ssd, WIDTH / 2, HEIGHT / 2, x, y, true);

    ssd1306_send_data(ssd);
}

void mic_detect(uint16_t mic_value) {
    if (mic_value > MIC_LIMIAR_2) {
        level_red = ((mic_value * 255) / (2 * MIC_LIMIAR_2)) % 255;
        level_blue = 0;
        level_green = 0;
    } else if (mic_value > MIC_LIMIAR_1 && mic_value < MIC_LIMIAR_2) {
        level_red = ((mic_value * 255) / MIC_LIMIAR_2) % 255;
        level_blue = 0;
        level_green = ((mic_value * 255) / MIC_LIMIAR_2) % 255;
    } else {
        level_green = ((mic_value * 255) / MIC_LIMIAR_1) % 255;
        level_red = 0;
        level_blue = 0;
    }
    
    pwm_set_gpio_level(LED_RGB_RED_PIN, level_red);
    pwm_set_gpio_level(LED_RGB_BLUE_PIN, level_blue);
    pwm_set_gpio_level(LED_RGB_GREEN_PIN, level_green);
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

    uint64_t interval = 1000000 / MIC_SAMPLE_RATE;
    uint8_t mode = 0; // 0: Waveform, 1: Spectrum, 2: VU Meter

    while (true) {
        if (stdio_usb_connected()) {
            int c = getchar_timeout_us(0);

            if (c != PICO_ERROR_TIMEOUT) {
                display_clean(&ssd);

                ssd1306_draw_char(&ssd, c, 0, 0);

                if (c >= '0' && c <= '9') {
                    display_symbol(c - '0');
                }
            }
        }

        // Verifica o nível de áudio do microfone
        uint16_t mic_level = read_mic();

        mic_detect(mic_level);

        switch (mode) {
            case 0:
                display_waveform(&ssd, mic_level);
                break;
            case 1:
                display_spectrum(&ssd, mic_level);
                break;
            case 2:
                display_vu_meter(&ssd, mic_level);
                break;
            case 3:
                display_radar(&ssd, mic_level);
                break;
        }

        sleep_ms(interval);
    }

    return 0;
}