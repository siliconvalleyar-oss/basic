/**
 * @file main.cpp
 * @brief Punto de entrada principal de la aplicación BASIC para Raspberry Pi.
 *
 * @details Punto de entrada que integra el funcionamiento de la pantalla OLED
 * SSD1306. Crea una instancia de Device::Device_t mediante un unique_ptr y
 * ejecuta su método run(), que inicializa el display (acceso I2C por /dev/i2c-N)
 * y dibuja la demostración básica en la pantalla.
 *
 * Si se pasa el argumento --version, solo se muestra la versión sin tocar el
 * hardware. La memoria se libera automáticamente al salir del main.
 *
 * Ejecución recomendada (necesita permisos de root para acceder a /dev/i2c-*):
 *     sudo ./bin/App
 *     sudo ./bin/App --version
 *
 * @author Proyecto BASIC (Raspberry Pi)
 * @version 1.1.0
 */

#include <memory>
#include <cstring>

#include "Device_t.hpp"

/**
 * @brief Punto de entrada de la aplicación.
 * @param argc Número de argumentos de la línea de comandos.
 * @param argv Vector de argumentos de la línea de comandos.
 * @return Código de retorno del proceso (0 = éxito).
 */
int main(int argc, char* argv[]) {
    // Soporte del argumento --version: muestra la versión y termina.
    bool showVersion = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--version") == 0 || std::strcmp(argv[i], "-v") == 0) {
            showVersion = true;
            break;
        }
    }

    // Crea el dispositivo (incluye la pantalla OLED) con memoria gestionada.
    // run() se encarga de: OLEDbegin() -> dibujar -> cerrar (acceso I2C vía /dev/i2c).
    auto device = std::make_unique<Device::Device_t>();
    return device->run(showVersion);
}
