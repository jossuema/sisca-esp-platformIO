#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H

// Inicia WiFi en modo estación SIN bloquear: arranca la conexión, habilita la
// reconexión automática y configura NTP. El dispositivo sigue arrancando (BLE
// disponible de inmediato) aunque el WiFi tarde o no esté disponible.
void wifi_init(void);

// Mantenimiento periódico de la conexión (reintentos, logs). Llamar desde loop().
void wifi_loop(void);

// true si hay conexión WiFi Y la hora está sincronizada (necesario para validar
// el certificado TLS en la verificación del token).
bool wifi_ready(void);

#endif // WIFI_CONNECT_H
