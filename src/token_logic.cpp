#include <Arduino.h>
#include <ArduinoJson.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "token_logic.h"
#include "http_token_verify.h"
#include "ble_server.h"
#include "device_config.h"
#include "wifi_connect.h"
#include "config.h"

static const char *TAG = "TOKEN_LOGIC";

// ----------------------------------------------------------------------------
//  Arquitectura no bloqueante:
//   - loop() (vigilado por el watchdog de 5 s) solo hace trabajo rápido y encola
//     el JSON recibido; nunca bloquea -> nunca dispara el watchdog.
//   - una TAREA worker (no vigilada por el WDT) hace la verificación HTTP, que
//     puede tardar; si el servidor se cuelga, el equipo NO se reinicia.
//   - el GPIO/timer de la cerradura tiene un único dueño (loop, vía lock_tick):
//     el worker solo solicita la apertura con un flag.
// ----------------------------------------------------------------------------
static QueueHandle_t  verifyQueue = nullptr;
static volatile bool  gRequestOpen = false;

static bool     lockActive = false;
static uint32_t lockUntil  = 0;

void lock_tick(void) {
    if (gRequestOpen) {            // solicitud del worker: abre / re-arma
        gRequestOpen = false;
        digitalWrite(GPIO_CERRADURA, HIGH);
        lockActive = true;
        lockUntil  = millis() + LOCK_OPEN_MS;
        Serial.printf("[%s] Cerradura activada (%d ms)\n", TAG, LOCK_OPEN_MS);
    }
    if (lockActive && (int32_t)(millis() - lockUntil) >= 0) {
        digitalWrite(GPIO_CERRADURA, LOW);
        lockActive = false;
        Serial.printf("[%s] Cerradura desactivada\n", TAG);
    }
}

// ----------------------------------------------------------------------------
//  Anti-replay (solo accedido desde el worker task): rechaza el mismo token si
//  vuelve a llegar dentro de REPLAY_WINDOW_MS. Defensa en profundidad frente al
//  reenvío de un JSON capturado.
// ----------------------------------------------------------------------------
#define REPLAY_CACHE_SIZE 16
static String   recentTokens[REPLAY_CACHE_SIZE];
static uint32_t recentStamps[REPLAY_CACHE_SIZE];
static int      recentCount = 0;

static bool is_replay(const char *token) {
    uint32_t nowMs = millis();
    for (int i = 0; i < recentCount; ++i) {
        if ((uint32_t)(nowMs - recentStamps[i]) < REPLAY_WINDOW_MS &&
            recentTokens[i] == token) {
            return true;
        }
    }
    return false;
}

static void remember_token(const char *token) {
    uint32_t nowMs = millis();
    // Si el token ya está registrado, solo refresca su marca (evita duplicados).
    for (int i = 0; i < recentCount; ++i) {
        if (recentTokens[i] == token) {
            recentStamps[i] = nowMs;
            return;
        }
    }
    // Reutiliza una entrada caducada si la hay.
    for (int i = 0; i < recentCount; ++i) {
        if ((uint32_t)(nowMs - recentStamps[i]) >= REPLAY_WINDOW_MS) {
            recentTokens[i] = token;
            recentStamps[i] = nowMs;
            return;
        }
    }
    if (recentCount < REPLAY_CACHE_SIZE) {
        recentTokens[recentCount] = token;
        recentStamps[recentCount] = nowMs;
        recentCount++;
    } else {
        int oldest = 0;
        for (int i = 1; i < REPLAY_CACHE_SIZE; ++i) {
            if (recentStamps[i] < recentStamps[oldest]) oldest = i;
        }
        recentTokens[oldest] = token;
        recentStamps[oldest] = nowMs;
    }
}

// Procesa un JSON ya recibido (corre en el worker task). Verifica el token y, si
// es válido, solicita la apertura y responde el ACK por BLE NOTIFY.
static void process_json(const char *data) {
    Serial.printf("[%s] Procesando: %s\n", TAG, data);

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data);
    if (err) {
        Serial.printf("[%s] JSON parse error: %s\n", TAG, err.c_str());
        ble_notify("{\"valido\":0,\"error\":\"formato\"}");
        return;
    }

    const char *user  = doc["user"];
    const char *token = doc["token"];
    if (!user || !token) {
        Serial.printf("[%s] Formato JSON inválido\n", TAG);
        ble_notify("{\"valido\":0,\"error\":\"formato\"}");
        return;
    }

    Serial.printf("[%s] User: %s | Token: %s\n", TAG, user, token);

    if (is_replay(token)) {
        Serial.printf("[%s] Token repetido (replay) ignorado\n", TAG);
        ble_notify("{\"valido\":0,\"error\":\"replay\"}");
        return;
    }

    if (!wifi_ready()) {
        Serial.printf("[%s] Sin red/hora; no se puede verificar\n", TAG);
        ble_notify("{\"valido\":0,\"error\":\"sin_red\"}");
        return;
    }

    if (verify_token(user, token, device_get_ecodigo())) {
        Serial.printf("[%s] Token válido\n", TAG);
        remember_token(token);
        gRequestOpen = true;       // lock_tick (loop) abre la cerradura
        ble_notify("{\"valido\":1}");
    } else {
        Serial.printf("[%s] Token inválido\n", TAG);
        ble_notify("{\"valido\":0}");
    }
}

static void verify_task(void *arg) {
    char *item = nullptr;
    for (;;) {
        if (xQueueReceive(verifyQueue, &item, portMAX_DELAY) == pdTRUE && item) {
            process_json(item);
            free(item);
            item = nullptr;
        }
    }
}

void token_logic_init(void) {
    pinMode(GPIO_CERRADURA, OUTPUT);
    digitalWrite(GPIO_CERRADURA, LOW);  // estado seguro: cerrada
    lockActive = false;

    verifyQueue = xQueueCreate(4, sizeof(char *));
    // Pila amplia: WiFiClientSecure (TLS/mbedTLS) + HTTPClient consumen bastante stack.
    xTaskCreatePinnedToCore(verify_task, "verify", 12288, nullptr, 1, nullptr, APP_CPU_NUM);
}

// Llamado desde loop() (ble_loop): NO bloquea. Copia el JSON y lo encola para el
// worker; si la cola está llena, descarta para no bloquear.
void handle_received_json(const char *data) {
    if (verifyQueue == nullptr) return;
    char *copy = strdup(data);
    if (copy == nullptr) return;
    if (xQueueSend(verifyQueue, &copy, 0) != pdTRUE) {
        Serial.printf("[%s] Cola llena; mensaje descartado\n", TAG);
        free(copy);
    }
}
