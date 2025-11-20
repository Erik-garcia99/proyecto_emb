//libereias para usar I2C 

#include<driver/i2c.h>
#include<driver/i2c_types.h>

//manejador de erores 
#include<esp_err.h>
#include<esp_log.h>

//mi libreria 
#include<modulos/I2C/i2c_lib.h>

static const char* TAG="ESP_SENS_LM75AB";


//se necesita el master y el slave porque el master dice quiero leer y el slave responde trayendo la lectura del sensro LM75AB 
//esta API es la viejita
void i2c_master_init(i2c_port_num_t num_i2c, gpio_num_t pin_sda, gpio_num_t pin_scl, uint32_t speed){
    i2c_config_t conf_master = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = pin_sda,
        .scl_io_num = pin_scl,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = speed,
    };

    esp_err_t ret = i2c_param_config(num_i2c, &conf_master);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C master param config failed: %s", esp_err_to_name(ret));
    }

    ret = i2c_driver_install(num_i2c, conf_master.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C master driver install failed: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "Bus I2C maestro para sensor inicializado correctamente");


}