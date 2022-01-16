#include <Arduino.h>
#include <SD_MMC.h>
#include <esp_camera.h>

#include "camera_pins.h"


const framesize_t GAS_FRAME_SIZE = FRAMESIZE_XGA;

extern "C" { esp_err_t camera_enable_out_clock(camera_config_t *config); void camera_disable_out_clock(); }
    
camera_config_t camera_config;


void config_camera() {

    camera_config.ledc_channel = LEDC_CHANNEL_0;
    camera_config.ledc_timer = LEDC_TIMER_0;
    camera_config.pin_d0 = Y2_GPIO_NUM;
    camera_config.pin_d1 = Y3_GPIO_NUM;
    camera_config.pin_d2 = Y4_GPIO_NUM;
    camera_config.pin_d3 = Y5_GPIO_NUM;
    camera_config.pin_d4 = Y6_GPIO_NUM;
    camera_config.pin_d5 = Y7_GPIO_NUM;
    camera_config.pin_d6 = Y8_GPIO_NUM;
    camera_config.pin_d7 = Y9_GPIO_NUM;
    camera_config.pin_xclk = XCLK_GPIO_NUM;
    camera_config.pin_pclk = PCLK_GPIO_NUM;
    camera_config.pin_vsync = VSYNC_GPIO_NUM;
    camera_config.pin_href = HREF_GPIO_NUM;
    camera_config.pin_sscb_sda = SIOD_GPIO_NUM;
    camera_config.pin_sscb_scl = SIOC_GPIO_NUM;
    camera_config.pin_pwdn = PWDN_GPIO_NUM;
    camera_config.pin_reset = RESET_GPIO_NUM;
    camera_config.xclk_freq_hz = 20000000;
    camera_config.pixel_format = PIXFORMAT_JPEG;
    camera_config.frame_size = GAS_FRAME_SIZE; 
    camera_config.jpeg_quality = 10;
    camera_config.fb_count = 2;

    // Init Camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    sensor_t *s = esp_camera_sensor_get();
    s->set_framesize(s, GAS_FRAME_SIZE);
    // s->set_vflip(s, 1);
    // s->set_hmirror(s, 1);
    // apprarently it fixes setup...
    pinMode(13, INPUT_PULLUP);

    //set flash
    pinMode(4, OUTPUT);

    camera_disable_out_clock();
}


void take_picture(int sequenceId) {
    camera_enable_out_clock(&camera_config);
    digitalWrite(4, HIGH);
    delay(100);

    camera_fb_t *fb = NULL;

    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }

    String path = "/pictures/picture" + String(sequenceId++) + ".jpg";

    fs::FS &fs = SD_MMC;
    Serial.printf("Picture file name: %s\n", path.c_str());

    File file = fs.open(path.c_str(), FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file in writing mode");
    } else {
        file.write(fb->buf, fb->len);
        Serial.printf("Saved file to path: %s\n", path.c_str());
    }

    file.close();
    esp_camera_fb_return(fb);
    digitalWrite(4, LOW);
    camera_disable_out_clock();
}