#ifndef HTTP_TOKEN_VERIFY_H
#define HTTP_TOKEN_VERIFY_H

// Consulta la API y devuelve true si el token del usuario es válido.
bool verify_token(const char *user, const char *token);

#endif // HTTP_TOKEN_VERIFY_H
