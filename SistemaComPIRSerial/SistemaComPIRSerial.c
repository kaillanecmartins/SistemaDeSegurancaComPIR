#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include <string.h>

#define PIR 18
#define RED 13
int cont = 0;
#define BUZZER1_PIN 10  // Pino do primeiro buzzer
#define BUZZER2_PIN 21  // Pino do segundo buzzer
#define NOTE_C5 523     // Frequência da nota C5 em Hz
#define WIFI_SSID "Nome"  // Substitua pelo nome da sua rede Wi-Fi
#define WIFI_PASS "Senha" // Substitua pela senha da sua rede Wi-Fi

// Estado dos botões (inicialmente sem mensagens)
char PIR_stats[50] = "Nenhum evento";
//char button2_message[50] = "Nenhum evento no botão 2";

// Buffer para resposta HTTP
char http_response[1024];

// Função para criar a resposta HTTP
void create_http_response() {
    snprintf(http_response, sizeof(http_response),
             "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
             "<!DOCTYPE html>"
             "<html>"
             "<head>"
             "  <meta charset=\"UTF-8\">"
             "  <title>Controle do LED e Botões</title>"
             "</head>"
             "<body>"
             "  <h1>Controle do LED e Botões</h1>"
             "  <p><a href=\"/led/on\">Ligar LED</a></p>"
             "  <p><a href=\"/led/off\">Desligar LED</a></p>"
             "  <p><a href=\"/update\">Atualizar Estado</a></p>"
             "  <h2>Estado do Sensor:</h2>"
             "  <p>Status: %s</p>"
             "</body>"
             "</html>\r\n",
             PIR_stats);
}




// Função de callback para processar requisições HTTP
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        // Cliente fechou a conexão
        tcp_close(tpcb);
        return ERR_OK;
    }

    // Processa a requisição HTTP
    char *request = (char *)p->payload;

    if (strstr(request, "GET /led/on")) {
        gpio_put(RED, 1);  // Liga o LED
    } else if (strstr(request, "GET /led/off")) {
        gpio_put(RED, 0);  // Desliga o LED
    }

    // Atualiza o conteúdo da página com base no estado dos botões
    create_http_response();

    // Envia a resposta HTTP
    tcp_write(tpcb, http_response, strlen(http_response), TCP_WRITE_FLAG_COPY);

    // Libera o buffer recebido
    pbuf_free(p);

    return ERR_OK;
}

// Callback de conexão: associa o http_callback à conexão
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_callback);  // Associa o callback HTTP
    return ERR_OK;
}

// Função de setup do servidor TCP
static void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB\n");
        return;
    }

    // Liga o servidor na porta 80
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }

    pcb = tcp_listen(pcb);  // Coloca o PCB em modo de escuta
    tcp_accept(pcb, connection_callback);  // Associa o callback de conexão

    printf("Servidor HTTP rodando na porta 80...\n");
}

void play_note(uint buzzer_pin, uint frequency) {
    gpio_set_function(buzzer_pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(buzzer_pin);
    
    uint clkdiv = 4; // Define um divisor de clock
    uint wrap = 125000000 / (clkdiv * frequency);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_clkdiv(slice_num, clkdiv);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(buzzer_pin), wrap / 2);
    pwm_set_enabled(slice_num, true);
}

void stop_buzzer(uint buzzer_pin) {
    uint slice_num = pwm_gpio_to_slice_num(buzzer_pin);
    pwm_set_enabled(slice_num, false);
}

int main()
{
    stdio_init_all();
    gpio_init(PIR);
    gpio_set_dir(PIR, GPIO_IN);
    
    gpio_init(BUZZER1_PIN);
    gpio_set_dir(BUZZER1_PIN, GPIO_OUT);
    gpio_put(BUZZER1_PIN, 0);
    
    gpio_init(BUZZER2_PIN);
    gpio_set_dir(BUZZER2_PIN, GPIO_OUT);
    gpio_put(BUZZER2_PIN, 0);

    gpio_init(RED);
    gpio_set_dir(RED, GPIO_OUT);
    gpio_put(RED, 0);

    stdio_init_all();  // Inicializa a saída padrão
    sleep_ms(10000);
    printf("Iniciando servidor HTTP\n");

    // Inicializa o Wi-Fi
    if (cyw43_arch_init()) {
        printf("Erro ao inicializar o Wi-Fi\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    printf("Conectando ao Wi-Fi...\n");

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        return 1;
    }else {
        printf("Connected.\n");
        // Read the ip address in a human readable way
        uint8_t *ip_address = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
        printf("Endereço IP %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    }

    printf("Wi-Fi conectado!\n");

    //printf("Botões configurados com pull-up nos pinos %d e %d\n", BUTTON1_PIN, BUTTON2_PIN);

    // Inicia o servidor HTTP
    start_http_server();

    while (true) {
        cyw43_arch_poll();  // Necessário para manter o Wi-Fi ativo
        if(gpio_get(PIR) == 1){
              //printf("1");
              for (int i = 0; i < 10; i++) {
                  gpio_put(RED, 1);
                  play_note(BUZZER1_PIN, NOTE_C5);
                  play_note(BUZZER2_PIN, NOTE_C5);
                  snprintf(PIR_stats, sizeof(PIR_stats), "ALGO DETECTADO!");
                  sleep_ms(100);
                  gpio_put(RED, 0);
                  stop_buzzer(BUZZER1_PIN);
                  stop_buzzer(BUZZER2_PIN);
                  sleep_ms(100);
                  
              }
        } else {
            //printf("0");
            gpio_put(RED, 0);
            stop_buzzer(BUZZER1_PIN);
            stop_buzzer(BUZZER2_PIN);
            snprintf(PIR_stats, sizeof(PIR_stats), "TUDO NORMAL!");
        }
        sleep_ms(100);
    }
    cyw43_arch_deinit();  // Desliga o Wi-Fi (não será chamado, pois o loop é infinito)
    return 0;
}

