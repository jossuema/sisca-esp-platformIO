#include <Arduino.h>
#include <WiFi.h>
#include "wifi_connect.h"
#include "config.h"

static const char *TAG = "WIFI";

void wifi_connect(void) {
    Serial.printf("[%s] Conectando a \"%s\"...\n", TAG, WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Bloqueante hasta conectar (igual que el wifi_connect() original).
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }
    Serial.println();

    Serial.printf("[%s] WiFi conectado. IP: %s\n", TAG, WiFi.localIP().toString().c_str());
}
