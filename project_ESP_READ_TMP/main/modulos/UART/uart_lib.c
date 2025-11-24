
#include "uart_lib.h"  // Debe ser el primero
#include <stdint.h>
#include<stdlib.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <esp_err.h>




//una macro que detectar si el uart que se envio esta dentro del rengo acordado 
#define MAX_UART UART_NUM_MAX


static uart_port_t current_uart;


QueueHandle_t event_uart;

static const char* TAG = "UART MASTER";

const char lf[]="\n";


esp_err_t init_uart(uart_port_t num_uart,int pin_tx, int pin_rx, uart_word_length_t data,uart_parity_t bits_parity,uart_stop_bits_t stop_bits_f){

    ESP_LOGI(TAG,"CONFIGURANDO UART");


    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = data,
        .parity = bits_parity,
        .stop_bits = stop_bits_f,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk= UART_SCLK_DEFAULT,
    };

    if(num_uart >= MAX_UART){
        //error al querer configurar un UART fura del rango 
        return ESP_FAIL;
    }

    else{
        current_uart = num_uart;
        //el numero del uart entra bein 
        ESP_ERROR_CHECK(uart_param_config(num_uart, &uart_config));
        ESP_ERROR_CHECK(uart_driver_install(num_uart,BUFF*2, BUFF*2,20,&event_uart,0));
        ESP_ERROR_CHECK(uart_set_pin(num_uart,pin_tx,pin_rx,UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

        uart_flush(num_uart);

        
        return ESP_OK; //el uart se configuro correctmanete 
    }
    
    return ESP_FAIL; //EN CASO DE cualquier error retornar false 
}

