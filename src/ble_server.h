#ifndef BLE_SERVER_H
#define BLE_SERVER_H

// Servidor Bluetooth Low Energy (GATT) que sustituye al antiguo SPP Classic.
//
// Expone un servicio (BLE_SERVICE_UUID) con una característica (BLE_CHARACTERISTIC_UUID)
// que es WRITE + NOTIFY: el móvil escribe el JSON {"user":..,"token":..} y el ESP
// responde por NOTIFY con el veredicto {"valido":0|1,...} (ACK de extremo a extremo).
// El móvil (BluetoothUtilsBLE) ya es un central GATT que espera este perfil (FFE0/FFE1).
//
// El JSON puede llegar fragmentado en varias escrituras (MTU BLE): se reensambla en
// el callback de escritura y, una vez completo, se encola (handle_received_json ->
// cola del worker) sin bloquear el host Bluetooth.

// Inicializa NimBLE, el servidor GATT y el advertising (nombre = ecodigo en runtime).
void ble_init(void);

// Envía una respuesta (NOTIFY) al central conectado. No-op si no hay suscriptor.
void ble_notify(const char *json);

#endif // BLE_SERVER_H
