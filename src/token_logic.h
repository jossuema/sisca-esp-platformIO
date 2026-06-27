#ifndef TOKEN_LOGIC_H
#define TOKEN_LOGIC_H

// Recibe un JSON {"user":..., "token":...}, lo valida contra la API y,
// si es válido, activa la cerradura (GPIO) durante 2 segundos.
void handle_received_json(const char *data);

#endif // TOKEN_LOGIC_H
