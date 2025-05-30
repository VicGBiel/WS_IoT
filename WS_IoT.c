// Bibliotecas Utilizadas
#include <stdio.h>               
#include <string.h>           
#include <stdlib.h>        
#include "pico/stdlib.h"     
#include "pico/bootrom.h"  
#include "hardware/pio.h"   
#include "hardware/pwm.h"      
#include "hardware/adc.h"    
#include "lib/ws2812.pio.h"
#include "pico/cyw43_arch.h"    
#include "lwip/pbuf.h"       
#include "lwip/tcp.h"           
#include "lwip/netif.h"          

// Credenciais WIFI 
#define WIFI_SSID "bythesword [2.4GHz]"
#define WIFI_PASSWORD "30317512"

// Definição dos pinos dos LEDs
#define led_b 12                 
#define led_g 11              
#define led_r 13               

// Definição de parâmetros para a matriz de LEDS
#define NUM_PIXELS 25 // Número de LEDs na matriz 
#define IS_RGBW false // Define se os LEDs são RGBW ou apenas RGB
#define WS2812_PIN 7 // Pino onde os LEDs WS2812 estão conectados

// Definição de botões
#define btn_b 6

// Definição do buzzer
#define buzzer_pin_l 10

// Variáveis Globais
static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)
uint32_t cor_quarto = 0x101010;  // Cor padrão (branco)
uint32_t cor_sala = 0x101010;
uint32_t cor_ext = 0x101010;
uint32_t led_buffer[NUM_PIXELS];
bool estado_quarto = false;
bool estado_sala = false;
bool estado_ext = false;
bool estado_cortina = false;
uint slice_buz;

// Protótipos
void gpio_setup(void); // Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
void ws2812_setup(); // Configura a matriz de LEDs
void buzzer_setup(); // Configura os buzzer via pwm
void gpio_irq_handler(uint gpio, uint32_t events); // Tratamento de interrupções
void user_request(char **request); // Tratamento do request do usuário
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err); // Função de callback ao aceitar conexões TCP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err); // Função de callback para processar requisições HTTP
void update_leds(); // Função para atualizar os LEDs
void set_leds(int start, int end, uint32_t color); // Função auxiliar para definir o estado de uma faixa de LEDs
float temp_read(void); // Lê a temperatura interna do dispositivo

// Função principal
int main()
{
    stdio_init_all();
    gpio_setup();
    ws2812_setup();
    buzzer_setup();

    adc_init();
    adc_set_temp_sensor_enabled(true);

    // Configuração de interrupção
    gpio_set_irq_enabled_with_callback(btn_b, GPIO_IRQ_EDGE_FALL,true, &gpio_irq_handler);    

    //Inicializa a arquitetura do cyw43
    while (cyw43_arch_init())
    {
        printf("Falha ao inicializar Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    printf("Conectando ao Wi-Fi...\n");
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK , 30000))
    {
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
    }
    printf("Conectado ao Wi-Fi\n");

    if (netif_default)
    {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }
    
    // Configura o servidor TCP - cria novos PCBs TCP. É o primeiro passo para estabelecer uma conexão TCP.
    struct tcp_pcb *server = tcp_new();
    if (!server)
    {
        printf("Falha ao criar servidor TCP\n");
        return -1;
    }

    //vincula um PCB (Protocol Control Block) TCP a um endereço IP e porta específicos.
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1;
    }

    // Coloca um PCB (Protocol Control Block) TCP em modo de escuta, permitindo que ele aceite conexões de entrada.
    server = tcp_listen(server);

    // Define uma função de callback para aceitar conexões TCP de entrada. É um passo importante na configuração de servidores TCP.
    tcp_accept(server, tcp_server_accept);
    printf("Servidor ouvindo na porta 80\n");

    while (true)
    {
        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        sleep_ms(100);     
    }

    //Desligar a arquitetura CYW43.
    cyw43_arch_deinit();
    return 0;
}

void gpio_setup(void){
    // Configuração dos LEDs como saída
    gpio_init(led_b);
    gpio_set_dir(led_b, GPIO_OUT);
    gpio_put(led_b, false);
    
    gpio_init(led_g);
    gpio_set_dir(led_g, GPIO_OUT);
    gpio_put(led_g, false);
    
    gpio_init(led_r);
    gpio_set_dir(led_r, GPIO_OUT);
    gpio_put(led_r, false);

    // Para ser utilizado o modo BOOTSEL com botão B
    gpio_init(btn_b);
    gpio_set_dir(btn_b, GPIO_IN);
    gpio_pull_up(btn_b);

}

void ws2812_setup(){
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
}

void buzzer_setup(){
    gpio_set_function(buzzer_pin_l, GPIO_FUNC_PWM);
    slice_buz = pwm_gpio_to_slice_num(buzzer_pin_l);
    pwm_set_clkdiv(slice_buz, 40);
    pwm_set_wrap(slice_buz, 12500);
    pwm_set_enabled(slice_buz, true);  
}

void gpio_irq_handler(uint gpio, uint32_t events){
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    if(current_time - last_time > 200000){
        last_time = current_time;
        
        if (gpio == btn_b){
            reset_usb_boot(0,0);
        }
    }

}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

void user_request(char **request) {
    if (strstr(*request, "GET /set_color_quarto") != NULL) {
        estado_quarto = !estado_quarto;
        char *color_param = strstr(*request, "cor=%23"); // %23 é '#'
        if (color_param) {
            unsigned int r, g, b;
            if (sscanf(color_param, "cor=%%23%02x%02x%02x", &g, &r, &b) == 3) {
                cor_quarto = (r << 16) | (g << 8) | b;
                set_leds(13, 16, estado_quarto ? cor_quarto : 0x000000); // LEDs do quarto
                set_leds(23, 24, estado_quarto ? cor_quarto : 0x000000);
            }
        }
    }
    else if (strstr(*request, "GET /set_color_sala") != NULL) {
        estado_sala = !estado_sala;
        char *color_param = strstr(*request, "cor=%23");
        if (color_param) {
            unsigned int r, g, b;
            if (sscanf(color_param, "cor=%%23%02x%02x%02x", &g, &r, &b) == 3) {
                cor_sala = (r << 16) | (g << 8) | b;
                set_leds(10, 12, estado_sala ? cor_sala : 0x000000);
                set_leds(17, 22, estado_sala ? cor_sala : 0x000000);
            }
        }             
    }
    else if (strstr(*request, "GET /set_color_ext") != NULL) {
        estado_ext = !estado_ext;
        char *color_param = strstr(*request, "cor=%23");
        if (color_param) {
            unsigned int r, g, b;
            if (sscanf(color_param, "cor=%%23%02x%02x%02x", &g, &r, &b) == 3) {
                cor_ext = (r << 16) | (g << 8) | b;
                set_leds(0, 9, estado_ext ? cor_ext : 0x000000);
                set_leds(0, 9, estado_ext ? cor_ext : 0x000000);
            }
        }
    }
    else if (strstr(*request, "GET /toggle_cur") != NULL) {
        estado_cortina = !estado_cortina;
        pwm_set_gpio_level(buzzer_pin_l, 60);
        sleep_ms(1000);
        pwm_set_gpio_level(buzzer_pin_l, 0);
    }

    
    update_leds(); // Atualiza todos os LEDs de acordo com o buffer atualizado
}

static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    // Alocação do request na memória dinámica
    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);

    // Leitura da temperatura
    float fTemp = temp_read();
    printf ("Temperatura interna: %.2f °C", fTemp);
    
    // Tratamento dos requests
    user_request(&request);
    
    // Cria a resposta HTML
    char html[1024];

    // Instruções html do webserver
    snprintf(html, sizeof(html),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "\r\n"
        "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
        "<title>AutomaTech</title>"
        "<style>"
        "body{background-color:rgb(41,21,132);font-family:Arial,sans-serif;color:white;text-align:center;margin-top:20px;}"
        "h1{font-size:44px;margin-bottom:15px;}"
        "h2{font-size:24px;margin-bottom:5px;}"
        "button{background-color:white;font-size:18px;margin:10px;padding:15px;border-radius:10px;}"
        "</style></head><body>"
        "<h1>AutomaTech</h1>"
        "<h2>Iluminação</h2>"
        "<form action=\"./set_color_quarto\" method=\"get\">"
        "<input type=\"color\" name=\"cor\" value=\"#101010\">"
        "<button type=\"submit\">Quarto ON/OFF</button>"
        "</form>"
        "<form action=\"./set_color_sala\" method=\"get\">"
        "<input type=\"color\" name=\"cor\" value=\"#101010\">"
        "<button type=\"submit\">Sala ON/OFF</button>"
        "</form>"
        "<form action=\"./set_color_ext\" method=\"get\">"
        "<input type=\"color\" name=\"cor\" value=\"#101010\">"
        "<button type=\"submit\">Área externa ON/OFF</button>"
        "</form>"
        "<h2>Cortinas</h2>"
        "<form action=\"./toggle_cur\">"
        "<button>Cortina ON/OFF</button>"
        "</form>"
        "</body></html>"
    );


    // Escreve dados para envio (mas não os envia imediatamente).
    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);

    // Envia a mensagem
    tcp_output(tpcb);

    //libera memória alocada dinamicamente
    free(request);
    
    pbuf_free(p);

    return ERR_OK;
}

void update_leds() {
    for (int i = 0; i < NUM_PIXELS; i++) {
        pio_sm_put_blocking(pio0, 0, led_buffer[i] << 8u);
    }
}

void set_leds(int start, int end, uint32_t color) {
    for (int i = start; i <= end; i++) {
        led_buffer[i] = color;
    }
}

float temp_read(void){
    adc_select_input(4);
    uint16_t raw_value = adc_read();
    const float conversion_factor = 3.3f / (1 << 12);
    float temperature = 27.0f - ((raw_value * conversion_factor) - 0.706f) / 0.001721f;
        return temperature;
}

