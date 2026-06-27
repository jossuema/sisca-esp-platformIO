#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H

// Conecta a la red WiFi configurada en config.h.
// Bloqueante hasta obtener conexión (igual que el proyecto ESP-IDF original).
void wifi_connect(void);

#endif // WIFI_CONNECT_H
