#include <Arduino.h>
#include <ArduinoJson.h>
#include "token_logic.h"
#include "http_token_verify.h"
#include "config.h"

#define GPIO_CERRADURA 18  // Ajusta el pin si usas otro

static const char *TAG = "TOKEN_LOGIC";
static bool gpio_initialized = false;

void handle_received_json(const char *data) {
    Serial.printf("[%s] Raw received: %s\n", TAG, data);

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data);
    if (err) {
        Serial.printf("[%s] JSON parse error: %s\n", TAG, err.c_str());
        return;
    }

    const char *user  = doc["user"];
    const char *token = doc["token"];

    if (user && token) {
        Serial.printf("[%s] User: %s\n", TAG, user);
        Serial.printf("[%s] Token: %s\n", TAG, token);

        if (!gpio_initialized) {
            pinMode(GPIO_CERRADURA, OUTPUT);
            digitalWrite(GPIO_CERRADURA, LOW);
            gpio_initialized = true;
        }

        if (verify_token(user, token)) {
            Serial.printf("[%s] Token válido\n", TAG);
            digitalWrite(GPIO_CERRADURA, HIGH);   // Activa
            vTaskDelay(pdMS_TO_TICKS(2000));      // Espera 2 segundos
            digitalWrite(GPIO_CERRADURA, LOW);    // Desactiva
            Serial.printf("[%s] Cerradura activada\n", TAG);
        } else {
            Serial.printf("[%s] Token inválido\n", TAG);
        }
    } else {
        Serial.printf("[%s] Invalid JSON format\n", TAG);
    }
}
