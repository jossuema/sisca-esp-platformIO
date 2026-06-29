#include <Arduino.h>
#include <NimBLEDevice.h>
#include "ble_server.h"
#include "token_logic.h"
#include "device_config.h"
#include "config.h"

// Característica WRITE+NOTIFY: guardamos el puntero para poder enviar el ACK.
static NimBLECharacteristic *gChar = nullptr;
static volatile bool gConnected = false;

// ----------------------------------------------------------------------------
//  Reensamblado del JSON (idéntico al usado antes con SPP): el flujo puede
//  llegar fragmentado entre varias escrituras BLE, así que acumulamos por
//  conteo de llaves hasta tener un objeto JSON completo, rastreando el contexto
//  string/escape para no contar llaves dentro de un valor (p. ej. {"u":"a}b"}).
// ----------------------------------------------------------------------------
static const size_t BLE_MAX_JSON = 1024;  // tope anti-objeto-sin-cerrar

static String rxBuffer;
static int    braceDepth = 0;
static bool   inJson     = false;
static bool   inString   = false;
static bool   escaped    = false;

static void reset_framing(void) {
    rxBuffer   = "";
    braceDepth = 0;
    inJson     = false;
    inString   = false;
    escaped    = false;
}

// Procesa un byte recibido y, al completar un objeto JSON, lo despacha a la cola.
static void feed_byte(char c) {
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
        if (rxBuffer.length() > BLE_MAX_JSON) {  // protección: reinicia framing
            reset_framing();
            return;
        }
    }

    // Solo cierra con una '}' que esté fuera de un string.
    if (c == '}' && inJson && !inString) {
        braceDepth--;
        if (braceDepth <= 0) {
            inJson = false;
            braceDepth = 0;
            // Despacha a la cola del worker. handle_received_json solo hace
            // strdup + xQueueSend (no bloquea), seguro desde el hilo del host BLE.
            handle_received_json(rxBuffer.c_str());
            rxBuffer = "";
        }
    }
}

// ----------------------------------------------------------------------------
//  Callbacks NimBLE
// ----------------------------------------------------------------------------
class WriteCallback : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *pChar) override {
        std::string value = pChar->getValue();
        for (size_t i = 0; i < value.length(); ++i) {
            feed_byte(value[i]);
        }
    }
};

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer *pServer) override {
        gConnected = true;
        Serial.printf("[%s] Central conectado\n", BLE_TAG);
        // Reinicia el framing por si la conexión anterior quedó a medio mensaje.
        reset_framing();
    }
    void onDisconnect(NimBLEServer *pServer) override {
        gConnected = false;
        Serial.printf("[%s] Central desconectado, re-anunciando\n", BLE_TAG);
        reset_framing();
        NimBLEDevice::startAdvertising();
    }
};

void ble_init(void) {
    // Nombre derivado del ecodigo en runtime (NVS) para coincidir con el móvil.
    String deviceName = device_ble_name();

    NimBLEDevice::init(deviceName.c_str());
    NimBLEDevice::setMTU(BLE_MTU);

#if BLE_REQUIRE_ENCRYPTION
    // Just Works (bond, sin MITM, secure connections): cifra el enlace sin pedir
    // passkey. El móvil mostrará un diálogo de emparejamiento la primera vez.
    NimBLEDevice::setSecurityAuth(true, false, true);
#endif

    NimBLEServer *pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService *pService = pServer->createService(BLE_SERVICE_UUID);

    // WRITE (recibe el token) + NOTIFY (responde el veredicto / ACK end-to-end).
    uint32_t props = NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR |
                     NIMBLE_PROPERTY::NOTIFY;
#if BLE_REQUIRE_ENCRYPTION
    props |= NIMBLE_PROPERTY::WRITE_ENC;
#endif

    gChar = pService->createCharacteristic(BLE_CHARACTERISTIC_UUID, props);
    gChar->setCallbacks(new WriteCallback());

    pService->start();

    NimBLEAdvertising *pAdv = NimBLEDevice::getAdvertising();
    pAdv->addServiceUUID(BLE_SERVICE_UUID);  // permite filtrar por Service UUID en el móvil
    pAdv->setScanResponse(true);
    NimBLEDevice::startAdvertising();

    Serial.printf("[%s] BLE GATT iniciado: %s (servicio %s / char %s)\n",
                  BLE_TAG, deviceName.c_str(), BLE_SERVICE_UUID, BLE_CHARACTERISTIC_UUID);
}

void ble_notify(const char *json) {
    if (gChar == nullptr || !gConnected) return;
    gChar->setValue((const uint8_t *)json, strlen(json));
    gChar->notify();
    Serial.printf("[%s] ACK -> %s\n", BLE_TAG, json);
}
