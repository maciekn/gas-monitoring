
#include "Arduino.h"
#include <ESPmDNS.h>
#include <HTTPUpdateServer.h>
#include <WebServer.h>
#include <WiFi.h>


#include "FS.h"
#include <SD_MMC.h>
#include "camera_utils.h"


int pictureNumber = 0;

extern const char *host;
extern const char *ssid;
extern const char *password;


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

void take_picture_wrapper(){
    take_picture(pictureNumber++);
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

void handle_cpu_freq() {
    const uint32_t freq = getCpuFrequencyMhz();
    String response = String(freq, 10);
    server.send(200, "text/plain", response);
}

void handle_take_image() {
    take_picture_wrapper();
    handle_last_image();
}

void setup() {
    Serial.begin(9600);
    Serial.println("Hello, world!");
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

    config_camera();

    server.on("/image", handle_last_image);
    server.on("/takeimage", handle_take_image);
    server.on("/freespace", handle_free_space);
    server.on("/cpufreq", handle_cpu_freq);

    httpUpdater.setup(&server);

    server.begin();


    if (!SD_MMC.begin("/sdcard", true)) {
        Serial.println("SD Card Mount Failed");
    }

    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD Card attached");
        return;
    }
    
    Serial.println("init completed");

    if (!SD_MMC.exists("/pictures")) SD_MMC.mkdir("/pictures");
    pictureNumber = countFiles("/pictures") + 1;

    MDNS.addService("http", "tcp", 80);

    digitalWrite(4, HIGH);
    delay(50);
    digitalWrite(4, LOW);
    delay(150);
    digitalWrite(4, HIGH);
    delay(100);
    digitalWrite(4, LOW);
}



unsigned long previousMillis = 0;
const unsigned long interval = 1000 * 60 * 20;


void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {

        previousMillis = currentMillis;
        take_picture_wrapper();
    } else if (currentMillis < previousMillis) {
        previousMillis = 0;
    }

    server.handleClient();


}
