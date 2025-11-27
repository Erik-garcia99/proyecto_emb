#ifndef I2C_LIB_H
#define I2C_LIB_H

//librerias que necesita 
#include<driver/i2c.h>
#include<driver/i2c_types.h>


/**
 * 
 * @brief libereia personal que se encarga de establecer la comuinicacion por I2C enter el ESP con el sensor en modo maestro y mandar los datos al ESP_AP pro medio de UART  
 * 
 * 
 * 
*/

/**
 *
 * @brief inicalizacion del I2C
 *
 * @param num_i2c -> numero del I2C a utulizar (I2C_NUM_0 - I2C_NUM_1)
 * @param pin_sda -> pin GPIO para los datos
 * @param pin_scl -> pin GPIO para el reloj
 * @param slave_addr -> direccion del dispositovo esclavo
 * @param bus_handle -> salida, controlador de bus I2C MASTER
 * @param dev_handle -> identificador del salve
 *
 *
*/
void i2c_master_init(i2c_port_num_t num_i2c, gpio_num_t pin_sda, gpio_num_t pin_scl, uint32_t speed);

/**
 *
 * @brief inicinado I2C slave
 * @param num_i2c -> numero del I2C a utulizar (I2C_NUM_0 - I2C_NUM_1)
 * @param pin_sda -> pin GPIO para los datos
 * @param pin_scl -> pin GPIO para el reloj
 * @param slave_addr -> direccion del dispositovo esclavo
 * @param slave_handle  -> bus de identificacion de slave
 * @param buff_tx -> tamanio del buffer circular para enviar datos
 * @param buff_rx -> tamanio del buffer del software para recepcion
 *
 *
 *
 *
 */

void i2C_slave_init(i2c_port_num_t num_i2c, gpio_num_t pin_sda, gpio_num_t pin_scl, uint8_t slave_addr);

#endif