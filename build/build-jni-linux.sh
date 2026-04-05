#!/usr/bin/env bash
# ================================================================
# build-jni-linux.sh
# Compiles the JNI bridge into librdvc_jni.so (Linux x86-64)
#
# Requirements:
#   - JAVA_HOME must point to JDK 21+
#   - librdvc_core.so must already be in build/output/
#   - GCC installed
#   - Run from the project root
# ================================================================

set -euo pipefail

if [[ -z "${JAVA_HOME:-}" ]]; then
    echo "[ERROR] JAVA_HOME is not set."
    exit 1
fi

OUTDIR="build/output"
JNISRC="jni/src"
COREINC="core/include"
JAVA_INC="$JAVA_HOME/include"
JAVALIN_INC="$JAVA_HOME/include/linux"
OUTLIB="$OUTDIR/librdvc_jni.so"
CORELIB="$OUTDIR/librdvc_core.so"

if [[ ! -f "$CORELIB" ]]; then
    echo "[ERROR] $CORELIB not found — run build-core-linux.sh first."
    exit 1
fi

echo "[RDVC] Building JNI Bridge (Linux x86-64)..."

SOURCES=()
while IFS= read -r -d '' f; do SOURCES+=("$f"); done \
    < <(find "$JNISRC" -name "*.c" -print0)

gcc -shared -fPIC -O2 \
    -DRDVC_LINUX \
    -I"$COREINC" \
    -I"jni/include" \
    -I"$JAVA_INC" \
    -I"$JAVALIN_INC" \
    "${SOURCES[@]}" \
    -L"$OUTDIR" -lrdvc_core \
    -lm \
    -o "$OUTLIB"

echo "[OK] Output: $OUTLIB"
echo
echo "IMPORTANT: Add build/output to LD_LIBRARY_PATH, or pass"
echo "           -Djava.library.path=build/output to the JVM."
