

#include <ESPmDNS.h>
#include <HTTPUpdateServer.h>
#include <WebServer.h>
#include <WiFi.h>

#include "Arduino.h"
#include "FS.h"
#include "SD_MMC.h"
#include "camera_pins.h"
#include "esp_camera.h"

int pictureNumber = 0;

extern const char *host;
extern const char *ssid;
extern const char *password;

const framesize_t GAS_FRAME_SIZE = FRAMESIZE_XGA;

WebServer server(80);
HTTPUpdateServer httpUpdater;

int countFiles(const char *dirname) {
    File root = SD_MMC.open(dirname);
    if (!root) {
        return 0;
    }

    if (!root.isDirectory()) {
        return 0;
    }

    int files = 0;
    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            files++;
        }
        file = root.openNextFile();
    }
    return files;
}

void handle_last_image() {
    String path = "/pictures/picture" + String(pictureNumber - 1) + ".jpg";
    File file = SD_MMC.open(path, FILE_READ);
    server.streamFile(file, "image/jpeg");
    file.close();
}

void handle_free_space() {
    const long unsigned int freespace = SD_MMC.totalBytes() - SD_MMC.usedBytes();
    String response = String(freespace, 10);
    server.send(200, "text/plain", response);
}

void take_picture();

void handle_take_image() {
    take_picture();
    handle_last_image();
}

void setup() {
    Serial.begin(9600);
    Serial.setDebugOutput(false);

    WiFi.mode(WIFI_AP_STA);
    WiFi.setHostname(host);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected successfully");
    Serial.print("Got IP: ");
    Serial.println(WiFi.localIP());  // Show ESP32 IP on serial

    if (MDNS.begin(host)) {
        Serial.println("mDNS responder started");
    }

    server.on("/image", handle_last_image);
    server.on("/takeimage", handle_take_image);
    server.on("/freespace", handle_free_space);

    httpUpdater.setup(&server);

    server.begin();

    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = GAS_FRAME_SIZE; 
    config.jpeg_quality = 10;
    config.fb_count = 2;

    // Init Camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    sensor_t *s = esp_camera_sensor_get();
    s->set_framesize(s, GAS_FRAME_SIZE);
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
    // apprarently it fixes setup...
    pinMode(13, INPUT_PULLUP);

    if (!SD_MMC.begin("/sdcard", true)) {
        Serial.println("SD Card Mount Failed");
    }

    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD Card attached");
        return;
    }

    pinMode(4, OUTPUT);

    Serial.println("init completed");

    if (!SD_MMC.exists("/pictures")) SD_MMC.mkdir("/pictures");

    pictureNumber = countFiles("/pictures") + 1;

    MDNS.addService("http", "tcp", 80);
}

void take_picture() {
    digitalWrite(4, HIGH);
    delay(100);

    camera_fb_t *fb = NULL;

    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }

    String path = "/pictures/picture" + String(pictureNumber++) + ".jpg";

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
}

unsigned long previousMillis = 0;
const long interval = 60000;

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        take_picture();
    }
    server.handleClient();
}
