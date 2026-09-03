#!/bin/bash
# =============================================================================
# fetch_sysroot.sh - Trae bcm2835 de la Raspberry Pi para compilación cruzada
# -----------------------------------------------------------------------------
# Copia desde una Raspberry Pi remota:
#   - /usr/include/bcm2835.h
#   - /usr/lib/aarch64-linux-gnu/libbcm2835.a  (lib estática aarch64)
# a un "sysroot" local (sysroot/<triple>/) que el Makefile usa para compilar
# en cruz (make ARCH=cross) en un PC x86-64.
#
# Requisitos:
#   - sshpass instalado.
#   - Variable SSHPASS con la contraseña del usuario remoto (sin divulgarse).
# =============================================================================
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEPLOY_CFG="$ROOT_DIR/config/deploy.cfg"

# triple por defecto (ARM64 Raspberry Pi de 64 bits)
TRIPLE="${1:-aarch64-linux-gnu}"

if ! command -v sshpass >/dev/null 2>&1; then
    echo "ERROR: 'sshpass' no está instalado (sudo apt install sshpass)."
    exit 1
fi

if [[ -z "${SSHPASS:-}" ]]; then
    echo "ERROR: La variable SSHPASS no está definida."
    echo "  Ejemplo:  read -s SSHPASS; export SSHPASS"
    exit 1
fi

# shellcheck disable=SC1090
source "$DEPLOY_CFG"

for var in REMOTE_USER REMOTE_HOST; do
    if [[ -z "${!var:-}" ]]; then
        echo "ERROR: '$var' no está definida en $DEPLOY_CFG"
        exit 1
    fi
done

REMOTE_TARGET="$REMOTE_USER@$REMOTE_HOST"

# Directorios del sysroot local
SYSROOT="$ROOT_DIR/sysroot/$TRIPLE"
INC_DIR="$SYSROOT/usr/include"
LIB_DIR="$SYSROOT/usr/lib/$TRIPLE"

# Nombres "corregidos" una vez traídos localmente
INC_LOCAL="$INC_DIR/bcm2835.h"
LIB_LOCAL="$LIB_DIR/libbcm2835.a"

# Localizar la librería en la Raspberry Pi (en 32 y 64 bits)
echo ">>> Localizando bcm2835 en $REMOTE_TARGET ..."
LIB_REMOTE=$(
  sshpass -e ssh -o StrictHostKeyChecking=no -o ConnectTimeout=20 \
    "$REMOTE_TARGET" \
    'ls -1 /usr/lib/aarch64-linux-gnu/libbcm2835.a \
           /usr/lib/arm-linux-gnueabihf/libbcm2835.a 2>/dev/null | head -n1'
)

if [[ -z "$LIB_REMOTE" ]]; then
    echo "ERROR: No se encontró libbcm2835.a en la Raspberry Pi."
    echo "  Instálala con: sudo apt install libbcm2835-dev"
    exit 1
fi

# Crear estructura local
mkdir -p "$INC_DIR" "$LIB_DIR"

echo ">>> Descargando cabecera bcm2835.h ..."
scp -q -o StrictHostKeyChecking=no "$REMOTE_TARGET:/usr/include/bcm2835.h" "$INC_LOCAL"

echo ">>> Descargando $LIB_REMOTE ..."
scp -q -o StrictHostKeyChecking=no "$REMOTE_TARGET:$LIB_REMOTE" "$LIB_LOCAL"

echo ""
echo ">>> Sysroot listo:"
echo "    Include: $INC_LOCAL"
echo "    Librería: $LIB_LOCAL"
echo ""
echo "    Ahora puedes compilar en cruz con:  make ARCH=cross"
