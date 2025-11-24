//librerias estandars 
#include <stdio.h>
#include<string.h>
#include<stdlib.h>

//librerias de freertos 
#include<freertos/FreeRTOS.h>
#include<freertos/task.h>
#include<freertos/queue.h>


//libereias de manejo de errores 
#include<esp_log.h>
#include<esp_err.h>

//drivers 
#include<driver/uart.h>

//librerias propias 
#include"modulos/UART/uart_lib.h"

//macros 
//recibe desde UART1 - igual lo cambiamos es lo verga 
#define NUM_UART UART_NUM_1
#define TX_PIN 19
#define RX_PIN 18


//varibales 
extern QueueHandle_t event_uart; //cola que recibe los datos de UART. 
QueueHandle_t send_temp;

static const char *TAG = "ESP_AP";


//funciones 

//tareas 



void app_main(void)
{

    //inciando colas 
    send_temp = xQueueCreate(10, sizeof(char[12])); /// la cola por la cual enviaremos o que se reciba del ESP que tiene el sensor de tmeperatura 



    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    //seccion de UART 

    init_uart(NUM_UART,TX_PIN,RX_PIN,UART_DATA_8_BITS,UART_PARITY_DISABLE,UART_STOP_BITS_1);


    //inicamos la tarea la cual se encarga de estar esperando a que se reciba lago por UART. 
    //debe de ener un numero de prioridad alto porque es lo que estara mostrando dentro de la pagina. 
    xTaskCreate(uart_task, "uart_task", 4098, NULL, 9, NULL);
    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

}


void uart_task(void *params){

    uart_event_t event; 
    uint8_t *buffer=(uint8_t*)malloc(BUFF);
    char data[12]={0};
    while(1){
        if(xQueueReceive(event_uart, (void *)&event, portMAX_DELAY)){
            memset(buffer,0, BUFF);

            switch(event.type){

                case UART_DATA:{
                    //recibio alg mi pa 

                    int len; 

                    len = uart_read_bytes(NUM_UART, buffer,event.size, portMAX_DELAY);
                    ESP_LOGI(TAG,"len: %d DATA:",event.size);
                    uart_write_bytes(NUM_UART, (const char *)buffer, event.size);
                    uart_write_bytes(NUM_UART, "\n", 2);

                    memset(data, 0, sizeof(data));
                    strncpy(data, (char*)buffer, sizeof(data) - 1);
    
                    

                    //una vez recibida la infromacion no como enviamos la infroacion para que sea procesada en la pagina web?, podemos enviar   la infromacion por una cola 
                    xQueueSend(send_temp, data, 0);

                }break;

                case UART_BUFFER_FULL:{
                    ESP_LOGI(TAG,"BUFFER FULL");
                    uart_flush_input(NUM_UART);
                    xQueueReset(event_uart);
                }break;

                case UART_FIFO_OVF:{
                    ESP_LOGI(TAG, "FIFO OVERFLOW");
                    uart_flush_input(NUM_UART);
                    xQueueReset(event_uart);
                }break;

                default :{
                    ESP_LOGI(TAG, "uart event type %s", event.type);
                }



            }



        }
    }



}