#include <Arduino.h>
#include "BluetoothSerial.h"
#include "spp_server.h"
#include "token_logic.h"
#include "config.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth Classic no está habilitado. Usa un ESP32 clásico (no S2/S3/C3).
#endif

static BluetoothSerial SerialBT;

// Reensamblado del JSON: el flujo SPP puede llegar fragmentado, así que
// acumulamos por conteo de llaves hasta tener un objeto JSON completo. Se
// rastrea el contexto de string/escape para no contar llaves que aparezcan
// dentro de un valor (p. ej. {"user":"a}b"}).
static const size_t SPP_MAX_JSON = 1024;  // tope anti-objeto-sin-cerrar

static String rxBuffer;
static int    braceDepth = 0;
static bool   inJson     = false;
static bool   inString   = false;
static bool   escaped    = false;

void spp_init(void) {
    if (!SerialBT.begin(SPP_SERVER_NAME)) {
        Serial.printf("[%s] Error iniciando Bluetooth SPP\n", SPP_TAG);
        return;
    }
    Serial.printf("[%s] Bluetooth SPP iniciado: %s\n", SPP_TAG, SPP_SERVER_NAME);
}

void spp_loop(void) {
    while (SerialBT.available()) {
        char c = (char)SerialBT.read();

        // Actualiza el contexto string/escape antes de contar llaves.
        if (escaped) {
            escaped = false;
        } else if (inString) {
            if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
            }
        } else {  // fuera de un string
            if (c == '"') {
                inString = true;
            } else if (c == '{') {
                if (braceDepth == 0) {
                    rxBuffer = "";
                    inJson = true;
                }
                braceDepth++;
            }
        }

        if (inJson) {
            rxBuffer += c;  // incluye la '}' de cierre antes de despachar
            if (rxBuffer.length() > SPP_MAX_JSON) {  // protección: reinicia framing
                rxBuffer = "";
                inJson = false;
                braceDepth = 0;
                inString = false;
                escaped = false;
                continue;
            }
        }

        // Solo cierra con una '}' que esté fuera de un string.
        if (c == '}' && inJson && !inString) {
            braceDepth--;
            if (braceDepth <= 0) {
                inJson = false;
                braceDepth = 0;
                handle_received_json(rxBuffer.c_str());
                rxBuffer = "";
            }
        }
    }
}
