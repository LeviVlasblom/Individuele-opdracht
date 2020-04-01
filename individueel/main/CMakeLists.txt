set(COMPONENT_REQUIRES esp32-smbus esp32-i2c-lcd1602 bluetooth ssd1306 i2cdev)
set(COMPONENT_PRIV_REQUIRES esp32-smbus esp32-i2c-lcd1602 bluetooth ssd1306 i2cdev)

set(COMPONENT_SRCS "main.c")
set(COMPONENT_ADD_INCLUDEDIRS .)

register_component()