# incu-telemetry-reporter

Custom Wokwi chip that reads live sensor/actuator signals from pins and sends
them as JSON telemetry over UART TX.

## Purpose

Bridges the gap between Wokwi simulation and both display + 3D viewer. The chip:
- samples connected sensor/actuator signals and transmits JSON telemetry,
- receives HMI return commands (`HMI,...`) from the display,
- emits `CTRL,STATE,...` feedback to emulate motherboard response,
- drives a humidifier override output from humidity setpoint feedback.

## Pins

| Pin           | Direction | Description |
|---------------|-----------|-------------|
| RX            | Input     | UART RX ← `display:TX` (HMI return channel) |
| TX            | Output    | UART TX → `display:RX` y `esp32:48` (bridge 3D) |
| VCC           | Input     | Power |
| GND           | Input     | Ground |
| CHAMBER_IN    | Input     | Analog chamber temp (from `sht4x:TEMP_OUT`) |
| SKIN_IN       | Input     | Analog skin temp (from `ntc:OUT`) |
| HUMIDITY_IN   | Input     | Analog humidity (from `sht4x:HUM_OUT`) |
| FAN_TACH_IN   | Input     | Fan tach pulses (from `fan:TACH_OUT`) |
| HEATER_PWM_IN | Input     | Heater PWM line |
| DOOR_IN       | Input     | Door analog signal (from `door:OUT`) |
| ALARM_IN      | Input     | Alarm/buzzer drive line |
| HUMIDIFIER_DUTY_OUT | Output | Analog duty override (0..3.3V = 0..95%) |
| HUMIDIFIER_OVERRIDE_EN | Output | Enable override hacia `incu-humidifier` |

## JSON Output Format

Transmitted every 500 ms at 115200 baud:

```json
{"temp":34.2,"skin":36.6,"hum":58.0,"fan":1200,"heater":42,"door":0,"alarm":0}
```

Also emits MB-style state frames when HMI link is active:

```text
CTRL,STATE,<act>,<mode>,<airSet>,<skinSet>,<humSet>,...
```

## Build

```bash
docker run --rm -v "$PWD/chips:/src" wokwi/builder-clang-wasm:latest make
```
