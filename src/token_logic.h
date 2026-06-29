#ifndef TOKEN_LOGIC_H
#define TOKEN_LOGIC_H

// Inicializa el GPIO de la cerradura en un estado seguro (cerrada) al arrancar.
void token_logic_init(void);

// Recibe un JSON {"user":..,"token":..}, lo valida contra la API (valtokenesp) y,
// si es válido, abre la cerradura (no bloqueante). Responde el veredicto por BLE
// NOTIFY: {"valido":1} (abierta) o {"valido":0,...} (rechazada/error).
void handle_received_json(const char *data);

// Cierra la cerradura cuando expira su tiempo de apertura. Llamar desde loop()
// (no bloqueante: no usa vTaskDelay, así el loop sigue atendiendo BLE/WiFi).
void lock_tick(void);

#endif // TOKEN_LOGIC_H
