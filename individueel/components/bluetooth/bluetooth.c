
#include "driver/gpio.h"
#include "driver/i2c.h"

#include "smbus.h"
#include "i2c-lcd1602.h"
#include "bluetooth.h"
#include "ssd1306.h"
#include "ssd1306_draw.h"
#include "ssd1306_font.h"
#include "ssd1306_default_if.h"

//Draw methods for SSD1306
// void SetupDemo(struct SSD1306_Device *DisplayHandle, const struct SSD1306_FontDef *Font);
// void SayHello(struct SSD1306_Device *DisplayHandle, const char *HelloText);

struct SSD1306_Device SSD1306Display;

//SSD1306 OLED
#define SSD1306DisplayAddress           0x3C
#define SSD1306DisplayWidth               128
#define SSD1306DisplayHeight              32
#define SSD1306ResetPin                      -1

static const char *BLUETTAG = "BLUETOOTH";

audio_event_iface_handle_t evt; 
audio_pipeline_handle_t pipeline;
audio_element_handle_t bt_stream_reader, i2s_stream_writer;
esp_periph_set_handle_t set;
esp_periph_handle_t bluetooth_periph;

//SSD1306 draw methods
bool DefaultBusInit(void)
{
    assert(SSD1306_I2CMasterInitDefault() == true);
    assert(SSD1306_I2CMasterAttachDisplayDefault(&SSD1306Display, SSD1306DisplayWidth, SSD1306DisplayHeight, SSD1306DisplayAddress, SSD1306ResetPin) == true);
    return true;
}

void SSD1306Setup(struct SSD1306_Device *DisplayHandle, const struct SSD1306_FontDef *Font)
{
    SSD1306_Clear(DisplayHandle, SSD_COLOR_BLACK);
    SSD1306_SetFont(DisplayHandle, Font);
}

void SSD1306Draw(struct SSD1306_Device *DisplayHandle, const char *HelloText)
{
    SSD1306_FontDrawAnchoredString(DisplayHandle, TextAnchor_Center, HelloText, SSD_COLOR_WHITE);
    SSD1306_Update(DisplayHandle);
}
//end SSD1306 draw methods


//Test task created for possible method for handeling the oled
// void testC_task(void* pvParameter){
//     if(DefaultBusInit() == true){
//     SSD1306Setup(&SSD1306Display, &Font_droid_sans_fallback_24x28);
//     SSD1306Draw(&SSD1306Display, "Connected");
//     vTaskDelete(NULL);
//     }
// }
// void testD_task(void* pvParameter){
//     if(DefaultBusInit() == true){
//     SSD1306Setup(&SSD1306Display, &Font_droid_sans_fallback_24x28);
//     SSD1306Draw(&SSD1306Display, "Disconnected");
//     vTaskDelete(NULL);
//     }

// }

//Oled ssd1306 callback method to switch text for user friendly display.
void oled_Callback(int state)
{
    if (DefaultBusInit() == true)
    {
        switch (state)
        {
        case 0:
            SSD1306_Clear(&SSD1306Display, SSD_COLOR_BLACK);
            SSD1306Setup(&SSD1306Display, &Font_droid_sans_fallback_24x28);
            SSD1306Draw(&SSD1306Display, "Disconnected");
            break;
        case 1:
        SSD1306_Clear(&SSD1306Display, SSD_COLOR_BLACK);
        SSD1306Setup(&SSD1306Display, &Font_droid_sans_fallback_24x28);
        SSD1306Draw(&SSD1306Display, "Connected");
            break;
        case 2:
        SSD1306_Clear(&SSD1306Display, SSD_COLOR_BLACK);
        SSD1306Setup(&SSD1306Display, &Font_droid_sans_fallback_24x28);
        SSD1306Draw(&SSD1306Display, "Connected");
            break;
        default:
        SSD1306_Clear(&SSD1306Display, SSD_COLOR_BLACK);
        SSD1306Setup(&SSD1306Display, &Font_droid_sans_fallback_24x28);
        SSD1306Draw(&SSD1306Display, "Disconnected");
            break;
        }
    }  
}

//initialisation for bluetooth task for handeling audio streams.
void bluetooth_init(void)
{
    oled_Callback(0);
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(BLUETTAG, ESP_LOG_DEBUG);


    /*Sets the correct bluetooth config. Sets the mode to A2DP("Advanced Audio Distribution Profile")
    This describes how Bluetooth devices can stream stereo-quality audio to remote devices.*/
    ESP_LOGI(BLUETTAG, "[ 1 ] Create Bluetooth service");
    bluetooth_service_cfg_t bluetooth_cfg = {
        .device_name = "ESP-ADF-SPEAKER",
        .mode = BLUETOOTH_A2DP_SINK,
    };
    bluetooth_service_start(&bluetooth_cfg);


    /*initialisation for the Audio_board*/
    ESP_LOGI(BLUETTAG, "[ 2 ] Start codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    /*Sets the config for the audio pipeline to default and initialises*/
    ESP_LOGI(BLUETTAG, "[ 3 ] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);

    /*Sets the config for i2s stream to default. and sets the type to stream writer*/
    ESP_LOGI(BLUETTAG, "[3.1] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    /*sets the Bluetooth stream reader open*/
    ESP_LOGI(BLUETTAG, "[3.2] Get Bluetooth stream");
    bt_stream_reader = bluetooth_service_create_stream();

    ESP_LOGI(BLUETTAG, "[3.2] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, bt_stream_reader, "bt");
    audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");

    ESP_LOGI(BLUETTAG, "[3.3] Link it together [Bluetooth]-->bt_stream_reader-->i2s_stream_writer-->[codec_chip]");

    audio_pipeline_link(pipeline, (const char *[]) {"bt", "i2s"}, 2);
    /*Sets the esp peripherals to default*/
    ESP_LOGI(BLUETTAG, "[ 4 ] Initialize peripherals");
    esp_periph_config_t peripherals_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    set = esp_periph_set_init(&peripherals_cfg);

    /*Initializes the key's on the esp board*/    
    ESP_LOGI(BLUETTAG, "[4.1] Initialize Touch peripheral");
    audio_board_key_init(set);

    /*Sets the bluetooth peripherals to default*/
    ESP_LOGI(BLUETTAG, "[4.2] Create Bluetooth peripheral");
    bluetooth_periph = bluetooth_service_create_periph();

    /*Starts the bluetooth peripherals*/
    ESP_LOGI(BLUETTAG, "[4.2] Start all peripherals");
    esp_periph_start(set, bluetooth_periph);

    /*Sets the audio event listener to default*/
    ESP_LOGI(BLUETTAG, "[ 5 ] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    evt = audio_event_iface_init(&evt_cfg);

    /*Sets listener events for both the pipeline and periphs*/
    ESP_LOGI(BLUETTAG, "[5.1] Listening event from all elements of pipeline");
    audio_pipeline_set_listener(pipeline, evt);
    ESP_LOGI(BLUETTAG, "[5.2] Listening event from peripherals");
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);

    /*Runs the audio pipeline*/
    ESP_LOGI(BLUETTAG, "[ 6 ] Start audio_pipeline");
    audio_pipeline_run(pipeline);

    /*After 6 it will listen to all events for a device to connect.
    Run the Main bluetooth task after this to handle all the connections and audio streams*/
    ESP_LOGI(BLUETTAG, "[ 7 ] Listen for all pipeline events");
    vTaskDelay(1000 / portTICK_RATE_MS);
    
}

/*Main task for the bluetooth. This will listen for a device to be conected. 
If device is connected it will read any audio stream comming from the device. */
void bluetooth_task(void *pvParameter)
{
    /*Main loop to listen to events*/
    while (1) {

        /*If statements for checking errors in the event*/
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(BLUETTAG, "[ * ] Event interface error : %d", ret);
            continue;
        }
        
        if (msg.cmd == AEL_MSG_CMD_ERROR) {
            ESP_LOGE(BLUETTAG, "[ * ] Action command error: src_type:%d, source:%p cmd:%d, data:%p, data_len:%d",
                     msg.source_type, msg.source, msg.cmd, msg.data, msg.data_len);
                     
        }
        
        /*handels the audio stream. checks what audio is comming through.
        giving callback to the oled display for state of the connection*/
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) bt_stream_reader
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
                oled_Callback(1);
            audio_element_info_t audioPlayed_info = {0};
            audio_element_getinfo(bt_stream_reader, &audioPlayed_info);
            ESP_LOGI(BLUETTAG, "[ * ] Receive music info from Bluetooth, sample_rates=%d, bits=%d, ch=%d",
                     audioPlayed_info.sample_rates, audioPlayed_info.bits, audioPlayed_info.channels);        
            audio_element_setinfo(i2s_stream_writer, &audioPlayed_info);
            i2s_stream_set_clk(i2s_stream_writer, audioPlayed_info.sample_rates, audioPlayed_info.bits, audioPlayed_info.channels);
            oled_Callback(2);
            continue;
        }

        /*Handeling the touch buttons*/
        if (msg.source_type == PERIPH_ID_TOUCH
            && msg.cmd == PERIPH_TOUCH_TAP
            && msg.source == (void *)esp_periph_set_get_by_id(set, PERIPH_ID_TOUCH)) {

            if ((int) msg.data == get_input_play_id()) {
                ESP_LOGI(BLUETTAG, "[ * ] [Play] touch tap event");
                periph_bluetooth_play(bt_periph);
            } else if ((int) msg.data == get_input_set_id()) {
                ESP_LOGI(BLUETTAG, "[ * ] [Set] touch tap event");
                periph_bluetooth_pause(bt_periph);
            } else if ((int) msg.data == get_input_volup_id()) {
                ESP_LOGI(BLUETTAG, "[ * ] [Vol+] touch tap event");
                periph_bluetooth_next(bt_periph);
            } else if ((int) msg.data == get_input_voldown_id()) {
                ESP_LOGI(BLUETTAG, "[ * ] [Vol-] touch tap event");
                periph_bluetooth_prev(bt_periph);
            }
        }

        /* Stop when the Bluetooth is disconnected or suspended */
        if (msg.source_type == PERIPH_ID_BLUETOOTH
            && msg.source == (void *)bt_periph) {
            if (msg.cmd == PERIPH_BLUETOOTH_DISCONNECTED) {
                ESP_LOGW(BLUETTAG, "[ * ] Bluetooth disconnected");
                oled_Callback(0);
                break;
            }
        }
        /* Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event */
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) i2s_stream_writer
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))) {
            ESP_LOGW(BLUETTAG, "[ * ] Stop event received");
            break;
        }
    }
    bluetooth_deinit();
    vTaskDelete(NULL);
}

/*Method for handeling the deinitialize*/
void bluetooth_deinit(void)
{

    ESP_LOGI(BLUETTAG, "[ 8 ] Stopping and removing pipelines");
    audio_pipeline_terminate(pipeline);

    audio_pipeline_unregister(pipeline, bt_stream_reader);
    audio_pipeline_unregister(pipeline, i2s_stream_writer);

    /* Terminate the pipeline before removing the listener */
    audio_pipeline_remove_listener(pipeline);

    /* Stop all peripherals before removing the listener */
    esp_periph_set_stop_all(set);
    audio_event_iface_remove_listener(esp_periph_set_get_event_iface(set), evt);

    /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    audio_event_iface_destroy(evt);

    /* Release all resources */
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(bt_stream_reader);
    audio_element_deinit(i2s_stream_writer);
    esp_periph_set_destroy(set);
    bluetooth_service_destroy();
}
