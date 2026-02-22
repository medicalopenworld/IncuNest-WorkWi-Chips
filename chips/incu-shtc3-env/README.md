# incu-shtc3-env

Chip personalizado para simular el sensor ambiental Sensirion SHTC3 (temperatura y humedad) en IncuNest.

## Características

- I2C esclavo fijo en `0x70`
- Comandos SHTC3 principales: wakeup, sleep, medición, lectura de ID y soft reset
- CRC-8 Sensirion (`poly=0x31`, init `0xFF`)
- Controles configurables en Wokwi:
  - `temperature` (°C, por defecto `25.0`)
  - `humidity` (%RH, por defecto `45.0`)

## Pines

- `SDA`
- `SCL`
- `VCC`
- `GND`
