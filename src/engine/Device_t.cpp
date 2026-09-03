/**
 * @file Device_t.cpp
 * @brief Implementación de la clase Device::Device_t para la pantalla OLED.
 *
 * @details Esta implementación inicializa la pantalla OLED SSD1306 vía I2C
 * (acceso por /dev/i2c-N mediante ioctl, compatible con todas las Raspberry Pi
 * incluidas Pi 5/CM5 con chip RP1) y dibuja una demostración básica: título,
 * rectángulos, un círculo y texto. La memoria se gestiona de forma segura con
 * unique_ptr.
 *
 * Nota: el acceso I2C ya no depende de la librería bcm2835 (que no soporta
 * Raspberry Pi 5 / Compute Module 5). El cierre seguro del adaptador I2C lo
 * realiza el destructor de la clase SSD1306.
 *
 * @author Proyecto BASIC (Raspberry Pi)
 * @version 1.1.0
 */

#include "Device_t.hpp"

#include <cstdio>
#include <cstring>
#include <unistd.h>

// Versión por defecto en caso de que el Makefile no la defina.
#ifndef VERSION
#define VERSION "dev"
#endif

namespace Device {

Device_t::Device_t()
    : oled_(std::make_unique<SSD1306>(OLED_WIDTH, OLED_HEIGHT)) {
    // La memoria del OLED se gestiona automáticamente.
}

Device_t::~Device_t() {
    // Solo apagamos/liberamos el display si se inicializó (evita acceder al I2C
    // cuando solo se mostró --version). El destructor de SSD1306 cierra el
    // adaptador I2C si quedó abierto y libera el frame buffer.
    if (hwReady_ && oled_) {
        oled_->OLEDPowerDown();
        oled_.reset();
    }
}

std::string Device_t::version() const {
    return VERSION;
}

void Device_t::printVersion() const {
    std::printf("App v%s\n", VERSION);
    std::printf("Compilado para: %s\n",
#ifdef __aarch64__
                "ARM64 (aarch64)"
#elif defined(__arm__)
                "ARM (32 bits)"
#else
                "x86-64"
#endif
    );
}

void Device_t::drawBasicDemo() {
    // Inicializa la pantalla OLED por I2C (velocidad y dirección por defecto).
    oled_->OLEDbegin();

    // Limpia el buffer interno de la pantalla.
    oled_->OLEDclearBuffer();

    // Configura color de texto (blanco) y posición inicial del cursor.
    oled_->setTextColor(WHITE);
    oled_->setTextSize(1);

    // Escribe el título del proyecto en la primera línea.
    oled_->setCursor(0, 0);
    oled_->print("BASIC RPi");

    // Escribe la versión de la aplicación en la segunda línea.
    oled_->setCursor(0, 10);
    oled_->print("v");
    oled_->print(VERSION);

    // Dibuja un rectángulo (contorno) en la parte inferior.
    oled_->drawRect(0, 24, OLED_WIDTH, 16, WHITE);

    // Dibuja un rectángulo relleno pequeño como indicador.
    oled_->fillRect(4, 28, 16, 8, WHITE);

    // Dibuja un círculo de demostración en el centro.
    oled_->drawCircle(OLED_WIDTH / 2, OLED_HEIGHT - 12, 6, WHITE);

    // Vuelca el buffer al display físico.
    oled_->OLEDupdate();
}

int Device_t::run(bool showVersion) {
    // Si solo se pide la versión, la muestra y termina (sin tocar el hardware).
    if (showVersion) {
        printVersion();
        return 0;
    }

    // Muestra la versión al iniciar la aplicación y ejecuta la demo.
    printVersion();
    return runDemo();
}

int Device_t::runDemo() {
    // Ejecuta la demostración básica de la pantalla OLED. El acceso I2C se
    // realiza por /dev/i2c-N (ioctl) y requiere permisos de root en /dev/i2c-*.
    drawBasicDemo();
    hwReady_ = true;

    std::printf("Pantalla OLED inicializada correctamente.\n");

    return 0;
}

int Device_t::fillWhite() {
    // Muestra la versión al iniciar.
    printVersion();

    // Inicializa el display (configuración y encendido).
    oled_->OLEDbegin();

    // Enciende todos los píxeles: patrón inconfundible de pantalla completa
    // (blanco) para validar visualmente que el display muestra contenido.
    std::memset(oled_->buffer, 0xFF, OLED_WIDTH * (OLED_HEIGHT / 8));
    oled_->OLEDupdate();
    hwReady_ = true;

    std::printf("Pantalla llenada en blanco (validación visual).\n");

    // Mantiene la imagen visible unos segundos antes de salir.
    usleep(5000 * 1000);
    return 0;
}

} // namespace Device
