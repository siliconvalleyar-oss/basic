/**
 * @file main.cpp
 * @brief Punto de entrada principal de la aplicación BASIC para Raspberry Pi.
 *
 * @details Punto de entrada que integra el funcionamiento de la pantalla OLED
 * SSD1306. Crea una instancia de Device::Device_t mediante un unique_ptr y
 * ejecuta su método run(), que inicializa la librería bcm2835 (obligatoria para
 * acceder a I2C/GPIO) y dibuja la demostración básica en el display.
 *
 * Si se pasa el argumento --version, solo se muestra la versión sin tocar el
 * hardware. La memoria se libera automáticamente al salir del main.
 *
 * Ejecución recomendada (necesita permisos de root para acceder al hardware):
 *     sudo ./bin/App
 *     sudo ./bin/App --version
 *
 * @author Proyecto BASIC (Raspberry Pi)
 * @version 1.0.0
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
    // run() se encarga de: bcm2835_init() -> OLEDbegin() -> dibujar -> cerrar.
    auto device = std::make_unique<Device::Device_t>();
    return device->run(showVersion);
}
