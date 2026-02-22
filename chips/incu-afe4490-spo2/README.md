# IncuNest AFE4490 SpO2 – Chip personalizado Wokwi

Simulación del front-end analógico **TI AFE4490** para pulsioximetría (SpO2 y frecuencia cardíaca), utilizado como **U17 (AFE4490RHAR)** en la PCB de la incubadora neonatal IncuNest.

## Descripción

El AFE4490 integra un ADC de 22 bits con tres canales (LED rojo, LED infrarrojo y luz ambiente) y se comunica con el ESP32-S3 vía SPI. Este chip personalizado genera valores ADC pulsátiles realistas a partir de los parámetros de SpO2 y frecuencia cardíaca configurados.

La relación R (rojo/infrarrojo) se calcula como:

```
R = (AC_rojo / DC_rojo) / (AC_ir / DC_ir)
SpO2 ≈ 110 − 25 × R
```

## Pines

| Pin       | Dirección | Descripción                                      |
|-----------|-----------|--------------------------------------------------|
| `CS`      | Entrada   | Chip Select SPI (activo bajo)                    |
| `MOSI`    | Entrada   | Master Out Slave In                              |
| `MISO`    | Salida    | Master In Slave Out                              |
| `SCK`     | Entrada   | Reloj SPI                                        |
| `ADC_RDY` | Salida    | Pulso bajo cuando hay nueva muestra (100 Hz)     |
| `VCC`     | Poder     | Alimentación (3.0–3.6 V)                         |
| `GND`     | Poder     | Tierra                                           |

## Protocolo SPI

- **Modo**: SPI Mode 0 (CPOL=0, CPHA=0)
- **Escritura**: byte de dirección (bit 7 = 0) + 3 bytes de datos (MSB primero)
- **Lectura**: byte de dirección (bit 7 = 1) + 3 bytes dummy → 3 bytes de datos retornados en MISO

### Registros principales

| Dirección | Nombre        | Descripción                              |
|-----------|---------------|------------------------------------------|
| `0x00`    | CONTROL0      | Reset por SW, habilitación de timer      |
| `0x1E`    | CONTROL1      | Habilitación de timer, promediado ADC    |
| `0x20`    | TIAGAIN       | Ganancia TIA, retroalimentación RF       |
| `0x22`    | LEDCNTRL      | Corriente de LEDs                        |
| `0x23`    | CONTROL2      | Configuración TX/RX                      |
| `0x2A`    | LED2VAL       | Dato ADC del LED2 (IR), 22 bits (solo lectura) |
| `0x2B`    | ALED2VAL      | Ambiente LED2 (solo lectura)             |
| `0x2C`    | LED1VAL       | Dato ADC del LED1 (Rojo), 22 bits (solo lectura) |
| `0x2D`    | ALED1VAL      | Ambiente LED1 (solo lectura)             |
| `0x2E`    | LED2-ALED2VAL | Diferencial IR (solo lectura)            |
| `0x2F`    | LED1-ALED1VAL | Diferencial Rojo (solo lectura)          |

## Atributos (Controles de simulación)

| Atributo        | Tipo    | Rango     | Defecto | Descripción                          |
|-----------------|---------|-----------|---------|--------------------------------------|
| `spo2`          | entero  | 70–100    | 97      | Saturación de oxígeno objetivo (%)   |
| `heartRate`     | entero  | 40–220    | 120     | Frecuencia cardíaca objetivo (BPM)   |
| `signalQuality` | entero  | 0–100     | 90      | Calidad de señal (modula amplitud AC)|

> Se añade variación aleatoria automática de ±1 % SpO2 y ±2 BPM para mayor realismo.

## Conexión con el ESP32-S3 (HW v15)

```
ESP32-S3 GPIO 21  → CS       (AFE44XX_CS)
ESP32-S3 GPIO 45  → ADC_RDY  (AFE4490_ADC_READY, interrupción)
ESP32-S3 SPI MOSI → MOSI
ESP32-S3 SPI MISO → MISO
ESP32-S3 SPI SCK  → SCK
```

## Compilación

```bash
# Dentro del contenedor Docker:
docker run --rm -v "$PWD/chips:/src" wokwi/builder-clang-wasm:latest make

# O de forma nativa (macOS):
make CC=/opt/homebrew/opt/llvm/bin/clang \
     WASI_SYSROOT=/opt/homebrew/opt/wasi-libc/share/wasi-sysroot \
     EXTRA_FLAGS="-nodefaultlibs -lc"
```

## Referencia

- [TI AFE4490 Datasheet (SBAS602B)](https://www.ti.com/lit/ds/sbas602b/sbas602b.pdf)
