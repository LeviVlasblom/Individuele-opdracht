//Individuele opdracht Levi Vlasblom/2139112

/*Alle code uit de main/Bluetooth component zijn geschreven door mij. 
De andere components zijn benodigde libs voor het draaien van het oled scherm*/


//i2C
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "smbus.h"
#include "i2c-lcd1602.h"
//Bluetooth
#include "bluetooth.h"
//OLED screen
#include "ssd1306.h"
#include "ssd1306_draw.h"
#include "ssd1306_font.h"
#include "ssd1306_default_if.h"

//I2C
#define I2C_MASTER_NUM                   I2C_NUM_0
#define I2C_MASTER_TX_BUF_LEN    0                     // disabled
#define I2C_MASTER_RX_BUF_LEN    0                     // disabled
#define I2C_MASTER_FREQ_HZ          20000 //100000
#define I2C_MASTER_SDA_IO              18
#define I2C_MASTER_SCL_IO               23

//SSD1306 OLED
#define SSD1306DisplayAddress           0x3C
#define SSD1306DisplayWidth               128
#define SSD1306DisplayHeight              32
#define SSD1306ResetPin                      -1

// i2c initialisation method for the config where the mode gets set to Master
static void i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;

    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE; // GY-2561 provides 10kΩ pullups
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE; // GY-2561 provides 10kΩ pullups
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_LEN, I2C_MASTER_TX_BUF_LEN, 0);
}

void app_main()
{
    //I2C init
    i2c_master_init();
    //Start bluetooth 
    bluetooth_init();
    xTaskCreate(&bluetooth_task, "bluetooth running", 8000, NULL, 3, NULL);
}