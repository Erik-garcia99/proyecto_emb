//librerias estandars 
#include <stdio.h>
#include<string.h>
#include<stdlib.h>

//modulos de freerots
#include<freertos/FreeRTOS.h>
#include<freertos/task.h>
#include<freertos/event_groups.h>
#include<freertos/queue.h>


//libereias de manejo de errores 
#include<esp_log.h>
#include<esp_err.h>

//WIFI-FLASH
#include<esp_wifi.h>
#include<esp_spiffs.h>
#include<esp_flash.h>
#include<nvs_flash.h>
#include<esp_netif.h>
#include<esp_http_server.h>

//drivers 
#include<driver/uart.h>

//librerias propias 
#include"modulos/WIFI/wifi_lib.h"
#include"modulos/UART/uart_lib.h"
#include"modulos/GPIO/gpio_lib.h"

//macros 

#define UART_DEMON_TMP UART_NUM_1// por medio de este UART el ESP AP recibe los datos del ESP que tiene el sensor de temperarutra. 
#define UART_DEMON_RFID UART_NUM_2

#define TX_PIN_TMP 19
#define RX_PIN_TMP 18

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

//MACROS UART 2
//UART 2 PINS
#define UART2_TX 17 //TX2
#define UART2_RX 16 //RX2
#define UART_NUM2 UART_NUM_2 //UART PUERTO 2


//varibales 
extern QueueHandle_t event_uart; //cola que recibe los datos de UART. 
QueueHandle_t send_temp;

static const char *TAG = "ESP_AP";

//funciones 
extern void send_temperature_to_clients(const char* temp);
extern void notify_new_temperature(void);


//tareas 
// void task_main(void *params);
//tarea que va a estar recibiendo los datos que esta leyendo el uart de TMP entonces este va ser mostrado en su apartado. 
void show_tmep_task(void *params);
void uart_task_read_temp(void *params);

//tareas RFID
void receive_RFID_task();
uint8_t compare_RFID();

//BUFFER RFID
char buffer_RFID[128];   

//LLAVES
#define KEYS 2
#define uid_len 12
char llaves[KEYS][uid_len] = {
    "E3:14:49:01",
    "EA:9A:CD:05",
};



void app_main(void)
{

    //inciando colas 
    send_temp = xQueueCreate(10, sizeof(char[12])); /// la cola por la cual enviaremos o que se reciba del ESP que tiene el sensor de tmeperatura 


    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%   WIFI 

    
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    //servidor - WIFI
    init_spiffs();
    wifi_init_softap();
    start_web_server();

    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%



    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    //seccion de UART 

    init_uart(UART_DEMON_TMP,TX_PIN_TMP,RX_PIN_TMP,UART_DATA_8_BITS,UART_PARITY_DISABLE,UART_STOP_BITS_1);

    //UART2 PARA RFID

    init_uart(UART_NUM2,UART2_TX,UART2_RX,UART_DATA_8_BITS,UART_PARITY_DISABLE,UART_STOP_BITS_1);


    //inicamos la tarea la cual se encarga de estar esperando a que se reciba lago por UART. 
    //debe de ener un numero de prioridad alto porque es lo que estara mostrando dentro de la pagina.
    //este debe ser de mayor prioridad porque es importante el tener los datos lo mas actualizado posible,  
    xTaskCreate(uart_task_read_temp, "uart_task", 4098, NULL, 10, NULL);
    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

        


    // xTaskCreate(task_main,"task_main", 4098, NULL, 8, NULL);
    xTaskCreate(show_tmep_task, "show_tmp", 2048, NULL, 9, NULL);

    //TAREA RECIBIR RFID
    xTaskCreate(receive_RFID_task, "Receive_RFID",2048,NULL,5,NULL); //recibe RFID

}


// static void task_main(void *params){


//     while(1){






//     }


// }



void uart_task_read_temp(void *params){

    uart_event_t event; 
    uint8_t *buffer=(uint8_t*)malloc(BUFF);

    if(buffer == NULL) {
        ESP_LOGE(TAG, "Error alocando buffer");
        vTaskDelete(NULL);
    }

    char data[12]={0};
    size_t pos = 0;
    while(1){
        if(xQueueReceive(event_uart, (void *)&event, portMAX_DELAY)){
            memset(buffer,0, BUFF);

            switch(event.type){

                case UART_DATA:{
                        // Leer byte a byte hasta encontrar '\n'
                        uint8_t byte;
                        // Lee mientras haya datos disponibles
                        while(uart_read_bytes(UART_DEMON_TMP, &byte, 1, pdMS_TO_TICKS(10)) > 0){
                            if(byte == '\n' || pos >= sizeof(data)-1){
                                data[pos] = '\0';
                                ESP_LOGI(TAG, "Temperatura recibida: %s", data);
                                
                                if(send_temp != NULL){
                                    xQueueSend(send_temp, &data, 0);
                                    notify_new_temperature();
                                }
                                
                                pos = 0;
                                memset(data, 0, sizeof(data));
                            } else {
                                data[pos++] = byte;  // CORREGIDO: usa data[]
                            }
                        }
                }break;

                case UART_BUFFER_FULL:{
                    ESP_LOGI(TAG,"BUFFER FULL");
                    uart_flush_input(UART_DEMON_TMP);
                    xQueueReset(event_uart);
                }break;

                case UART_FIFO_OVF:{
                    ESP_LOGI(TAG, "FIFO OVERFLOW");
                    uart_flush_input(UART_DEMON_TMP);
                    xQueueReset(event_uart);
                }break;

                default :{
                    ESP_LOGI(TAG, "uart event type %d", event.type);
                }



            }



        }
    }



}


void show_tmep_task(void *params){
    char temp[12];
    
    while(1){
        if(xQueueReceive(send_temp, &temp, portMAX_DELAY)){
            ESP_LOGI(TAG, "Temperatura recibida: %s", temp);
            // Enviar a todos los clientes SSE
            send_temperature_to_clients(temp);
        }
    }
}

uint8_t compare_RFID()
{
    for (int j = 0; j < KEYS; j++)
    {
        if (strcmp(buffer_RFID, llaves[j]) == 0)
        {
            ESP_LOGI(TAG, "ACCESO PERMITIDO");
            return 1;
        }
    }

    ESP_LOGI(TAG, "ACCESO DENEGADO");
    return 0;
}

void receive_RFID_task()
{
    uint8_t data[12];

    while(1){

        int len = uart_read_bytes(UART_NUM_2, data, 11, portMAX_DELAY);

        if (len > 0) {
            data[len] = '\0';              // terminar la cadena recibida
            strcpy(buffer_RFID, (char*)data);   // guardar en la variable global
        }

        ESP_LOGI(TAG, "UARTO-2 RFID recibido: %s", buffer_RFID);
        compare_RFID();
    }
}
