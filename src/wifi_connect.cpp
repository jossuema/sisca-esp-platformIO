#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "wifi_connect.h"
#include "config.h"

static const char *TAG = "WIFI";

static bool timeConfigured = false;

void wifi_init(void) {
    Serial.printf("[%s] Iniciando WiFi (no bloqueante) hacia \"%s\"...\n", TAG, WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Configura NTP una sola vez; SNTP sincronizará en cuanto haya conectividad.
    // La validación del certificado TLS (http_token_verify) necesita la hora.
    // UTC; el huso no afecta la comprobación de fechas del certificado.
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    timeConfigured = true;
}

bool wifi_ready(void) {
    return WiFi.status() == WL_CONNECTED && time(nullptr) > 1700000000;  // hora >= 2023
}

void wifi_loop(void) {
    static bool     wasConnected = false;
    static uint32_t lastRetry    = 0;

    bool connected = (WiFi.status() == WL_CONNECTED);

    if (connected && !wasConnected) {
        wasConnected = true;
        Serial.printf("[%s] WiFi conectado. IP: %s\n", TAG, WiFi.localIP().toString().c_str());
    } else if (!connected && wasConnected) {
        wasConnected = false;
        Serial.printf("[%s] WiFi perdido; reintentando en segundo plano\n", TAG);
    }

    if (!connected) {
        uint32_t now = millis();
        if (now - lastRetry >= WIFI_RETRY_MS) {
            lastRetry = now;
            WiFi.reconnect();
        }
    }
}
