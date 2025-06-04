#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h> 

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/unique_id.h"

#include "hardware/gpio.h"

#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"
#include "lwip/dns.h"
#include "lwip/altcp_tls.h"

#include "lib/matrixws.h"
#include "lib/display_init.h" 

// --- Configurações Wi-Fi ---
#define WIFI_SSID       "XXXX"
#define WIFI_PASSWORD   "XXXX"

// --- Configurações MQTT ---
#define MQTT_SERVER     "192.168.1.121" // Substitua pelo endereço do seu broker Mosquitto
#define MQTT_USERNAME   "RC"            // Substitua pelo nome de usuário
#define MQTT_PASSWORD   "RC"            // Substitua pela senha

// QoS e Tópicos
#define MQTT_SUBSCRIBE_QOS 1
#define MQTT_PUBLISH_QOS 1
#define MQTT_PUBLISH_RETAIN 0 // Não reter mensagens de estado por padrão

// Last Will and Testament (Indicador de online/offline)
#define MQTT_WILL_TOPIC "/online"
#define MQTT_WILL_MSG "0"
#define MQTT_WILL_QOS 1

#ifndef MQTT_DEVICE_NAME
#define MQTT_DEVICE_NAME "pico_lighting"
#endif

#ifndef MQTT_UNIQUE_TOPIC
#define MQTT_UNIQUE_TOPIC 0 // Definir como 1 para adicionar o nome do cliente aos tópicos
#endif

#ifndef MQTT_TOPIC_LEN
#define MQTT_TOPIC_LEN 100
#endif

// --- Configurações da Matriz ---
#define BRILHO_PADRAO 30
#define PINO_MATRIZ   7

// Estados dos quadrantes (variáveis globais)
static bool q1_on = false, q2_on = false, q3_on = false, q4_on = false;

// Matriz base: cruz branca de brilho baixo para separar os quadrantes
static int base_mat[5][5][3] = {
    {{0,0,0},{0,0,0},{BRILHO_PADRAO,BRILHO_PADRAO,BRILHO_PADRAO},{0,0,0},{0,0,0}},
    {{0,0,0},{0,0,0},{BRILHO_PADRAO,BRILHO_PADRAO,BRILHO_PADRAO},{0,0,0},{0,0,0}},
    {{BRILHO_PADRAO,BRILHO_PADRAO,BRILHO_PADRAO},{BRILHO_PADRAO,BRILHO_PADRAO,BRILHO_PADRAO},{BRILHO_PADRAO,BRILHO_PADRAO,BRILHO_PADRAO},{BRILHO_PADRAO,BRILHO_PADRAO,BRILHO_PADRAO},{BRILHO_PADRAO,BRILHO_PADRAO,BRILHO_PADRAO}},
    {{0,0,0},{0,0,0},{BRILHO_PADRAO,BRILHO_PADRAO,BRILHO_PADRAO},{0,0,0},{0,0,0}},
    {{0,0,0},{0,0,0},{BRILHO_PADRAO,BRILHO_PADRAO,BRILHO_PADRAO},{0,0,0},{0,0,0}}
};

// --- Estrutura de dados do cliente MQTT ---
typedef struct {
    mqtt_client_t* mqtt_client_inst;
    struct mqtt_connect_client_info_t mqtt_client_info;
    char data[MQTT_OUTPUT_RINGBUF_SIZE];
    char topic[MQTT_TOPIC_LEN];
    uint32_t len;
    ip_addr_t mqtt_server_address;
    bool connect_done;
    int subscribe_count;
    bool stop_client;
} MQTT_CLIENT_DATA_T;

// Debugging macros
#ifndef DEBUG_printf
#ifndef NDEBUG
#define DEBUG_printf printf
#else
#define DEBUG_printf(...)
#endif
#endif

#ifndef INFO_printf
#define INFO_printf printf
#endif

#ifndef ERROR_printf
#define ERROR_printf printf
#endif

// --- Protótipos das funções ---
static void pub_request_cb(__unused void *arg, err_t err);
static const char *full_topic(MQTT_CLIENT_DATA_T *state, const char *name);
static void sub_request_cb(void *arg, err_t err);
static void unsub_request_cb(void *arg, err_t err);
static void sub_unsub_topics(MQTT_CLIENT_DATA_T* state, bool sub);
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len);
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
static void start_client(MQTT_CLIENT_DATA_T *state);
static void dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg);

// Funções de controle do LED e display
void update_matrix(MQTT_CLIENT_DATA_T *state);
void update_display(MQTT_CLIENT_DATA_T *state);


// --- Implementação das Funções ---

// Publica o estado de um quadrante específico (função auxiliar mantida para evitar repetição)
// Esta função é chamada internamente pela lógica principal.
static void publish_quadrant_state_explicit(MQTT_CLIENT_DATA_T *state, int quadrant_num, bool on_off) {
    char topic_name[MQTT_TOPIC_LEN];
    snprintf(topic_name, sizeof(topic_name), "/quadrante%d/state", quadrant_num);
    const char* message = on_off ? "On" : "Off";
    INFO_printf("Publishing %s to %s\n", message, full_topic(state, topic_name));
    mqtt_publish(state->mqtt_client_inst, full_topic(state, topic_name), message, strlen(message), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
}

// Publica a potência total (função auxiliar mantida)
static void publish_total_power_explicit(MQTT_CLIENT_DATA_T *state) {
    int potencia = (q1_on + q2_on + q3_on + q4_on) * 10;
    char power_str[16];
    snprintf(power_str, sizeof(power_str), "%d", potencia);
    INFO_printf("Publishing total power %s W to %s\n", power_str, full_topic(state, "/potencia/total"));
    mqtt_publish(state->mqtt_client_inst, full_topic(state, "/potencia/total"), power_str, strlen(power_str), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
}


// Atualiza o display OLED com potência total e publica o estado
void update_display(MQTT_CLIENT_DATA_T *state) {
    int potencia = (q1_on + q2_on + q3_on + q4_on) * 10;
    char buf[32];
    ssd1306_fill(&ssd, false);
    snprintf(buf, sizeof(buf), "Potencia: %d W", potencia);
    ssd1306_draw_string(&ssd, buf, 0, 0);
    ssd1306_send_data(&ssd);
    publish_total_power_explicit(state); // Chama a função auxiliar para publicar
}

// Atualiza a matriz de LEDs
void update_matrix(MQTT_CLIENT_DATA_T *state) {
    int mat[5][5][3];
    memcpy(mat, base_mat, sizeof(mat)); // Copia a matriz base com a cruz acesa

    if (q1_on) for (int y=0;y<2;y++) for (int x=0;x<2;x++) mat[y][x][0]=mat[y][x][1]=mat[y][x][2]=BRILHO_PADRAO;
    if (q2_on) for (int y=0;y<2;y++) for (int x=3;x<5;x++) mat[y][x][0]=mat[y][x][1]=mat[y][x][2]=BRILHO_PADRAO;
    if (q3_on) for (int y=3;y<5;y++) for (int x=0;x<2;x++) mat[y][x][0]=mat[y][x][1]=mat[y][x][2]=BRILHO_PADRAO;
    if (q4_on) for (int y=3;y<5;y++) for (int x=3;x<5;x++) mat[y][x][0]=mat[y][x][1]=mat[y][x][2]=BRILHO_PADRAO;

    desenhaMatriz(mat);
    set_brilho(BRILHO_PADRAO);
    bf();
    update_display(state);
}

// --- Funções MQTT ---

static void pub_request_cb(__unused void *arg, err_t err) {
    if (err != 0) {
        ERROR_printf("pub_request_cb failed %d", err);
    }
}

static const char *full_topic(MQTT_CLIENT_DATA_T *state, const char *name) {
#if MQTT_UNIQUE_TOPIC
    static char full_topic_buf[MQTT_TOPIC_LEN];
    snprintf(full_topic_buf, sizeof(full_topic_buf), "/%s%s", state->mqtt_client_info.client_id, name);
    return full_topic_buf;
#else
    return name;
#endif
}

static void sub_request_cb(void *arg, err_t err) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
    if (err != 0) {
        panic("subscribe request failed %d", err);
    }
    state->subscribe_count++;
}

static void unsub_request_cb(void *arg, err_t err) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
    if (err != 0) {
        panic("unsubscribe request failed %d", err);
    }
    state->subscribe_count--;
    assert(state->subscribe_count >= 0);

    if (state->subscribe_count <= 0 && state->stop_client) {
        mqtt_disconnect(state->mqtt_client_inst);
    }
}

// Tópicos de assinatura (para a placa ESCUTAR comandos)
static void sub_unsub_topics(MQTT_CLIENT_DATA_T* state, bool sub) {
    mqtt_request_cb_t cb = sub ? sub_request_cb : unsub_request_cb;
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/quadrante1"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/quadrante2"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/quadrante3"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/quadrante4"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/print"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/ping"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/exit"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
#if MQTT_UNIQUE_TOPIC
    const char *basic_topic = state->topic + strlen(state->mqtt_client_info.client_id) + 1;
#else
    const char *basic_topic = state->topic;
#endif
    strncpy(state->data, (const char *)data, len);
    state->len = len;
    state->data[len] = '\0';

    DEBUG_printf("Topic: %s, Message: %s\n", state->topic, state->data);

    bool changed = false;
    const char* publish_message; // Variável para a mensagem "On" ou "Off"

    if (strcmp(basic_topic, "/quadrante1") == 0) {
        if (lwip_stricmp((const char *)state->data, "On") == 0 || strcmp((const char *)state->data, "1") == 0) {
            if (!q1_on) { q1_on = true; changed = true; }
            publish_message = "On";
        } else if (lwip_stricmp((const char *)state->data, "Off") == 0 || strcmp((const char *)state->data, "0") == 0) {
            if (q1_on) { q1_on = false; changed = true; }
            publish_message = "Off";
        }
        // Publicação explícita do estado do quadrante 1
        if (changed) {
            INFO_printf("Publishing %s to %s\n", publish_message, full_topic(state, "/quadrante1/state"));
            mqtt_publish(state->mqtt_client_inst, full_topic(state, "/quadrante1/state"), publish_message, strlen(publish_message), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
        }
    } else if (strcmp(basic_topic, "/quadrante2") == 0) {
        if (lwip_stricmp((const char *)state->data, "On") == 0 || strcmp((const char *)state->data, "1") == 0) {
            if (!q2_on) { q2_on = true; changed = true; }
            publish_message = "On";
        } else if (lwip_stricmp((const char *)state->data, "Off") == 0 || strcmp((const char *)state->data, "0") == 0) {
            if (q2_on) { q2_on = false; changed = true; }
            publish_message = "Off";
        }
        // Publicação explícita do estado do quadrante 2
        if (changed) {
            INFO_printf("Publishing %s to %s\n", publish_message, full_topic(state, "/quadrante2/state"));
            mqtt_publish(state->mqtt_client_inst, full_topic(state, "/quadrante2/state"), publish_message, strlen(publish_message), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
        }
    } else if (strcmp(basic_topic, "/quadrante3") == 0) {
        if (lwip_stricmp((const char *)state->data, "On") == 0 || strcmp((const char *)state->data, "1") == 0) {
            if (!q3_on) { q3_on = true; changed = true; }
            publish_message = "On";
        } else if (lwip_stricmp((const char *)state->data, "Off") == 0 || strcmp((const char *)state->data, "0") == 0) {
            if (q3_on) { q3_on = false; changed = true; }
            publish_message = "Off";
        }
        // Publicação explícita do estado do quadrante 3
        if (changed) {
            INFO_printf("Publishing %s to %s\n", publish_message, full_topic(state, "/quadrante3/state"));
            mqtt_publish(state->mqtt_client_inst, full_topic(state, "/quadrante3/state"), publish_message, strlen(publish_message), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
        }
    } else if (strcmp(basic_topic, "/quadrante4") == 0) {
        if (lwip_stricmp((const char *)state->data, "On") == 0 || strcmp((const char *)state->data, "1") == 0) {
            if (!q4_on) { q4_on = true; changed = true; }
            publish_message = "On";
        } else if (lwip_stricmp((const char *)state->data, "Off") == 0 || strcmp((const char *)state->data, "0") == 0) {
            if (q4_on) { q4_on = false; changed = true; }
            publish_message = "Off";
        }
        // Publicação explícita do estado do quadrante 4
        if (changed) {
            INFO_printf("Publishing %s to %s\n", publish_message, full_topic(state, "/quadrante4/state"));
            mqtt_publish(state->mqtt_client_inst, full_topic(state, "/quadrante4/state"), publish_message, strlen(publish_message), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
        }
    } else if (strcmp(basic_topic, "/print") == 0) {
        INFO_printf("%.*s\n", len, data);
    } else if (strcmp(basic_topic, "/ping") == 0) {
        char buf[11];
        snprintf(buf, sizeof(buf), "%u", to_ms_since_boot(get_absolute_time()) / 1000);
        mqtt_publish(state->mqtt_client_inst, full_topic(state, "/uptime"), buf, strlen(buf), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
    } else if (strcmp(basic_topic, "/exit") == 0) {
        state->stop_client = true;
        sub_unsub_topics(state, false);
    }

    if (changed) {
        update_matrix(state); // Somente atualiza a matriz e display se houve mudança
    }
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
    strncpy(state->topic, topic, sizeof(state->topic));
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
    if (status == MQTT_CONNECT_ACCEPTED) {
        state->connect_done = true;
        sub_unsub_topics(state, true); // Assina os tópicos de controle
        // Indicar online e publicar estado inicial dos quadrantes e potência
        if (state->mqtt_client_info.will_topic) {
            mqtt_publish(state->mqtt_client_inst, full_topic(state, "/online"), "1", 1, MQTT_WILL_QOS, true, pub_request_cb, state);
        }
        
        // PUBLICANDO ESTADOS INICIAIS DE FORMA EXPLÍCITA:
        const char* initial_message_q1 = q1_on ? "On" : "Off";
        INFO_printf("Publishing %s to %s\n", initial_message_q1, full_topic(state, "/quadrante1/state"));
        mqtt_publish(state->mqtt_client_inst, full_topic(state, "/quadrante1/state"), initial_message_q1, strlen(initial_message_q1), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);

        const char* initial_message_q2 = q2_on ? "On" : "Off";
        INFO_printf("Publishing %s to %s\n", initial_message_q2, full_topic(state, "/quadrante2/state"));
        mqtt_publish(state->mqtt_client_inst, full_topic(state, "/quadrante2/state"), initial_message_q2, strlen(initial_message_q2), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);

        const char* initial_message_q3 = q3_on ? "On" : "Off";
        INFO_printf("Publishing %s to %s\n", initial_message_q3, full_topic(state, "/quadrante3/state"));
        mqtt_publish(state->mqtt_client_inst, full_topic(state, "/quadrante3/state"), initial_message_q3, strlen(initial_message_q3), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);

        const char* initial_message_q4 = q4_on ? "On" : "Off";
        INFO_printf("Publishing %s to %s\n", initial_message_q4, full_topic(state, "/quadrante4/state"));
        mqtt_publish(state->mqtt_client_inst, full_topic(state, "/quadrante4/state"), initial_message_q4, strlen(initial_message_q4), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);

        // Publica a potência total inicial
        int initial_potencia = (q1_on + q2_on + q3_on + q4_on) * 10;
        char initial_power_str[16];
        snprintf(initial_power_str, sizeof(initial_power_str), "%d", initial_potencia);
        INFO_printf("Publishing total power %s W to %s\n", initial_power_str, full_topic(state, "/potencia/total"));
        mqtt_publish(state->mqtt_client_inst, full_topic(state, "/potencia/total"), initial_power_str, strlen(initial_power_str), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);

        // Chame update_matrix aqui para exibir o estado inicial da matriz e display
        update_matrix(state); 

    } else if (status == MQTT_CONNECT_DISCONNECTED) {
        if (!state->connect_done) {
            panic("Failed to connect to mqtt server");
        }
    } else {
        panic("Unexpected status");
    }
}

static void start_client(MQTT_CLIENT_DATA_T *state) {
#if LWIP_ALTCP && LWIP_ALTCP_TLS
    const int port = MQTT_TLS_PORT;
    INFO_printf("Using TLS\n");
#else
    const int port = MQTT_PORT;
    INFO_printf("Warning: Not using TLS\n");
#endif

    state->mqtt_client_inst = mqtt_client_new();
    if (!state->mqtt_client_inst) {
        panic("MQTT client instance creation error");
    }
    INFO_printf("IP address of this device %s\n", ipaddr_ntoa(&(netif_list->ip_addr))); 
    INFO_printf("Connecting to mqtt server at %s\n", ipaddr_ntoa(&state->mqtt_server_address));

    cyw43_arch_lwip_begin();
    if (mqtt_client_connect(state->mqtt_client_inst, &state->mqtt_server_address, port, mqtt_connection_cb, state, &state->mqtt_client_info) != ERR_OK) {
        panic("MQTT broker connection error");
    }
#if LWIP_ALTCP && LWIP_ALTCP_TLS
    mbedtls_ssl_set_hostname(altcp_tls_context(state->mqtt_client_inst->conn), MQTT_SERVER);
#endif
    mqtt_set_inpub_callback(state->mqtt_client_inst, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, state);
    cyw43_arch_lwip_end();
}

static void dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    MQTT_CLIENT_DATA_T *state = (MQTT_CLIENT_DATA_T*)arg;
    if (ipaddr) {
        state->mqtt_server_address = *ipaddr;
        start_client(state);
    } else {
        panic("dns request failed");
    }
}

// --- Função Main ---
int main() {
    stdio_init_all();
    INFO_printf("MQTT client for lighting control starting\n");

    controle(PINO_MATRIZ);
    display();        // inicializa OLED
    static MQTT_CLIENT_DATA_T state; // Cria registro com os dados do cliente

    if (cyw43_arch_init()) {
        panic("Failed to initialize CYW43");
    }

    char unique_id_buf[5];
    pico_get_unique_board_id_string(unique_id_buf, sizeof(unique_id_buf));
    for(int i=0; i < sizeof(unique_id_buf) - 1; i++) {
        unique_id_buf[i] = tolower(unique_id_buf[i]);
    }
    char client_id_buf[sizeof(MQTT_DEVICE_NAME) + sizeof(unique_id_buf) - 1];
    memcpy(&client_id_buf[0], MQTT_DEVICE_NAME, sizeof(MQTT_DEVICE_NAME) - 1);
    memcpy(&client_id_buf[sizeof(MQTT_DEVICE_NAME) - 1], unique_id_buf, sizeof(unique_id_buf) - 1);
    client_id_buf[sizeof(client_id_buf) - 1] = 0;
    INFO_printf("Device name %s\n", client_id_buf);

    state.mqtt_client_info.client_id = client_id_buf;
    state.mqtt_client_info.keep_alive = 60;
#if defined(MQTT_USERNAME) && defined(MQTT_PASSWORD)
    state.mqtt_client_info.client_user = MQTT_USERNAME;
    state.mqtt_client_info.client_pass = MQTT_PASSWORD;
#else
    state.mqtt_client_info.client_user = NULL;
    state.mqtt_client_info.client_pass = NULL;
#endif
    static char will_topic[MQTT_TOPIC_LEN];
    strncpy(will_topic, full_topic(&state, MQTT_WILL_TOPIC), sizeof(will_topic));
    state.mqtt_client_info.will_topic = will_topic;
    state.mqtt_client_info.will_msg = MQTT_WILL_MSG;
    state.mqtt_client_info.will_qos = MQTT_WILL_QOS;
    state.mqtt_client_info.will_retain = true;
    
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        panic("Failed to connect to Wi-Fi");
    }
    INFO_printf("\nConnected to Wifi\n");
    if (netif_default) printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));

    cyw43_arch_lwip_begin();
    int err = dns_gethostbyname(MQTT_SERVER, &state.mqtt_server_address, dns_found, &state);
    cyw43_arch_lwip_end();

    if (err == ERR_OK) {
        start_client(&state);
    } else if (err != ERR_INPROGRESS) {
        panic("DNS request failed");
    }

    while (!state.connect_done || mqtt_client_is_connected(state.mqtt_client_inst)) {
        cyw43_arch_poll();
        sleep_ms(10);
    }

    INFO_printf("MQTT client for lighting control exiting\n");
    cyw43_arch_deinit();
    return 0;
}