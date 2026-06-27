#include <Arduino.h>
#include "wifi_connect.h"
#include "spp_server.h"

static const char *TAG = "MAIN";

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.printf("[%s] Inicializando conexión WiFi...\n", TAG);
    wifi_connect();  // Bloqueante hasta conectarse

    Serial.printf("[%s] Inicializando Bluetooth SPP...\n", TAG);
    spp_init();
}

void loop() {
    // El servidor SPP se atiende desde el bucle principal de Arduino.
    spp_loop();
    vTaskDelay(pdMS_TO_TICKS(10));
}
