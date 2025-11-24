//libereiras basicas de C 
#include <stdio.h>
#include<string.h>

//libereias de freeRTOS
#include<freertos/FreeRTOS.h>
#include<freertos/task.h>

//DRIVERS
#include<driver/i2c.h>
#include<driver/i2c_types.h>
#include<driver/uart.h>


//manjeador de errores
#include<esp_err.h>
#include<esp_log.h>     

//libererias personlaies 
#include<modulos/I2C/i2c_lib.h>
#include"modulos/UART/uart_lib.h"



//macros
//I2C 
#define I2C_SDA_IO 21
#define I2C_SCL_IO 22
#define SPEED 100000
#define I2C_PORT I2C_NUM_0
#define LM75AB_ADDR 0x48
#define ACK_CHECK_EN 0x1

//UART
#define NUM_UART UART_NUM_0
#define TX_PIN 18
#define RX_PIN 19

//variables 
static const char *TAG = "NODE LM75AB - TEMP";

//creamos una estrucutura para representar los grados que esta leyendo el sensor. de la mejor forma puesto que un uControlador no es que no pueda 
//pero se le es mas complicado trabajar con puntos decimales y mas aparte que este es una respuesta con punto decimal con signo 
typedef struct{
    int8_t integer; //porque la temperatura puede ser negativa o positiva
    uint8_t decimal; // 0 o 5 para repesentar los valores, estara escalador
}temperature_t;


//funciones 
bool read_sens_tmp(temperature_t *tmp);

//tareas 

void main_task(void *params);






void app_main(void)
{
    
    i2c_master_init(I2C_PORT, I2C_SDA_IO,I2C_SCL_IO, SPEED);
    init_uart(NUM_UART,TX_PIN, RX_PIN, UART_DATA_8_BITS,UART_PARITY_DISABLE, UART_STOP_BITS_1);
    


    xTaskCreate(main_task, "main_task_temp", 4098, NULL, 10, NULL);

}


void main_task(void *params){

    temperature_t tmp;
    while(1){


        if(read_sens_tmp(&tmp)){
            ESP_LOGI(TAG, "dato leido: %d.%d", tmp.integer, tmp.decimal);

            //una vez que ya de haya leido mandamos por medio de uart, como como saber de que tamanio sera nuestra temperatira, creo que lo mejor seria man
            //convertimos nuestro dato en ASCII para r enviado por UART 

            char buff[11]; //un buffer de 2 bytes no creo que ocue mas 

            sprintf(buff,"%d.%d",tmp.integer, tmp.decimal);
            //enviamos por uart 
            uart_write_bytes(NUM_UART, (const char*)buff, strlen(buff)); 

            ESP_LOGI(TAG, "datos enviados %s", buff);
            memset(buff,0, sizeof(buff));

        }
        else{
            ESP_LOGE(TAG, "hubo un error en la comunicacion con el sentor ");
        }
        //falta agregar else y delay

        vTaskDelay(2000/portTICK_PERIOD_MS);

    }
}

bool read_sens_tmp(temperature_t *tmp){
    uint8_t reg_addr = 0x00;
    uint8_t data[2] = {0};

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (LM75AB_ADDR << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    
    i2c_master_write_byte(cmd, (LM75AB_ADDR << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
    i2c_master_read(cmd, data, 2, I2C_MASTER_LAST_NACK); 
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(1000)); 
    i2c_cmd_link_delete(cmd);

    if(ret != ESP_OK){
        ESP_LOGE(TAG, "hubo un error al leer el sensor: %s", esp_err_to_name(ret));
        return false; 
    }

    // Misma fórmula exacta que el código que SÍ funciona
    int16_t temp_raw = (data[0] << 8) | data[1];
    temp_raw >>= 7;  // ← CLAVE: Solo 7 bits, no 5
    
    // Convertir a estructura sin floats
    tmp->integer = temp_raw >> 1;               // Dividir entre 2 para parte entera
    tmp->decimal = (temp_raw & 0x01) ? 5 : 0;   // Bit 0 = 0.5°C
    
    return true;
}