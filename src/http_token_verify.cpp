#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "http_token_verify.h"
#include "config.h"

static const char *TAG = "HTTP_TOKEN";

// Percent-encode (RFC 3986): codifica todo lo que no sea un carácter
// "unreserved". user/token provienen de JSON no confiable (Bluetooth SPP); sin
// codificar, caracteres como & = # o espacios permitirían inyectar o truncar
// parámetros de la query (y esto controla una cerradura física).
static String url_encode(const char *s) {
    static const char *hex = "0123456789ABCDEF";
    String out;
    for (const uint8_t *p = (const uint8_t *)s; *p; ++p) {
        uint8_t c = *p;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            out += (char)c;
        } else {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 0x0F];
        }
    }
    return out;
}

bool verify_token(const char *user, const char *token) {
    char url[512];
    String eUser  = url_encode(user);
    String eToken = url_encode(token);

    int n = snprintf(url, sizeof(url),
             "%s?funcion=%s&user=%s&ecodigo=%d&token=%s&pkey=%s",
             API_URL, API_FUNCTION, eUser.c_str(), CLASSROOM_ECODIGO, eToken.c_str(), PKEY);
    if (n < 0 || n >= (int)sizeof(url)) {
        Serial.printf("[%s] URL demasiado larga, abortando\n", TAG);
        return false;
    }

    WiFiClientSecure client;
    // El proyecto ESP-IDF original no validaba el certificado del servidor;
    // mantenemos ese comportamiento con setInsecure(). Para producción, lo
    // recomendable es fijar el certificado raíz con client.setCACert(...).
    client.setInsecure();

    HTTPClient http;
    http.setTimeout(API_TIMEOUT);

    if (!http.begin(client, url)) {
        Serial.printf("[%s] No se pudo iniciar la petición HTTP\n", TAG);
        return false;
    }

    int status = http.GET();
    if (status != 200) {
        Serial.printf("[%s] HTTP status code: %d\n", TAG, status);
        http.end();
        return false;
    }

    String payload = http.getString();
    Serial.printf("[%s] Respuesta: %s\n", TAG, payload.c_str());
    http.end();

    // Parsear JSON: { "data": [ { "valido": 1 } ] }
    bool valido = false;
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[%s] Error parseando JSON: %s\n", TAG, err.c_str());
        return false;
    }

    JsonArray data = doc["data"].as<JsonArray>();
    if (!data.isNull() && data.size() > 0) {
        int v = data[0]["valido"] | 0;  // 0 si falta o no es numérico
        valido = (v == 1);
    }

    return valido;
}
