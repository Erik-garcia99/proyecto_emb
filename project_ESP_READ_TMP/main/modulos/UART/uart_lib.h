#ifndef UART_LIB_H
#define UART_LIB_H

// #include <freertos/FreeRTOS.h>
// #include <freertos/queue.h>
#include <stdint.h>
#include <esp_err.h>
#include <driver/uart.h>
//macros
#define BUFF 1024

//varibales
// extern QueueHandle_t event_uart;


//definiciones de funciones 


/**
 * 
 * @brief define el UART, todos sus parametros 
 * 
 * @param num_uart el numero del uart 
 * @param pin_tx pin del transmisor
 * @param pin_rx pin del reseptor 
 * @param data el tamanio de bits del frame
 * @param bits_parity habilitado o deshabitiadp el bit de paridad
 * @param stop_bits_f la cantidad de stop bits que delimietan el final del frame
 * 
 * @return
 *  ESP_OK -> si la configuracion se realizo con exito 
 *  ESP_FAIL -> si ocurrio un error que algun parametro no esta bien 
 * 
 */
esp_err_t init_uart(uart_port_t num_uart,int pin_tx, int pin_rx, uart_word_length_t data,uart_parity_t bits_parity,uart_stop_bits_t stop_bits_f);


//porque este ESP no va a recibir nada por UART se comunicara por UART. 

// //tareas 
// /**
//  * @brief tarea que se encargara de recibir y procesar los datos obtenidos por UART 
//  * 
//  * 
//  * 
//  * 
//  * @return -> noting
//  */

// void uart_task(void *params);

#endif