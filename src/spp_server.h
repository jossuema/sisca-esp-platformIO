#ifndef SPP_SERVER_H
#define SPP_SERVER_H

// Inicializa el servidor Bluetooth Classic SPP.
void spp_init(void);

// Procesa los bytes entrantes por SPP. Debe llamarse periódicamente
// desde loop(). Reensambla el JSON recibido y lo despacha a token_logic.
void spp_loop(void);

#endif // SPP_SERVER_H
