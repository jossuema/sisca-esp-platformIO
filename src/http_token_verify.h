#ifndef HTTP_TOKEN_VERIFY_H
#define HTTP_TOKEN_VERIFY_H

// Consulta la API (funcion=valtokenesp) y devuelve true si el token del usuario
// es válido para el espacio indicado. Devuelve false si no hay red/hora aún.
bool verify_token(const char *user, const char *token, int ecodigo);

#endif // HTTP_TOKEN_VERIFY_H
