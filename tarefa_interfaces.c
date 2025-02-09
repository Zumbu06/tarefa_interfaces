#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "ssd1306.h"
#include "ws2812b.h"

// Definição dos pinos
#define LED_VERMELHO 13
#define LED_AZUL 12
#define LED_VERDE 11
#define BOTAO_A 5
#define BOTAO_B 6
#define WS2812B_PIN 7
#define I2C_SDA 14
#define I2C_SCL 15
#define UART_TX 0
#define UART_RX 1

// Variáveis globais
volatile bool estado_led_verde = false;
volatile bool estado_led_azul = false;
volatile uint32_t ultimo_tempo_a = 0;
volatile uint32_t ultimo_tempo_b = 0;
const uint32_t debounce_time = 200;

// Protótipos das funções
void configurar();
void alternar_led_verde();
void alternar_led_azul();
void botao_a_callback(uint gpio, uint32_t events);
void botao_b_callback(uint gpio, uint32_t events);
void processar_uart();
void mostrar_numero_matriz(uint8_t numero);

int main() {
    configurar();
    while (1) {
        processar_uart();
        tight_loop_contents();
    }
}

void configurar() {
    stdio_init_all();
    
    // Configuração dos LEDs
    gpio_init(LED_VERMELHO);
    gpio_init(LED_VERDE);
    gpio_init(LED_AZUL);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_set_dir(LED_AZUL, GPIO_OUT);
    
    // Configuração dos botões
    gpio_init(BOTAO_A);
    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_A);
    gpio_pull_up(BOTAO_B);
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &botao_a_callback);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &botao_b_callback);
    
    // Configuração da UART
    uart_init(uart0, 115200);
    gpio_set_function(UART_TX, GPIO_FUNC_UART);
    gpio_set_function(UART_RX, GPIO_FUNC_UART);
    
    // Configuração do I2C para o display SSD1306
    i2c_init(i2c0, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    // Inicialização do display SSD1306
    ssd1306_init();
    ssd1306_clear();
    ssd1306_write_string("Sistema Iniciado", 0, 0);
    
    // Inicialização da matriz de LEDs WS2812B
    ws2812b_init(WS2812B_PIN);
}

void alternar_led_verde() {
    estado_led_verde = !estado_led_verde;
    gpio_put(LED_VERDE, estado_led_verde);
    printf("LED Verde: %s\n", estado_led_verde ? "ON" : "OFF");
    ssd1306_clear();
    ssd1306_write_string(estado_led_verde ? "LED Verde ON" : "LED Verde OFF", 0, 0);
}

void alternar_led_azul() {
    estado_led_azul = !estado_led_azul;
    gpio_put(LED_AZUL, estado_led_azul);
    printf("LED Azul: %s\n", estado_led_azul ? "ON" : "OFF");
    ssd1306_clear();
    ssd1306_write_string(estado_led_azul ? "LED Azul ON" : "LED Azul OFF", 0, 0);
}

void botao_a_callback(uint gpio, uint32_t events) {
    uint32_t agora = to_ms_since_boot(get_absolute_time());
    if (agora - ultimo_tempo_a > debounce_time) {
        alternar_led_verde();
        ultimo_tempo_a = agora;
    }
}

void botao_b_callback(uint gpio, uint32_t events) {
    uint32_t agora = to_ms_since_boot(get_absolute_time());
    if (agora - ultimo_tempo_b > debounce_time) {
        alternar_led_azul();
        ultimo_tempo_b = agora;
    }
}

void processar_uart() {
    if (uart_is_readable(uart0)) {
        char recebido = uart_getc(uart0);
        ssd1306_clear();
        char str[2] = {recebido, '\0'};
        ssd1306_write_string(str, 0, 0);
        printf("Recebido: %c\n", recebido);
        if (recebido >= '0' && recebido <= '9') {
            mostrar_numero_matriz(recebido - '0');
        }
    }
}

void mostrar_numero_matriz(uint8_t numero) {
    static const uint32_t simbolos[10] = {
        0b01110100011001110101, // 0
        0b00100101110100101110, // 1
        0b01110000011100001111, // 2
        0b01110100011100101110, // 3
        0b00011011111100100010, // 4
        0b11110100011100101110, // 5
        0b01110100011110101110, // 6
        0b01110000100010001000, // 7
        0b01110100011110101110, // 8
        0b01110100011100101110  // 9
    };
    ws2812b_clear();
    for (int i = 0; i < 25; i++) {
        if (simbolos[numero] & (1 << i)) {
            ws2812b_set_pixel(i, 255, 255, 255);
        }
    }
    ws2812b_show();
}