# sisca-esp (PlatformIO / Arduino)

Migración del controlador de acceso **sisca-esp** desde ESP-IDF al framework
**Arduino** sobre **PlatformIO**. Mantiene la misma funcionalidad y arquitectura
modular del proyecto original.

## Funcionamiento

1. **WiFi** (`wifi_connect`) — conecta en modo STA, bloqueante hasta obtener IP.
2. **Bluetooth SPP** (`spp_server`) — servidor Bluetooth Classic. Recibe JSON
   desde el celular y lo despacha a la lógica de token.
3. **Lógica de token** (`token_logic`) — parsea `{"user":..., "token":...}`,
   lo verifica contra la API y, si es válido, activa la cerradura (GPIO18) 2 s.
4. **Verificación HTTP** (`http_token_verify`) — `GET` a la API de la UTMachala
   y lee `data[0].valido`.

## Hardware

| Señal      | GPIO |
|------------|------|
| Cerradura  | 18   |

> Requiere un **ESP32 clásico** (board `esp32dev`). Las variantes S2/S3/C3 no
> tienen Bluetooth Classic y `BluetoothSerial` no compilaría.

## Configuración

Edita `include/config.h` (credenciales WiFi, URL y clave de la API, nombre SPP).

## Uso

```bash
# Compilar
pio run

# Compilar y flashear
pio run -t upload

# Monitor serie
pio device monitor
```

## Dependencias (gestionadas por PlatformIO)

- `bblanchon/ArduinoJson` (v7)
- `WiFi`, `HTTPClient`, `WiFiClientSecure`, `BluetoothSerial` (incluidas en el
  core Arduino-ESP32).

## Notas de la migración (ESP-IDF → Arduino)

| ESP-IDF                          | Arduino                                  |
|----------------------------------|------------------------------------------|
| `esp_wifi` + event groups + NVS  | `WiFi.h` (`WiFi.begin` / `WL_CONNECTED`) |
| `esp_http_client`                | `HTTPClient` + `WiFiClientSecure`        |
| `esp_spp_api` (Bluedroid)        | `BluetoothSerial`                        |
| `cJSON`                          | `ArduinoJson` v7                         |
| `gpio_config` / `gpio_set_level` | `pinMode` / `digitalWrite`               |
| `app_main`                       | `setup()` + `loop()`                     |

Detalles relevantes:

- **TLS**: el original no validaba el certificado del servidor; aquí se replica
  con `client.setInsecure()`. Para producción, fija el certificado raíz con
  `client.setCACert(...)`.
- **Reensamblado SPP**: a diferencia del callback de Bluedroid (que entregaba el
  buffer completo), `BluetoothSerial` es un stream; el JSON se reconstruye
  contando llaves `{ }` (rastreando contexto de strings/escapes) hasta cerrar
  el objeto.
- **URL-encoding**: `user` y `token` se codifican (percent-encoding) antes de
  insertarlos en la query, para evitar inyección/truncación de parámetros.
