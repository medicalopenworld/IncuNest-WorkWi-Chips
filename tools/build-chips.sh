#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CHIPS_DIR="$ROOT_DIR/chips"

# Preferred: Docker with wokwi/builder-clang-wasm
if command -v docker >/dev/null 2>&1 && docker info >/dev/null 2>&1; then
  echo "Compilando chips con Docker (wokwi/builder-clang-wasm)..."
  docker run --rm -u "$(id -u):$(id -g)" -v "$CHIPS_DIR:/src" \
    wokwi/builder-clang-wasm:latest make
  echo "✅ Todos los chips compilados correctamente."
  exit 0
fi

# Fallback: wokwi-cli
if command -v wokwi-cli >/dev/null 2>&1; then
  echo "Compilando chips con wokwi-cli..."
  for chip_c in "$CHIPS_DIR"/*/incu-*.c; do
    chip_dir="$(dirname "$chip_c")"
    chip_name="$(basename "$chip_c" .c)"
    out_file="$chip_dir/$chip_name.chip.wasm"
    echo "  $chip_name..."
    wokwi-cli chip compile "$chip_c" -o "$out_file"
  done
  echo "✅ Compilación finalizada."
  exit 0
fi

echo "⚠️  Ni Docker ni wokwi-cli encontrados."
echo "   Instala Docker o descarga wokwi-cli de https://github.com/wokwi/wokwi-cli"
echo "   Generando placeholders .chip.wasm (no funcionarán en simulación)..."
node "$ROOT_DIR/tools/generate-placeholder-wasm.mjs"
exit 1
