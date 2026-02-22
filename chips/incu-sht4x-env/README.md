# incu-sht4x-env

Sensor ambiental SHT4x virtual (temperatura y humedad) por I2C.

## Pines

- `VCC`
- `GND`
- `SCL`
- `SDA`
- `HUMIDITY_INFLUENCE` (entrada analógica de humedad extra, 0..3.3V = 0..100% RH)
- `TEMP_OUT` (señal analógica auxiliar para telemetría)
- `HUM_OUT` (señal analógica auxiliar para telemetría)

## Controles

- `temperature` (°C)
- `humidity` (%) como humedad de referencia base

Humedad efectiva simulada:

`humidity_effective = clamp(humidity_ref + humidity_influence, 0..100)`
