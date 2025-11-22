//libereiras basicas de C 
#include <stdio.h>
#include<string.h>

//libereias de freeRTOS
#include<freertos/FreeRTOS.h>
#include<freertos/task.h>
#include<driver/i2c.h>
#include<driver/i2c_types.h>

//manjeador de errores
#include<esp_err.h>
#include<esp_log.h>     

//libererias personlaies 
#include<modulos/I2C/i2c_lib.h>



//macros 
#define I2C_SDA_IO 21
#define I2C_SCL_IO 22
#define SPEED 100000
#define I2C_PORT I2C_NUM_0
#define LM75AB_ADDR 0x48
#define ACK_CHECK_EN 0x1


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


    xTaskCreate(main_task, "main_task_temp", 4098, NULL, 10, NULL);

}


void main_task(void *params){

    temperature_t tmp;
    while(1){


        if(read_sens_tmp(&tmp)){
            ESP_LOGI(TAG, "dato leido: %d.%d", tmp.decimal, tmp.integer);

        }
        else{
            ESP_LOGE(TAG, "hubo un error en la comunicacion con el sentor ");
        }
        //falta agregar else y delay

        vTaskDelay(2000/portTICK_PERIOD_MS);

    }
}



bool read_sens_tmp(temperature_t *tmp){

    //este recibe un apuntador a un tipo de dato de 16 bits entero con signo ya que el dato que nos regresa el senero puede ser tanto positivo como negastivo 
    //y es un dato de 16 bits 

    uint8_t reg_addr = 0x00;
    uint8_t data[2] = {0}; //inicamos un arreglo con la infromacion que vamos a recibir del sensor 


    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    //bit de inicio 
    i2c_master_start(cmd);
    //se le esta indicando que queremos leer, por lo que en el bus de datos mandamos que queremos leer de la direccion que se marca, en I2C primero se manda la direccion 
    // del esclavo 

    // Escribir la direccion del sensor y el registro que queremos leer
    i2c_master_write_byte(cmd, (LM75AB_ADDR << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_addr, true);
    //reiniciamos la comunicacion para leer
    i2c_master_start(cmd);
    

    i2c_master_write_byte(cmd, (LM75AB_ADDR << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
    i2c_master_read(cmd, data, 2, I2C_MASTER_LAST_NACK); 

    i2c_master_stop(cmd);

    esp_err_t ret =   i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(1000)); 
    i2c_cmd_link_delete(cmd); //para liberar el bus 

    if(ret != ESP_OK){
        ESP_LOGE(TAG, "hubo un error al leer el senror: %s", esp_err_to_name(ret));
        return false; 
    }

    
    int16_t temp = (data[0] << 8) | data[1];

    int8_t temp_integer = (int8_t)data[0]; //toma la parte entera 

    uint8_t temp_decimal = (data[1] & 0x80) >> 7;

    tmp->integer = temp_integer;
    tmp->decimal = temp_decimal * 5;

    return true;
}