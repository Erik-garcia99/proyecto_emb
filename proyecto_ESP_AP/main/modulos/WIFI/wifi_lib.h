#ifndef WIFI_LIB_H
#define WIFI_LIB_H 

#include <esp_http_server.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <stdbool.h>
//funciones estandar que estan si o si 


// Event Groups bits
#define AUTH_OK_BIT BIT0
#define AUTH_CHANGED_BIT BIT1
#define NEW_TEMP_BIT BIT0





/**
 * 
 * @brief esta funcion permite guardar en memoria flash los archivos que serviran como la presetnacion web del sistema guarda, el HTML/CSS/JS asi como imgaenes esto para que todo sea mucho mas rapido
 * 
*/

void init_spiffs(void);

/**
 *
 * @brief el proposito es convertir el ESP32 en un router WIFI, en esta funcion se configura el SSID <nombre de la red>, password, la cantiad mazima de estaciones <conexiones> que permite. configura los aspectos basicos de la red mas no inicaliza el rpoceso de la conversion a un ACCESS POINT
 *
 *
 *
*/

void wifi_init_softap(void);

/**
 * 
 * @brief funcion que monta los manejadores de las diferentes peticiones, paginas que se muestras o se solicita
 * 
 * 
*/
void start_web_server(void);




//metodos de URI para gestionar peticiones URI 

/**
 * @brief Manejador GET para la pagina de inicio de sesión
 * 
 * @param req Puntero a la estructura de solicitud HTTP
 * @return ESP_OK si se envió la página correctamente, ESP_FAIL si hubo error
 *
 * @details sirve la pagina login.html, es la pagina de iniciod donde se pude la autenticacion
 */
esp_err_t login_get_handler(httpd_req_t *req);

/**
 * @brief Manejador POST para procesar el inicio de sesión
 * 
 * @param req Puntero a la estructura de solicitud HTTP
 * @return ESP_OK si la autenticación fue exitosa, ESP_FAIL si falló
 *
 * @details Recibe los datos del formulario de login, verifica el RFID (simulado por ahorita que no tengo el RFID), redirige a la pagina del dashboard si tiene existo si no muestra un error 
 */
esp_err_t login_post_handler(httpd_req_t *req);


/**
 * @brief Manejador GET para el dashboard principal
 * 
 * @param req Puntero a la estructura de solicitud HTTP
 * @return ESP_OK si se envió la página correctamente, ESP_FAIL si hubo error
 *
 * @details Verifica que el usuario esté autenticado mediante cookie.
 * Si no lo esta, redirige a la página de login. Si está autenticado,
 * sirve el dashboard.html con la visualización de temperatura en tiempo real.
 */
esp_err_t dashboard_get_handler(httpd_req_t *req);

/**
 * @brief Manejador GET para Server-Sent Events de temperatura
 * 
 * @param req Puntero a la estructura de solicitud HTTP
 * @return ESP_OK mientras la conexión esté activa
 *
 * @details Establece una conexión persistente con el navegador para enviar
 * actualizaciones de temperatura en tiempo real. Requiere autenticacion.
 * Mantiene el cliente en una lista hasta que se desconecta.
 */
esp_err_t events_get_handler(httpd_req_t *req);

/**
 * 
 * @brief entrega css para dar estilo a la pagina 
 * 
 * @return ESP_OK si se pudo cargar con exito la pagina 
 * 
 * 
 */
esp_err_t css_get_handler(httpd_req_t *req);


bool is_authenticated(void); //verifica autenticacion




#endif