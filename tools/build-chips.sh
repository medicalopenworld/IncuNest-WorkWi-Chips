#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if ! command -v wokwi-chip-builder >/dev/null 2>&1; then
  echo "wokwi-chip-builder no encontrado. Generando placeholders .chip.wasm..."
  node "$ROOT_DIR/tools/generate-placeholder-wasm.mjs"
  echo "Placeholders generados. Instala wokwi-chip-builder para compilación real."
  exit 0
fi

mapfile -t CHIP_C_FILES < <(find "$ROOT_DIR/chips" -maxdepth 2 -type f -name "*.c" | sort)

for chip_c in "${CHIP_C_FILES[@]}"; do
  chip_dir="$(dirname "$chip_c")"
  chip_name="$(basename "$chip_c" .c)"
  out_file="$chip_dir/$chip_name.chip.wasm"
  echo "Compilando $chip_name..."
  wokwi-chip-builder "$chip_c" -o "$out_file"
done

echo "Compilación finalizada."
