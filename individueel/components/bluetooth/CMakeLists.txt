set(COMPONENT_REQUIRES audio_pipeline audio_stream esp_peripherals nvs_flash bluetooth_service esp32-smbus esp32-i2c-lcd1602 ssd1306 i2cdev)
set(COMPONENT_PRIV_REQUIRES audio_pipeline audio_stream esp_peripherals nvs_flash bluetooth_service esp32-smbus esp32-i2c-lcd1602 ssd1306 i2cdev)

set(COMPONENT_SRCS "bluetooth.c")
set(COMPONENT_ADD_INCLUDEDIRS include)

register_component()