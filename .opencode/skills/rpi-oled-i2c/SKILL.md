---
name: rpi-oled-i2c
description: Uso del display OLED SSD1306 por I2C en Raspberry Pi (proyecto BASIC). Aplica cuando se trabaja con los headers o fuentes de src/oled, la capa SSD1306_I2C, el acceso I2C por /dev/i2c-N, compilación cruzada/remota para cm5 (ARM64) o rpi2w (ARM32), configuración I2C/HDMI de /boot/config.txt, o el esquema de versionado con tags git del proyecto. Usa ONLY cuando aparezcan conceptos como bcm2835, /dev/i2c, SSD1306, GPIO2/GPIO3, OLED, crossover, tag, VERSION, CM5 o rpi2w.
---

# Raspberry Pi OLED SSD1306 por I2C (Proyecto BASIC)

Conocimiento aprendido en la sesión sobre cómo manejar la pantalla OLED SSD1306
en el proyecto BASIC para Raspberry Pi. Este skill resume los errores que ya se
resolvieron para **no repetirlos**.

## Resumen crítico (aprendido con errores)

1. **`bcm2835` NO funciona en Raspberry Pi 5 / Compute Module 5 (chip RP1).**
   La librería `bcm2835` (v1.75) accede a los registros I2C del SoC BCM2835/2711,
   que en la Pi 5/CM5 están en el chip RP1 con un mapa de registros distinto.
   El síntoma es `bcm2835I2CReasonCodes :: Error code 1` (NACK) en cada escritura,
   **aunque `i2cset`/`i2cget`/`i2cdetect` funcionen bien** (el display responde
   por el driver estándar de Linux). NO es un bug del display ni del código:
   es incompatibilidad de la librería con el hardware.

2. **La solución es acceder al I2C por `/dev/i2c-N` con ioctl de Linux**
   (igual que hacen `i2cset`/`i2cget`), que funciona en TODAS las Pi.
   El proyecto creó la capa `SSD1306_I2C` para esto y eliminó la dependencia
   de `bcm2835` del OLED (y del `Makefile`: ya no enlaza `-lbcm2835`).

## Arquitectura / flujo de la app

- `src/main.cpp` crea `std::make_unique<Device::Device_t>()` y llama `run()`.
  Soportes: `--version`/`-v` (solo versión, sin tocar HW) y `--fill`
  (pantalla completa blanca, validación visual).
- `Device::Device_t` (`include/core/Device_t.hpp`, `src/engine/Device_t.cpp`)
  envuelve el OLED con un `unique_ptr<SSD1306>`. `run()` → `runDemo()` →
  `drawBasicDemo()`; `fillWhite()` llena el buffer con `0xFF` y mantiene 5 s.
- El destructor apaga el display SOLO si `hwReady_` es verdadero (evita tocar
  el I2C cuando solo se usó `--version`).

## Capa I2C Linux (`include/oled/SSD1306_I2C.hpp` / `src/oled/SSD1306_I2C.cpp`)

```cpp
namespace SSD1306LinuxI2C {
    const char* I2C_DEV_PATH;             // "/dev/i2c-1"
    int  i2c_open(const char* dev, uint8_t addr);         // fd>=0 o -1
    bool i2c_set_address(int fd, uint8_t addr);
    bool i2c_write_byte(int fd, uint8_t value, uint8_t cmd); // true = ACK
    void i2c_close(int fd);
}
```

- `cmd` es el byte de control: `0x00` = comando, `0x40` = dato.
- `i2c_write_byte` devuelve `true` (ACK) solo si el `write(fd,{cmd,value},2)`
  completa los 2 bytes → el display confirmó.
- En `SSD1306_OLED.cpp`, `OLED_I2C_ON()` abre el fd, `OLED_I2C_OFF()` lo cierra;
  `I2C_Write_Byte` reintenta 3 veces ante fallo (no se cuelga).

## Uso del OLED SSD1306 (API real del proyecto)

Constructor: `SSD1306 oled(128, 64);` → `oled.OLEDbegin();` (abre `/dev/i2c-1`,
dirección `0x3C`), `OLEDclearBuffer()`, dibujar, `OLEDupdate()`,
`OLEDPowerDown()`. El buffer es público: `uint8_t* buffer` (128*8=1024 B).

Funciones clave (nombres REALES de esta versión):

- Texto: `setCursor(x,y)`, `print()`, `println()`, `setTextSize(s)`,
  `setTextColor(c)` / `setTextColor(c,bg)`, `setTextWrap(b)`,
  `setFontNum(OLEDFontType_e)`.
- `drawChar(x,y,c,color,bg,size)`, `drawText(x,y,pText,color,bg,size)`,
  `drawCharNumFont(...)`, `drawTextNumFont(...)`.
- Gráficos: `drawPixel`, `drawLine`, `drawFastVLine`, `drawFastHLine`,
  `drawRect`, `fillRect`, `fillScreen`, `drawCircle`, `fillCircle`,
  `drawTriangle`, `fillTriangle`, `drawRoundRect`, `fillRoundRect`.
- Buffer: `OLEDclearBuffer`, `OLEDupdate`, `OLEDBuffer(x,y,w,h,data)`,
  `OLEDFillScreen(pixel,delay)`, `OLEDFillPage(page,pixels,delay)`.
- Control: `OLEDEnable(on)`, `OLEDContrast(0x00-0xFF)`, `OLEDInvert(bool)`,
  `OLEDPowerDown()`, `setRotation(0-3)`/`getRotation()`, `OLEDReset()`, `OLEDinit()`.
- Scroll (con prefijo OLED_): `OLED_StartScrollRight/Left/DiagRight/DiagLeft`,
  `OLED_StopScroll()`.
- Bitmap: `OLEDBitmap(x,y,w,h,data,invert)`.
- Colores: `BLACK=0`, `WHITE=1`, `INVERSE=2`.

### Fuentes (8, en esta versión)

`OLEDFontType_Default(5x8), Thick(7x8), SevenSeg(4x8), Wide(8x8), Tiny(3x8),
Homespun(7x8), Bignum(16x32), Mednum(16x16)`.

- Default/Thick/SevenSeg/Wide/Tiny/Homespun son escalables (con `setTextSize`).
- Bignum/Mednum son numéricas fijas (no escalables), solo dígitos y `- . / :`.

### Códigos de retorno (`OLED_Return_Codes_e`)

`OLED_Success=0, WrongFont=2, CharScreenBounds=3, CharFontASCIIRange=4,
CharArrayNullptr=5, BitmapNullptr=7, BitmapScreenBounds=8,
BitmapLargerThanScreen=9, BitmapVerticalSize=10, BitmapHorizontalSize=11,
BitmapSize=12, CustomCharLen=13`.

## Compilación

- Local (PC): `make`, `make crossover` (auto-detecta), `make crossover64`
  (aarch64), `make crossover32` (arm-linux-gnueabihf).
- Remoto (nativo en cada Pi): `make remote` (usa `scripts/build_remote.sh`),
  o `make deploy` (`scripts/deploy.sh`, scp del binario).
- `config/deploy.cfg` define `REMOTE_USER/HOST/DIR/BRANCH`. Actualmente apunta a
  `rpi2w.local` en la rama `crossover`.
- `scripts/fetch_sysroot.sh` trae `libbcm2835.a`/`bcm2835.h` de la Pi para
  compilación cruzada local. **OBSOLETO**: ya no se usa porque el OLED no
  depende de bcm2835; se puede dejar para otras librerías o eliminar.
- Acceso SSH sin exponer contraseña: `sshpass -e ssh pi@<host>` con la variable
  `SSHPASS` seteada (nunca guardar la contraseña en el repo).

### Máquinas destino

- `cm5.local` = Raspberry Pi Compute Module 5, **aarch64 (64-bit)**, chip RP1.
  Display requiere la capa `/dev/i2c-N` (bcm2835 NO funciona aquí en I2C).
- `rpi2w.local` = en realidad Pi **Zero 2 W** (revisión `902120`), **ARM (32-bit)**,
  SoC BCM2835 (bcm2835 podría funcionar, igual se usa `/dev/i2c-N` por uniformidad).

## Configuración de la Pi (`/boot/config.txt`)

- Habilitar I2C1 (pines GPIO2=SDA, GPIO3=SCL): `dtparam=i2c_arm=on` y opcional
  `dtparam=i2c_arm_baudrate=100000`.
- Deshabilitar HDMI (cuando no se usa) y ahorrar GPU:
  `hdmi_force_hotplug=0`, `hdmi_group=0`, `hdmi_mode=0`, `disable_splash=1`,
  `gpu_mem=16`. Verificar apagado: no debe existir `/sys/class/drm/*`.
- **Importante**: si hay un "bonnet" ST7789 SPI montado sobre el header de 40
  pines, cubre los pines 3 y 5 (GPIO2/3) donde va el I2C del OLED. Si el bonnet
  ya no se usa, retirarlo para liberar los pines.
- Después de editar `/boot/config.txt` hay que **reiniciar** (`sudo reboot`) y
  verificar con `sudo i2cdetect -y 1` (el display debe aparecer en `0x3C`).

## Diagnóstico del I2C

- `sudo i2cdetect -y 1` → detecta dispositivos en el bus 1.
- `sudo i2cget -y 1 0x3c` / `sudo i2cset -y 1 0x3c 0x00 0xAF` → prueba directa.
- Si el bus está **completamente vacío** (ninguna dirección responde) aunque el
  display "esté conectado", el problema es eléctrico (cableado SDA/SCL, VCC,
  GND o dirección `0x3D`), no de configuración.
- NACK de bcm2835 (`Error code 1`) distinto de NACK de `/dev/i2c`: el primero
  es incompatibilidad de librería, el segundo es display ausente/cableado.

## Versionado con tags (regla del proyecto)

- **Todo push lleva su tag.** No se pushea sin tag.
- `VERSION` (archivo en raíz) debe coincidir con el último tag (con y sin `v`):
  `git tag v1.2.0` → `VERSION` = `1.2.0`.
- Un tag de versión por rama/push. Commits significativos → tag secuencial
  (patch 0-9, luego minor: `v1.0.9` → `v1.1.0`). No retroceder versiones.
- Commit convencional: `feat:`, `fix:`, `docs:`, `refactor:`, `chore:`, etc.
- Flujo: actualizar `VERSION` → `git add -A` → `git commit -m "tipo: ..."` →
  `git push origin <branch>` → `git tag -a vX.Y.Z -m "..."` → `git push origin vX.Y.Z`.
- Estado actual: `main`→`v1.0.0`, `crossover`→`v1.1.0`, `v1.2.0`, `v1.3.0`, `v1.3.1`.
- No eliminar tags publicados; ante error, crear el siguiente número.

## Errores resueltos (NO repetir)

| Error | Causa | Fix |
|-------|-------|-----|
| Segfault en OLEDclearBuffer | `SSD1306` no asignaba memoria al buffer (era nullptr) | `malloc` en constructor, `free` en destructor |
| Segfault al usar `--version` | destructor tocaba I2C sin init | flag `hwReady_` |
| `bcm2835I2CReasonCodes :: Error code 1` en CM5 | bcm2835 incompatible con RP1 | capa `/dev/i2c-N` con ioctl |
| `Pending o NACK` en rpi2w / Zero 2W | display no conectado eléctricamente al bus 1 | revisar cableado (SDA↔GPIO2, SCL↔GPIO3, VCC, GND) |
| Conflicto macro `swap` con `<new>`/STL | `SSD1306_OLED_graphics.hpp` define `swap(a,b)` global | `#undef swap` tras incluir, y no usar `new[]`/`<memory>` antes |
| `Cannot start I2C` | falta `bcm2835_init()` o no hay acceso a /dev/i2c | en capa actual: abrir `/dev/i2c-1` con permisos (sudo/grupo i2c) |
| `Permission denied` abrir /dev/i2c-1 | usuario sin permiso | ejecutar con sudo o agregar al grupo `i2c` |
| Display no detectado tras config | bonnet SI ocupa pines 3/5 | retirar bonnet; verificar en `0x3C`/`0x3D` |

## Comandos útiles

```sh
read -s SSHPASS; export SSHPASS            # setear contraseña SSH (no guardar)
sshpass -e ssh pi@cm5.local    "sudo i2cdetect -y 1"
sshpass -e ssh pi@rpi2w.local  "cd ~/src/basic && git pull && make -j4 && sudo ./bin/App"
make crossover64                            # compilar ARM64 local
make remote                                 # compilar remoto (config/deploy.cfg)
```
