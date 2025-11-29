
//libereias estandares
#include<string.h>
#include<stdlib.h>
#include <ctype.h>

// FreeRTOS
#include<freertos/FreeRTOS.h>
#include<freertos/event_groups.h>

//manejo de erroes
#include<esp_log.h>
#include<esp_err.h>

//librerias wifi-servidor
#include <esp_wifi.h>
#include<esp_spiffs.h>
#include<nvs_flash.h>
#include<esp_netif.h>
#include<esp_https_server.h>

//libreias personalziadas 
#include"wifi_lib.h"



#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif


static const char *TAG ="modulo/WIFI";

static volatile bool auth_status = false;

// Event Groups
static EventGroupHandle_t auth_event_group = NULL;
static EventGroupHandle_t temp_event_group = NULL;

char buffer_RFID[128];


#define MAX_SSE_CLIENTS 5
typedef struct {
    httpd_req_t* req;
    volatile bool active;
} sse_client_t;

static sse_client_t sse_clients[MAX_SSE_CLIENTS] = {0};

// static volatile bool auth_status = false;
static char current_uid[20] = {0}; // Para guardar el UID autenticado

const char llaves[KEYS][uid_len] = {
    "E3:14:49:01",
    "EA:9A:CD:05",
};


void init_spiffs(void){
    esp_vfs_spiffs_conf_t conf ={
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if(ret != ESP_OK){
        ESP_LOGE(TAG, "error al iniciar SPIFFS (%s)", esp_err_to_name(ret));
        return;
    }

    size_t total = 0, used = 0;

    ret = esp_spiffs_info(NULL, &total, &used);
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "No se puede obtener la informacion de la particion SPIFFS (%s)", esp_err_to_name(ret));
    }
    else{
        ESP_LOGI(TAG, "SPIFFS: tamanio total %d, usado %d", total, used);
    }

     // Crear Event Groups
    auth_event_group = xEventGroupCreate();
    temp_event_group = xEventGroupCreate();
    
    if(auth_event_group == NULL || temp_event_group == NULL){
        ESP_LOGE(TAG, "Error creando event groups");
    }

    // Inicializar estado de autenticación
    xEventGroupClearBits(auth_event_group, AUTH_OK_BIT);
}




void wifi_init_softap(void){
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    const char mi_ssid[]= "TempGuard-3N";

    wifi_config_t wifi_config={
        .ap ={
            .ssid_len = strlen(mi_ssid),
            .channel = 1,
            .password = "987654321",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    memcpy(wifi_config.ap.ssid, mi_ssid, strlen(mi_ssid));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "ESP32 AP iniciado SSID: %s password: %s channel: %d", 
             wifi_config.ap.ssid, wifi_config.ap.password, wifi_config.ap.channel);
}





void start_web_server(void){
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if(httpd_start(&server, &config) == ESP_OK){
        httpd_uri_t login_get = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = login_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &login_get);

        httpd_uri_t login_post = {
            .uri = "/login",
            .method = HTTP_POST,
            .handler = login_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &login_post);

        httpd_uri_t dashboard_get = {
            .uri = "/dashboard",
            .method = HTTP_GET,
            .handler = dashboard_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &dashboard_get);

        httpd_uri_t events_get = {
            .uri = "/events",
            .method = HTTP_GET,
            .handler = events_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &events_get);

          httpd_uri_t css_uri ={
            .uri="/styles.css",
            .method=HTTP_GET,
            .handler= css_get_handler,
            .user_ctx =NULL
        };
        httpd_register_uri_handler(server, &css_uri);

        ESP_LOGI(TAG, "Servidor web iniciado con Event Groups");
    }
}

// Verifica autenticación leyendo estado volatile
bool is_authenticated(void){
    return auth_status;
}

void notify_new_temperature(void){
    if(temp_event_group != NULL) {  // Verificar que exista
        xEventGroupSetBits(temp_event_group, NEW_TEMP_BIT);
    }
}

bool validate_rfid_uid(const char* uid) {
    if(uid == NULL || strlen(uid) == 0) return false;
    
    for(int i = 0; i < KEYS; i++) {
        if(strcmp(uid, llaves[i]) == 0) {
            ESP_LOGI(TAG, "UID VÁLIDO: %s", uid);
            return true;
        }
    }
    ESP_LOGW(TAG, " UID NO AUTORIZADO: %s", uid);
    return false;
}





void notify_rfid_auth(const char* uid){
    
    if(uid == NULL) return;
    
    // Actualizar estado
    auth_status = true;
    strncpy(current_uid, uid, sizeof(current_uid)-1);
    
    // Notificar evento
    if(auth_event_group != NULL){
        xEventGroupSetBits(auth_event_group, AUTH_OK_BIT | AUTH_CHANGED_BIT);
    }
    
    ESP_LOGI(TAG, "Autenticación RFID exitosa. UID: %s", uid);
}



void logout_user(void){
    auth_status = false;
    memset(current_uid, 0, sizeof(current_uid));
    if(auth_event_group != NULL){
        xEventGroupClearBits(auth_event_group, AUTH_OK_BIT);
        xEventGroupSetBits(auth_event_group, AUTH_CHANGED_BIT);
    }
    ESP_LOGI(TAG, "Sesión cerrada");
}




//manejadores

//manjador GET para la pagina de login

esp_err_t login_get_handler(httpd_req_t *req){
    // Verificar si ya está autenticado por RFID
    if(auth_status){
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/dashboard");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    FILE *file = fopen("/spiffs/login.html", "r");
    if(file == NULL){
        ESP_LOGE(TAG, "No se encuentra login.html");
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    char line[256];
    httpd_resp_set_type(req, "text/html");
    while(fgets(line, sizeof(line), file)){
        httpd_resp_sendstr_chunk(req, line);
    }
    httpd_resp_sendstr_chunk(req, NULL);
    fclose(file);
    return ESP_OK;
}

void url_decode(const char *src, char *dest, size_t dest_size) {
    char a, b;
    size_t i = 0;
    while(*src && i < dest_size - 1) {
        if((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b))) {
            if(a >= 'a') a -= 'a'-'A';
            if(a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if(b >= 'a') b -= 'a'-'A';
            if(b >= 'A') b -= ('A' - 10);
            else b -= '0';
            dest[i++] = 16*a+b;
            src+=3;
        } else if(*src == '+') {
            dest[i++] = ' ';
            src++;
        } else {
            dest[i++] = *src++;
        }
    }
    dest[i] = '\0';
}

esp_err_t login_post_handler(httpd_req_t *req){
    char buf[64]; // Aumentado para mayor seguridad
    int ret = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf)-1));
    
    if(ret <= 0){
        ESP_LOGE(TAG, "Error recibiendo datos del POST");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request");
        return ESP_FAIL;
    }
    
    buf[ret] = '\0';
    ESP_LOGI(TAG, "Datos recibidos: '%s'", buf); // LOG IMPORTANTE

    // Verificar formato: "rfid=UID"
    if(strncmp(buf, "rfid=", 5) == 0) {
        char *rfid_value = buf + 5;
        
        // Decodificar caracteres especiales (%3A -> :)
        char decoded_uid[32];
        url_decode(rfid_value, decoded_uid, sizeof(decoded_uid));
        
        ESP_LOGI(TAG, "RFID decodificado: '%s'", decoded_uid);
        
        if(validate_rfid_uid(decoded_uid)) {
            notify_rfid_auth(decoded_uid);
            
            // RESPUESTA INMEDIATA Y CLARA
            httpd_resp_set_status(req, "302 Found");
            httpd_resp_set_hdr(req, "Location", "/dashboard");
            httpd_resp_set_hdr(req, "Set-Cookie", "auth=1; Path=/; HttpOnly");
            httpd_resp_send(req, NULL, 0);
            
            ESP_LOGI(TAG, "ACCESO CONCEDIDO - Redirigiendo a dashboard");
            return ESP_OK;
        }
    }
    
    // ❌ ACCESO DENEGADO
    ESP_LOGW(TAG, "Acceso denegado para: '%s'", buf);
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/?error=1");
    httpd_resp_send(req, NULL, 0);
    return ESP_FAIL;
}





esp_err_t css_get_handler(httpd_req_t *req){


    FILE *file = fopen("/spiffs/styles.css", "r");
    if(file==NULL){
        ESP_LOGE(TAG,"no se pudo abrir el archivo para lectura");
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "text/css");

    char line[255];
    while (fgets(line, sizeof(line), file)){

        httpd_resp_sendstr_chunk(req, line);
    }

    httpd_resp_sendstr_chunk(req, NULL);
    fclose(file);
    return ESP_OK;
}


//manjeador dashboard
// Reemplaza dashboard_get_handler para verificar cookie
esp_err_t dashboard_get_handler(httpd_req_t *req){
    // Verificar estado volatile primero
    if(!is_authenticated()){
        // Verificar cookie como fallback
        char cookie[64] = {0};
        size_t cookie_len = httpd_req_get_hdr_value_len(req, "Cookie");
        
        if(cookie_len > 0 && cookie_len < sizeof(cookie)) {
            if(httpd_req_get_hdr_value_str(req, "Cookie", cookie, cookie_len + 1) == ESP_OK) {
                if(strstr(cookie, "auth=1") != NULL) {
                    // Restaurar sesión
                    auth_status = true;
                    ESP_LOGI(TAG, "Auth: Restaurado por cookie");
                }
            }
        }
        
        // Si aún no está autenticado, redirigir
        if(!is_authenticated()){
            ESP_LOGW(TAG, "Auth: No autenticado - Redirigiendo a login");
            httpd_resp_set_status(req, "302 Found");
            httpd_resp_set_hdr(req, "Location", "/");
            httpd_resp_send(req, NULL, 0);
            return ESP_OK;
        }
    }

    ESP_LOGI(TAG, "Auth: Acceso concedido a dashboard");
    FILE *file = fopen("/spiffs/dashboard.html", "r");
    if(file == NULL){
        ESP_LOGE(TAG, "No se encuentra dashboard.html");
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    char line[256];
    httpd_resp_set_type(req, "text/html");
    while(fgets(line, sizeof(line), file)){
        httpd_resp_sendstr_chunk(req, line);
    }
    httpd_resp_sendstr_chunk(req, NULL);
    fclose(file);
    return ESP_OK;
}

//manjeador para tmepreatura

esp_err_t events_get_handler(httpd_req_t *req){
    // Verificar autenticación
    if(!is_authenticated()){
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Forbidden");
        return ESP_FAIL;
    }

    // Configurar headers SSE
    httpd_resp_set_type(req, "text/event-stream");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");

    // Buscar slot libre
    int client_idx = -1;
    for(int i = 0; i < MAX_SSE_CLIENTS; i++){
        if(!sse_clients[i].active){
            sse_clients[i].req = req;
            sse_clients[i].active = true;
            client_idx = i;
            break;
        }
    }

    if(client_idx == -1){
        ESP_LOGE(TAG, "Máximo de clientes SSE alcanzado");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Too many clients");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Cliente SSE conectado: %d", client_idx);

    // Mantener conexión abierta
    while(1){
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Verificar si cliente sigue conectado
        if(httpd_req_to_sockfd(req) < 0){
            break;
        }
    }

    // Desactivar cliente
    sse_clients[client_idx].active = false;
    ESP_LOGI(TAG, "Cliente SSE desconectado: %d", client_idx);
    return ESP_OK;
}


void send_temperature_to_clients(const char* temp){
    if(temp_event_group == NULL) return;

    for(int i = 0; i < MAX_SSE_CLIENTS; i++){
        if(sse_clients[i].active){
            char event[64];
            snprintf(event, sizeof(event), "data: %s\n\n", temp);
            httpd_resp_sendstr_chunk(sse_clients[i].req, event);
        }
    }
}




