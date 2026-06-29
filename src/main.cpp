#include <Arduino.h>
#include "esp_task_wdt.h"
#include "wifi_connect.h"
#include "ble_server.h"
#include "token_logic.h"
#include "device_config.h"
#include "config.h"

static const char *TAG = "MAIN";

void setup() {
    Serial.begin(115200);
    delay(100);

    // ecodigo del aula desde NVS (aprovisionable por Serial), antes del BLE.
    device_config_begin();

    // GPIO de la cerradura en estado seguro (cerrada) cuanto antes.
    token_logic_init();

    // WiFi NO bloqueante: el BLE queda disponible aunque no haya red todavía.
    Serial.printf("[%s] Iniciando WiFi (no bloqueante)...\n", TAG);
    wifi_init();

    Serial.printf("[%s] Iniciando Bluetooth BLE (GATT)...\n", TAG);
    ble_init();

    // Watchdog de tarea sobre loop(). La verificación HTTP (lenta) corre en una
    // tarea aparte, así que loop() nunca bloquea y no dispara el WDT. El timeout
    // efectivo es el del sistema (5 s) si el TWDT ya estaba inicializado; en
    // entornos donde no lo esté, esta llamada lo fija en WDT_TIMEOUT_S.
    esp_task_wdt_init(WDT_TIMEOUT_S, true);
    esp_task_wdt_add(NULL);
    Serial.printf("[%s] Watchdog activo sobre loop()\n", TAG);
}

void loop() {
    esp_task_wdt_reset();           // alimenta el watchdog

    lock_tick();                    // abre/cierra la cerradura (no bloqueante)
    wifi_loop();                    // mantiene/reintenta la conexión WiFi
    device_config_serial_tick();    // comandos de aprovisionamiento por Serial

    vTaskDelay(pdMS_TO_TICKS(10));
}
