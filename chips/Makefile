CC = clang
CFLAGS = --target=wasm32-unknown-wasi --sysroot=/opt/wasi-libc \
         -nostartfiles -Wl,--import-memory -Wl,--export-table -Wl,--no-entry \
         -Werror -I.

CHIPS = $(wildcard incu-*/incu-*.c)
WASMS = $(CHIPS:.c=.chip.wasm)

all: $(WASMS)

%.chip.wasm: %.c wokwi-api.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(WASMS)

.PHONY: all clean
