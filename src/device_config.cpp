#include <Preferences.h>
#include "device_config.h"
#include "config.h"

static const char *TAG = "DEV_CFG";

static Preferences prefs;
static int ecodigo = DEVICE_ECODIGO;

void device_config_begin(void) {
    prefs.begin(NVS_NAMESPACE, /*readOnly=*/false);
    ecodigo = prefs.getInt(NVS_KEY_ECODIGO, DEVICE_ECODIGO);
    prefs.end();
    Serial.printf("[%s] ecodigo = %d (BLE: %s)\n",
                  TAG, ecodigo, device_ble_name().c_str());
    Serial.printf("[%s] Para cambiar el aula escribe por Serial: ecodigo <N>\n", TAG);
}

int device_get_ecodigo(void) {
    return ecodigo;
}

String device_ble_name(void) {
    return String(BLE_NAME_PREFIX) + String(ecodigo);
}

static void set_ecodigo(int value) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putInt(NVS_KEY_ECODIGO, value);
    prefs.end();
    Serial.printf("[%s] ecodigo guardado = %d. Reiniciando para re-anunciar BLE...\n",
                  TAG, value);
    delay(300);
    ESP.restart();
}

void device_config_serial_tick(void) {
    if (!Serial.available()) return;

    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;

    if (line == "help") {
        Serial.println("Comandos: 'ecodigo <N>' (guardar y reiniciar) | 'ecodigo' (ver) | 'help'");
    } else if (line == "ecodigo") {
        Serial.printf("[%s] ecodigo actual = %d (%s)\n", TAG, ecodigo, device_ble_name().c_str());
    } else if (line.startsWith("ecodigo ")) {
        int value = line.substring(8).toInt();
        if (value > 0) {
            set_ecodigo(value);  // no retorna (reinicia)
        } else {
            Serial.printf("[%s] ecodigo inválido: '%s'\n", TAG, line.c_str());
        }
    } else {
        Serial.printf("[%s] Comando desconocido: '%s' (escribe 'help')\n", TAG, line.c_str());
    }
}
