# ── Wokwi Custom Chips – WASM build ──────────────────────────────
# Default: Docker image wokwi/builder-clang-wasm (clang 17, /opt/wasi-libc)
#
# ▸ Full rebuild (Docker):
#     docker run --rm -v "$PWD/chips:/src" wokwi/builder-clang-wasm:latest make -B
#
# ▸ Incremental build (Docker):
#     docker run --rm -v "$PWD/chips:/src" wokwi/builder-clang-wasm:latest make
#
# ▸ Via helper script (auto-detects Docker → native fallback):
#     ./tools/build-chips.sh          # incremental
#     ./tools/build-chips.sh -B       # full rebuild
#
# ▸ Native build without Docker (macOS – requires brew install llvm wasi-libc):
#     make CC=/opt/homebrew/opt/llvm/bin/clang \
#          WASI_SYSROOT=/opt/homebrew/opt/wasi-libc/share/wasi-sysroot \
#          EXTRA_FLAGS="-nodefaultlibs -lc"
#
# ▸ Native build without Docker (Linux – requires apt install lld clang wasi-libc):
#     make WASI_SYSROOT=/usr/share/wasi-sysroot
# ─────────────────────────────────────────────────────────────────

CC = clang
CFLAGS = --target=wasm32-unknown-wasi --sysroot=/opt/wasi-libc \
         -nostartfiles -Wl,--import-memory -Wl,--export-table -Wl,--no-entry \
         -Werror -I.

# Override sysroot for native (non-Docker) builds
ifdef WASI_SYSROOT
  CFLAGS = --target=wasm32-unknown-wasi --sysroot=$(WASI_SYSROOT) \
           -nostartfiles $(EXTRA_FLAGS) \
           -Wl,--import-memory -Wl,--export-table -Wl,--no-entry -Werror -I.
endif

CHIPS = $(wildcard incu-*/incu-*.c)
WASMS = $(CHIPS:.c=.chip.wasm)

all: $(WASMS)

%.chip.wasm: %.c wokwi-api.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(WASMS)

.PHONY: all clean
