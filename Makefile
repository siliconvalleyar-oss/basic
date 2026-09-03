# =============================================================================
# Makefile - Aplicación BASIC para Raspberry Pi (pantalla OLED SSD1306)
# -----------------------------------------------------------------------------
# Uso:
#   make                -> compilación nativa (usa g++ del sistema)
#   make crossover      -> compilación cruzada ARM detectando arquitectura actual
#                          (x86_64 → ARM64 aarch64-linux-gnu)
#                          (aarch64 → ARM32 arm-linux-gnueabihf)
#   make crossover32    -> compilación cruzada ARM 32-bit (arm-linux-gnueabihf)
#   make crossover64    -> compilación cruzada ARM 64-bit (aarch64-linux-gnu)
#   make run            -> ejecuta el binario local (bin/App, muestra versión)
#   make clean          -> elimina objetos y binario
#   make distclean      -> elimina objetos, binario y sysroot cruzado
#   make ARCH=cross CROSS_TRIPLE=aarch64-linux-gnu  -> cruce explícito
#
# Nota sobre compilación cruzada:
#   Para compilar en cruz se necesita la librería bcm2835 para la arquitectura
#   de destino, que se obtiene ejecutando:
#       scripts/fetch_sysroot.sh [aarch64-linux-gnu | arm-linux-gnueabihf]
#   (trae libbcm2835.a y bcm2835.h desde la Raspberry Pi a sysroot/<triple>/)
# =============================================================================

# --- Versión (leída del archivo VERSION) ------------------------------------
VERSION := $(shell cat VERSION 2>/dev/null || echo "0.0.0")

# --- Directorios -------------------------------------------------------------
BUILD_DIR := obj
BIN_DIR   := bin
SRC_DIR   := src
INC_DIR   := include
SYSROOT_DIR := sysroot
TARGET    := App
BINARY    := $(BIN_DIR)/$(TARGET)

# --- Archivos fuente (todos los .cpp de src/ y subdirectorios) ---------------
SRCS := $(shell find $(SRC_DIR) -name '*.cpp')
# Objetos equivalentes manteniendo la jerarquía dentro de obj/ (ej. obj/src/main.o)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/$(SRC_DIR)/%.o,$(SRCS))

# --- Arquitección: native o cross --------------------------------------------
ARCH ?= native

# Triple del compilador cruzado (ARM64 para Raspberry Pi de 64 bits por defecto)
CROSS_TRIPLE ?= aarch64-linux-gnu

# --- Selección de compilador y flags según arquitectura ----------------------
ifeq ($(ARCH),cross)
  CC  := $(CROSS_TRIPLE)-g++
  # Sysroot de la Raspberry Pi (contiene libbcm2835.a y bcm2835.h)
  SYSROOT   := $(SYSROOT_DIR)/$(CROSS_TRIPLE)
  # Indicar al compilador dónde están include y libs de la arquitectura destino
  SYSROOT_FLAGS := --sysroot=$(abspath $(SYSROOT))
  SYSROOT_INC   := $(SYSROOT)/usr/include
  SYSROOT_LIB   := $(SYSROOT)/usr/lib/$(CROSS_TRIPLE)
  CPPFLAGS += -I$(SYSROOT_INC) $(SYSROOT_FLAGS)
  LDFLAGS  += -L$(SYSROOT_LIB) $(SYSROOT_FLAGS)
else
  # Compilación nativa: el compilador del sistema.
  # (En una Raspberry Pi nativa usa g++ y libbcm2835 instalada en el sistema.)
  CC := g++
endif

# --- Flags comunes ------------------------------------------------------------
# -DVERSION para mostrar la versión en tiempo de compilación (macro).
CPPFLAGS += -DVERSION="\"$(VERSION)\"" -I$(INC_DIR) -I$(INC_DIR)/core -I$(INC_DIR)/oled

# Estándar C++17 y advertencias. sin -Werror para no romper el build.
CXXFLAGS += -std=c++17 -Wall -Wextra -O2

# Librerías necesarias: bcm2835 (hardware RPi) + matemáticas (usada por gráficos).
LIBS := -lbcm2835 -lm

# --- Detección de arquitectura local para compilación cruzada automática ------
# uname -m → aarch64 / armv7l / x86_64 / i686 / ...
HOST_ARCH := $(shell uname -m)

# Mapeo de arquitectura local → triple del compilador cruzado:
#   x86_64 / i686  → compila para ARM64 (RPi moderna de 64 bits)
#   aarch64        → compila para ARM32 (compatibilidad con RPi viejas)
#   armv7l / armhf → compila para ARM64 (cross de 32→64 bits)
#   Otros          → no soportado
ifeq ($(filter x86_64 i686,$(HOST_ARCH)),)
  # Estamos en ARM (32 o 64): cruzamos al otro tamaño.
  ifeq ($(HOST_ARCH),aarch64)
    CROSS_DETECT := arm-linux-gnueabihf
  else
    # armv7l / armhf → cruzar a 64 bits
    CROSS_DETECT := aarch64-linux-gnu
  endif
else
  # Estamos en x86: cruzamos a ARM64 (el más común para RPi moderna).
  CROSS_DETECT := aarch64-linux-gnu
endif

# --- Objetivos por defecto ---------------------------------------------------
.PHONY: all clean distclean fetch sysroot cross crossover crossover32 crossover64 run info

all: $(BINARY)

# Crea los directorios de objetos antes de compilar (con mkdir -p dentro de la
# receta para ser robusto incluso con builds en paralelo).
# Regla de compilación: genera cada objeto desde su fuente manteniendo la ruta.
$(BUILD_DIR)/$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Regla de enlazado: genera el binario final.
$(BINARY): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o $(BINARY)
	@echo "Build completado: $(BINARY)"
	@echo "Versión: $(VERSION) | Arquitectura: $(ARCH)"

# --- Objetivos auxiliares -----------------------------------------------------

# Limpia objetos y binario.
clean:
	rm -rf $(BUILD_DIR) $(BINARY)
	@echo "Limpieza completada."

# Limpia objetos, binario y sysroot cruzado descargado.
distclean: clean
	rm -rf $(SYSROOT_DIR)
	@echo "Limpieza total completada."

# Trae la librería bcm2835 desde la Raspberry Pi para compilación cruzada local.
fetch:
	./scripts/fetch_sysroot.sh

# Alias de fetch para el objetivo mnemónico "sysroot".
sysroot: fetch

# Compilación cruzada detectando arquitectura automáticamente.
# En x86_64 → compila para ARM64 (aarch64-linux-gnu).
# En aarch64 → compila para ARM32 (arm-linux-gnueabihf).
crossover:
	$(MAKE) ARCH=cross CROSS_TRIPLE=$(CROSS_DETECT) all

# Compilación cruzada para ARM 32-bit (armv7/armhf).
crossover32:
	$(MAKE) ARCH=cross CROSS_TRIPLE=arm-linux-gnueabihf all

# Compilación cruzada para ARM 64-bit (aarch64).
crossover64:
	$(MAKE) ARCH=cross CROSS_TRIPLE=aarch64-linux-gnu all

# Copia del binario a la Raspberry Pi mediante scp (requiere sshpass y $SSHPASS).
deploy:
	./scripts/deploy.sh

# Compila de forma remota en la Raspberry Pi (git pull + make clean + make).
remote:
	./scripts/build_remote.sh

# Ejecuta el binario local. Soporta argumentos (p.ej. make run ARGS="--version").
run: $(BINARY)
	$(BINARY) $(ARGS)

# Muestra información de configuración.
info:
	@echo "VERSION=$(VERSION)"
	@echo "ARCH=$(ARCH)"
	@echo "CC=$(CC)"
	@echo "Objetos: $(OBJS)"

# --- Inclusión de dependencias generadas (para rebuild incremental) ----------
-include $(OBJS:.o=.d)
