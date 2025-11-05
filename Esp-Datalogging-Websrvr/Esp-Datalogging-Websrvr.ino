#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "LittleFS.h"

// Replace with your network credentials
const char* ssid = "Galaxy A529DB7";
const char* password = "shadowless12";
// const char* ssid = "4G-MIFI-621A";
// const char* password = "//123Sewornu123//";

#define DHTPIN 4
#define RAINSENSOR_PIN 34
#define SOILMOISTURE_PIN 35
#define LDR_PIN 21
#define PH_PIN 32
#define WATER_LEVEL_PIN 33
#define PH_UP_PIN 25
#define PH_DOWN_PIN 26
#define STIRRER_PIN 2
#define PUMP_PIN 14
#define LIGHT_INTENSITY 27

// Variables
unsigned long epochTime = 0;
bool loggingEnabled = true;
String dataMessage;
const char* dataPath = "/data.txt";
unsigned long lastLogTime = 0;
const unsigned long logInterval = 500; // Log every 5 seconds

AsyncWebServer server(80);

// Initialize SD Card
void initSDCard() {
    if (!SD.begin(5)) {
        Serial.println("SD Card Mount Failed. Logging disabled.");
        loggingEnabled = false; // Prevent logging if SD card fails
    }
}

// Write Data to SD
void appendFile(fs::FS &fs, const char *path, const char *message) {
    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return;
    }
    file.print(message);
    file.close();
}

// Read SD File for Webpage
String readFile(fs::FS &fs, const char *path) {
    File file = fs.open(path);
    if (!file) return "";
    String data;
    while (file.available()) {
        data += file.readStringUntil('\n') + "\n";
    }
    file.close();
    return data;
}

// Delete Data File
void deleteFile(fs::FS &fs, const char *path) {
    if (fs.remove(path)) {
        Serial.println("Data file deleted.");
    }
}

// Initialize WiFi with Timeout
void initWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 30000) { // 30 seconds timeout
        delay(1000);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected: " + WiFi.localIP().toString());
    } else {
        Serial.println("\nWiFi Connection Failed! Restarting...");
        ESP.restart();
    }
}

// Handle Web Requests
void setupServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", "text/html");
    });

    server.on("/view-data", HTTP_GET, [](AsyncWebServerRequest *request) {
        String data = readFile(SD, dataPath);
        request->send(200, "text/plain", data);
    });

    server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request) {
        deleteFile(SD, dataPath);
        request->send(200, "text/plain", "Data deleted.");
    });

    server.on("/toggle-logging", HTTP_GET, [](AsyncWebServerRequest *request) {
        loggingEnabled = !loggingEnabled;
        request->send(200, "text/plain", loggingEnabled ? "Logging Enabled" : "Logging Paused");
    });

    server.begin();
}

void setup() {
    Serial.begin(115200);
    
    if (!LittleFS.begin()) {
        Serial.println("LittleFS Mount Failed!");
    }

    initWiFi();
    initSDCard();
    setupServer();
}

void loop() {
    if (loggingEnabled && millis() - lastLogTime >= logInterval) {
        lastLogTime = millis();
        epochTime = millis(); // Timestamp

        // Generate Dummy Sensor Data
        float dht = random(200, 300) / 10.0;
        float rain = random(0, 100);
        float soil = random(200, 800) / 10.0;
        float ldr = random(0, 1023);
        float ph = random(50, 90) / 10.0;
        float water_level = random(0, 100);
        float ph_up = random(0, 2);   // Fixed Boolean Randomization
        float ph_down = random(0, 2);
        float stirrer = random(0, 2);
        float pump = random(0, 2);
        float light_intensity = random(0, 1023);

        // Create Data String
        dataMessage = String(epochTime) + "," + String(dht) + "," + String(rain) + "," +
                      String(soil) + "," + String(ldr) + "," + String(ph) + "," +
                      String(water_level) + "," + String(ph_up) + "," + 
                      String(ph_down) + "," + String(stirrer) + "," + 
                      String(pump) + "," + String(light_intensity) + "\r\n";

        // Save Data
        appendFile(SD, dataPath, dataMessage.c_str());

        Serial.println("Data Logged: " + dataMessage);
    }
}
