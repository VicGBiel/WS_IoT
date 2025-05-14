#include <stdio.h>               // Biblioteca padrão para entrada e saída
#include <string.h>              // Biblioteca manipular strings
#include <stdlib.h>              // funções para realizar várias operações, incluindo alocação de memória dinâmica (malloc)
#include "pico/stdlib.h"         // Biblioteca da Raspberry Pi Pico para funções padrão (GPIO, temporização, etc.)
#include "hardware/pio.h"        // Biblioteca da Raspberry Pi Pico para PIO
#include "lib/ws2812.pio.h"
#include "pico/cyw43_arch.h"     // Biblioteca para arquitetura Wi-Fi da Pico com CYW43  
#include "lwip/pbuf.h"           // Lightweight IP stack - manipulação de buffers de pacotes de rede
#include "lwip/tcp.h"            // Lightweight IP stack - fornece funções e estruturas para trabalhar com o protocolo TCP
#include "lwip/netif.h"          // Lightweight IP stack - fornece funções e estruturas para trabalhar com interfaces de rede (netif)

// Credenciais WIFI 
#define WIFI_SSID "M34 de Victor"
#define WIFI_PASSWORD "12345678"

// Definição dos pinos dos LEDs
#define led_b 12                 // GPIO12 - LED azul
#define led_g 11                // GPIO11 - LED verde
#define led_r 13                  // GPIO13 - LED vermelho

// Definição de parâmetros para a matriz de LEDS
#define NUM_PIXELS 25 // Número de LEDs na matriz 
#define IS_RGBW false // Define se os LEDs são RGBW ou apenas RGB
#define WS2812_PIN 7 // Pino onde os LEDs WS2812 estão conectados

uint32_t led_buffer[NUM_PIXELS];

// Protótipos
void gpio_setup(void); // Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
void ws2812_setup();
void user_request(char **request); // Tratamento do request do usuário
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err); // Função de callback ao aceitar conexões TCP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err); // Função de callback para processar requisições HTTP
void update_leds(); // Função para atualizar os LEDs
void set_leds(int start, int end, uint32_t color); // Função auxiliar para definir o estado de uma faixa de LEDs

// Função principal
int main()
{
    stdio_init_all();
    gpio_setup();
    ws2812_setup();

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
}

void ws2812_setup(){
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

// Função que processa os requests e altera apenas os LEDs da área desejada
void user_request(char **request) {
    if (strstr(*request, "GET /quarto_on") != NULL) {

        set_leds(13, 16, 0x101010); // LEDs do quarto (13 a 16)
        set_leds(23, 24, 0x101010); // LEDs do quarto (23 e 24)
    } 
    else if (strstr(*request, "GET /quarto_off") != NULL) {
        set_leds(13, 16, 0x000000); // Desligar LEDs do quarto (13 a 16)
        set_leds(23, 24, 0x000000); // Desligar LEDs do quarto (23 e 24)
    } 
    else if (strstr(*request, "GET /sala_on") != NULL) {
        set_leds(10, 12, 0x101010); // LEDs da sala (10 a 12)
        set_leds(17, 22, 0x101010); // LEDs da sala (17 a 22)
    } 
    else if (strstr(*request, "GET /sala_off") != NULL) {
        set_leds(10, 12, 0x000000); // Desligar LEDs da sala (10 a 12)
        set_leds(17, 22, 0x000000); // Desligar LEDs da sala (17 a 22)
    } 
    else if (strstr(*request, "GET /ext_on") != NULL) {
        set_leds(0, 9, 0x101010); // LEDs da área externa (0 a 9)
    } 
    else if (strstr(*request, "GET /ext_off") != NULL) {
        set_leds(0, 9, 0x000000); // Desligar LEDs da área externa (0 a 9)
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

    // Tratamento de request - Controle dos LEDs
    user_request(&request);
    
    // Cria a resposta HTML
    char html[1024];

    // Instruções html do webserver
    snprintf(html, sizeof(html), // Formatar uma string e armazená-la em um buffer de caracteres
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "\r\n"
             "<!DOCTYPE html>\n"
             "<html>\n"
             "<head>\n"
             "<title> AutomaTech </title>\n"
             "<style>\n"
             "body { background-color:rgb(73, 233, 145); font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }\n"
             "h1 { font-size: 64px; margin-bottom: 30px; }\n"
             "button { background-color: rgb(255, 255, 255); font-size: 36px; margin: 10px; padding: 20px 40px; border-radius: 10px; }\n"
             "</style>\n"
             "</head>\n"
             "<body>\n"
             "<h1>AutomaTech</h1>\n"
             "<form action=\"./quarto_on\"><button>Ligar Quarto</button></form>\n"
             "<form action=\"./quarto_off\"><button>Desligar Quarto</button></form>\n"
             "<form action=\"./sala_on\"><button>Ligar Sala</button></form>\n"
             "<form action=\"./sala_off\"><button>Desligar Sala</button></form>\n"
             "<form action=\"./ext_on\"><button>Ligar Área Externa</button></form>\n"
             "<form action=\"./ext_off\"><button>Desligar Área Externa</button></form>\n"
             "</body>\n"
             "</html>\n"
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
