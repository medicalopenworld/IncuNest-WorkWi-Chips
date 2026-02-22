#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CHIPS_DIR="$ROOT_DIR/chips"
MAKE_ARGS="${*:--}" # pass all args to make (e.g. -B for full rebuild)
[ "$MAKE_ARGS" = "-" ] && MAKE_ARGS=""

# ── 1. Docker (preferred — platform-independent) ─────────────────
if command -v docker >/dev/null 2>&1 && docker info >/dev/null 2>&1; then
  echo "🐳 Compilando chips con Docker (wokwi/builder-clang-wasm)..."
  docker run --rm -u "$(id -u):$(id -g)" -v "$CHIPS_DIR:/src" \
    wokwi/builder-clang-wasm:latest make $MAKE_ARGS
  echo "✅ Todos los chips compilados correctamente."
  exit 0
fi

# ── 2. Native LLVM (fallback — requires local toolchain) ─────────
# macOS:  brew install llvm wasi-libc
# Linux:  apt install lld clang wasi-libc  (or download wasi-sdk)
detect_native_clang() {
  # macOS Homebrew
  if [ -x /opt/homebrew/opt/llvm/bin/clang ]; then
    CLANG=/opt/homebrew/opt/llvm/bin/clang
    SYSROOT=/opt/homebrew/opt/wasi-libc/share/wasi-sysroot
    EXTRA="-nodefaultlibs -lc"
    return 0
  fi
  # Linux system clang with wasm support
  if clang --print-targets 2>/dev/null | grep -q wasm32; then
    CLANG=clang
    for sr in /usr/share/wasi-sysroot /opt/wasi-libc; do
      [ -d "$sr" ] && SYSROOT="$sr" && EXTRA="" && return 0
    done
  fi
  # wasi-sdk
  if [ -d "${WASI_SDK_PATH:-/opt/wasi-sdk}" ]; then
    local sdk="${WASI_SDK_PATH:-/opt/wasi-sdk}"
    CLANG="$sdk/bin/clang"
    SYSROOT="$sdk/share/wasi-sysroot"
    EXTRA=""
    return 0
  fi
  return 1
}

if detect_native_clang; then
  echo "🔧 Compilando chips con clang nativo ($CLANG)..."
  make -C "$CHIPS_DIR" CC="$CLANG" WASI_SYSROOT="$SYSROOT" EXTRA_FLAGS="$EXTRA" $MAKE_ARGS
  echo "✅ Todos los chips compilados correctamente."
  exit 0
fi

# ── 3. wokwi-cli (last resort) ───────────────────────────────────
if command -v wokwi-cli >/dev/null 2>&1; then
  echo "🔌 Compilando chips con wokwi-cli..."
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

# ── No toolchain found ───────────────────────────────────────────
echo "⚠️  No se encontró toolchain de compilación."
echo ""
echo "Opciones (en orden de preferencia):"
echo "  1. Docker (recomendado, multiplataforma):"
echo "       docker run --rm -v \"\$PWD/chips:/src\" wokwi/builder-clang-wasm:latest make -B"
echo "  2. macOS nativo:  brew install llvm wasi-libc"
echo "  3. Linux nativo:  apt install lld clang wasi-libc"
echo "  4. wasi-sdk:      https://github.com/WebAssembly/wasi-sdk/releases"
echo "  5. wokwi-cli:     https://github.com/wokwi/wokwi-cli"
exit 1
