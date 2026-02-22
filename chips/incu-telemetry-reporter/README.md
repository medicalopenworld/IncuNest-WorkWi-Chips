# incu-telemetry-reporter

Custom Wokwi chip that reads incubator sensor values from chip controls (sliders)
and sends them as JSON telemetry over UART TX.

## Purpose

Bridges the gap between Wokwi simulation and the 3D viewer. The chip reads
attribute values configured via Wokwi's UI sliders and periodically transmits
them as a JSON object to the ESP32 via UART, which the firmware echoes to the
RFC2217 port for the `wokwi-bridge.mjs` to forward to the 3D viewer.

## Pins

| Pin     | Direction | Description                        |
|---------|-----------|------------------------------------|
| TX      | Output    | UART TX → ESP32 RX2 (GPIO16/48)   |
| VCC     | Input     | Power                              |
| GND     | Input     | Ground                             |
| DOOR_IN | Input     | Digital door-open sensor (optional)|

## Controls (Wokwi sliders)

| Control       | Range     | Step | Description          |
|---------------|-----------|------|----------------------|
| chamberTemp   | 20–42 °C  | 0.1  | Chamber air temp     |
| skinTemp      | 28–40 °C  | 0.1  | Baby skin temp       |
| humidity      | 10–100 %  | 1    | Relative humidity    |
| fanRpm        | 0–4000    | 50   | Fan speed            |
| heaterDuty    | 0–100 %   | 1    | Heater duty cycle    |
| doorOpen      | 0–1       | 1    | Door state           |
| alarm         | 0–5       | 1    | Alarm code           |

## JSON Output Format

Transmitted every 500 ms at 115200 baud:

```json
{"chamberTemp":36.5,"skinTemp":36.8,"humidity":65,"fanRpm":2000,"heaterDuty":45,"doorOpen":0,"alarm":0}
```

## Build

```bash
docker run --rm -v "$PWD/chips:/src" wokwi/builder-clang-wasm:latest make
```
