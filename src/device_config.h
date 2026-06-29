#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <Arduino.h>

// Configuración por dispositivo persistida en NVS. Permite que el MISMO binario
// sirva para cualquier aula: el ecodigo se aprovisiona una vez por consola serie
// (comando "ecodigo N") y queda guardado en flash.

// Carga la configuración desde NVS (usa DEVICE_ECODIGO como valor por defecto).
void device_config_begin(void);

// ecodigo actual (runtime). El nombre BLE y la verificación de token lo usan.
int device_get_ecodigo(void);

// Nombre BLE derivado: BLE_NAME_PREFIX + ecodigo.
String device_ble_name(void);

// Atiende comandos de aprovisionamiento por Serial. Llamar desde loop().
// Comandos:  "ecodigo N"  -> guarda el ecodigo y reinicia
//            "ecodigo"    -> muestra el ecodigo actual
//            "help"       -> ayuda
void device_config_serial_tick(void);

#endif // DEVICE_CONFIG_H
