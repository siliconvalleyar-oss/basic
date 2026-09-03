/**
 * @file Device_t.hpp
 * @brief Clase principal del dispositivo que controla la pantalla OLED SSD1306.
 *
 * @details Define la clase Device::Device_t que encapsula la inicialización y el
 * uso básico de la pantalla OLED (acceso I2C por /dev/i2c-N). Permite mostrar
 * información por consola y en el display, así como manejar el argumento --version.
 *
 * @author Proyecto BASIC (Raspberry Pi)
 * @version 1.1.0
 */

#ifndef DEVICE_T_HPP
#define DEVICE_T_HPP

// La biblioteca OLED define una macro global "swap(a,b)" (en
// SSD1306_OLED_graphics.hpp) que interfiere con std::swap de la STL. Para
// evitarlo incluimos primero las cabeceras de la STL y, tras los includes de
// la librería OLED, anulamos dicha macro con #undef swap.
#include <memory>
#include <cstdint>
#include <string>

// Incluye la cabecera de la pantalla OLED (SSD1306), que usa la capa I2C Linux.
#include "SSD1306_OLED.hpp"

// Anula la macro swap definida por la librería OLED para no romper std::swap.
#ifdef swap
#undef swap
#endif

/**
 * @namespace Device
 * @brief Espacio de nombres que agrupa los componentes de alto nivel del dispositivo.
 */
namespace Device {

/**
 * @class Device_t
 * @brief Encapsula el ciclo de vida y el funcionamiento de la pantalla OLED.
 *
 * @details La clase crea internamente un objeto SSD1306 con memoria gestionada
 * automáticamente (unique_ptr). El método run() inicializa el display y dibuja
 * una serie de elementos básicos (texto, rectángulos, círculos) como demostración.
 */
class Device_t {
public:
    /** @brief Ancho del display en píxeles. */
    static constexpr int16_t OLED_WIDTH = 128;
    /** @brief Alto del display en píxeles. */
    static constexpr int16_t OLED_HEIGHT = 64;

    /**
     * @brief Constructor de la clase.
     * @details Crea la instancia de la pantalla OLED con las dimensiones del display.
     */
    Device_t();

    /**
     * @brief Destructor de la clase.
     * @details Apaga la pantalla OLED antes de liberar la memoria.
     */
    ~Device_t();

    // Elimina copia y asignación para evitar dobles liberaciones del display.
    Device_t(const Device_t&) = delete;
    Device_t& operator=(const Device_t&) = delete;

    /**
     * @brief Ejecuta la lógica principal del dispositivo.
     * @param showVersion Si es true, solo muestra la versión y termina.
     * @return Código de salida (0 = éxito).
     */
    int run(bool showVersion = false);

    /**
     * @brief Ejecuta la demostración principal del display.
     * @return Código de salida (0 = éxito).
     */
    int runDemo();

    /**
     * @brief Enciende todos los píxeles de la pantalla (vacía en blanco).
     * @details Útil para validar visualmente que el display muestra contenido
     * (patrón inconfundible de pantalla completa). Mantiene la imagen visible
     * unos segundos antes de salir.
     * @return Código de salida (0 = éxito).
     */
    int fillWhite();

    /**
     * @brief Devuelve la versión de la aplicación compilada.
     * @return Cadena con el número de versión.
     */
    std::string version() const;

private:
    /**
     * @brief Muestra la versión por consola.
     */
    void printVersion() const;

    /**
     * @brief Dibuja una demostración básica en la pantalla OLED.
     * @details Escribe el título y dibuja formas geométricas básicas.
     */
    void drawBasicDemo();

    /** @brief Instancia de la pantalla OLED (memoria gestionada automáticamente). */
    std::unique_ptr<SSD1306> oled_;

    /** @brief Indica si el display se inicializó (para apagado seguro en destructor). */
    bool hwReady_ = false;
};

} // namespace Device

#endif // DEVICE_T_HPP
