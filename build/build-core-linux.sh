#!/usr/bin/env bash
# ================================================================
# build-core-linux.sh
# Compiles the RDVC C core library into librdvc_core.so (Linux x86-64)
#
# Requirements:
#   - GCC (build-essential) installed
#   - Run from the project root:
#       Raw Disk Viwer CPLX/
# ================================================================

set -euo pipefail

OUTDIR="build/output"
SRCDIR="core/src"
INCDIR="core/include"
OUTLIB="$OUTDIR/librdvc_core.so"

mkdir -p "$OUTDIR"

echo "[RDVC] Building C Core (Linux x86-64)..."

SOURCES=()
while IFS= read -r -d '' f; do SOURCES+=("$f"); done \
    < <(find "$SRCDIR" -name "*.c" -print0)

gcc -shared -fPIC -O2 \
    -DRDVC_LINUX \
    -I"$INCDIR" \
    "${SOURCES[@]}" \
    -lm \
    -o "$OUTLIB"

echo "[OK] Output: $OUTLIB"
