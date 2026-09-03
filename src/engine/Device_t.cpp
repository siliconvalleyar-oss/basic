/**
 * @file Device_t.cpp
 * @brief Implementación de la clase Device::Device_t para la pantalla OLED.
 *
 * @details Esta implementación inicializa la pantalla OLED SSD1306 vía I2C
 * (librería bcm2835) y dibuja una demostración básica: título, rectángulos,
 * un círculo y texto. La memoria se gestiona de forma segura con unique_ptr.
 *
 * Nota importante: la librería bcm2835 exige llamar a bcm2835_init() antes de
 * acceder a cualquier periférico (GPIO/I2C). En caso contrario los punteros de
 * registro quedan sin inicializar (0xffffffff) y cualquier acceso produce un
 * Segmentation fault. Por eso run() llama a bcm2835_init() antes de usar el
 * display.
 *
 * @author Proyecto BASIC (Raspberry Pi)
 * @version 1.0.0
 */

#include "Device_t.hpp"

#include <bcm2835.h>
#include <cstdio>

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
    // Solo apaga/libera si el hardware se inicializó correctamente. Esto evita
    // llamar a bcm2835_* sin bcm2835_init() (p.ej. al usar --version), lo que
    // produciría un Segmentation fault.
    if (hwReady_) {
        if (oled_) {
            oled_->OLEDPowerDown();
            oled_.reset();
        }
        // Libera los recursos de bcm2835 (mapeos de memoria).
        bcm2835_close();
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

    // Muestra la versión al iniciar la aplicación.
    printVersion();

    // Inicializa la librería bcm2835 (mapea los registros del hardware).
    // Es OBLIGATORIO llamarlo antes de usar I2C/GPIO. Sin esto se produce un
    // Segmentation fault al acceder a los punteros de registro (0xffffffff).
    if (!bcm2835_init()) {
        std::printf("Error: No se pudo inicializar bcm2835 (¿ejecutar como root?)\n");
        return 1;
    }
    hwReady_ = true;

    // Ejecuta la demostración básica de la pantalla OLED.
    drawBasicDemo();

    std::printf("Pantalla OLED inicializada correctamente.\n");

    return 0;
}

} // namespace Device
