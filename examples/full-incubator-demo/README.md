# IncuNest Simulación — Hardware v14 (ESP32)

> ⚠️ **Esta simulación está configurada para Hardware v14 (ESP32-WROOM-32).** Para la versión actual v15 (ESP32-S3), ver [`../full-incubator-demo-v15/`](../full-incubator-demo-v15/).

Demo completa de la incubadora virtual usando `firmware.bin` y todos los custom chips.

## Mapeo de Pines v14 (board.h HW_NUM==14)

| Señal | GPIO | Componente |
|-------|------|------------|
| I2C SDA | 21 | STS35×2, SHT4x, INA3221×2, Humidifier |
| I2C SCL | 22 | (bus compartido) |
| Fan PWM | 12 | incu-fan-pwm |
| Fan Tach | 35 | incu-fan-pwm |
| Actuators EN | 14 | incu-fan-pwm |
| Heater SSR | 27 | incu-heater-ssr |
| Buzzer | 5 | wokwi-buzzer |
| Baby NTC | 39 | incu-ntc-skin |
| Door Touch | 32 | wokwi-slide-switch |
| Encoder A | 25 | wokwi-ky-040 |
| Encoder B | 34 | wokwi-ky-040 |
| Encoder SW | 4 | wokwi-ky-040 |

### Direcciones I2C

| Dispositivo | Dirección |
|-------------|-----------|
| STS35 (air) | 0x4A (74) |
| STS35 (skin) | 0x4B (75) |
| SHT4x | 0x44 |
| INA3221 (main) | 0x40 (64) |
| INA3221 (sec) | 0x41 (65) |
| Humidifier | 0x02 (2) |

### Limitaciones conocidas

- El firmware binario (`firmware.bin`) está compilado para ESP32-S3 (v15). En Wokwi, la simulación sobre ESP32 v14 es aproximada.
- En v14, el display usaba SPI (TFT_eSPI). La simulación usa el chip UART HMI como aproximación visual.
- El display y telemetry reporter reciben datos del chip telemetry, no del firmware directamente.

## Inicio Rápido

1. Desde la raíz del repo: `./tools/build-chips.sh`
2. Abre esta carpeta en VS Code.
3. Ejecuta `Wokwi: Start Simulator`.

## Contenidos

- `diagram.json` — Esquema Wokwi con ESP32 v14 (15 partes)
- `wokwi.toml` — Configuración Wokwi (rutas relativas `../../`)
