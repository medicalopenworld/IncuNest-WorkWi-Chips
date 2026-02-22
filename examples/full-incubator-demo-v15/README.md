# IncuNest Simulación — Hardware v15 (ESP32-S3)

> ✅ **Versión principal.** Usa ESP32-S3 (`board-esp32-s3-devkitc-1`) con pinout v15.
> Para la versión anterior v14 (ESP32 clásico), ver `../full-incubator-demo/`.

## Componentes (19 partes)

### Sensores (7)
| Componente | Chip | Protocolo | Pin/Dir |
|-----------|------|-----------|---------|
| NTC 10k (piel) | `incu-ntc-skin` | Analog | GPIO 10 |
| STS35 (aire principal) | `incu-sts35-temp` | I2C | 0x4A |
| STS35 (aire redundante) | `incu-sts35-temp` | I2C | 0x4B |
| SHT4x (ambiente cámara) | `incu-sht4x-env` | I2C | 0x44 |
| SHTC3 (ambiente externo) | `incu-shtc3-env` | I2C | 0x70 |
| INA3221 (corriente principal) | `incu-ina3221` | I2C | 0x40 |
| INA3221 (corriente secundario) | `incu-ina3221` | I2C | 0x41 |

### Actuadores (5)
| Componente | Chip | Pin |
|-----------|------|-----|
| Heater SSR | `incu-heater-ssr` | GPIO 16 |
| Fan PWM + TACH | `incu-fan-pwm` | GPIO 12 / 38 |
| Buzzer | `wokwi-buzzer` (nativo) | GPIO 5 |
| Fototerapia LED | `incu-phototherapy` | GPIO 13 |
| Sensor puerta | `incu-door-touch` | GPIO 1/2 |

### Clínicos (1)
| Componente | Chip | Protocolo | Pines |
|-----------|------|-----------|-------|
| AFE4490 SpO2 | `incu-afe4490-spo2` | SPI (FSPI) | CS=21, MOSI=35, MISO=37, SCK=36, ADC_RDY=45 |

### Comunicaciones (1)
| Componente | Chip | Protocolo | Pines |
|-----------|------|-----------|-------|
| SIM800C GPRS | `incu-sim800c-gprs` | UART2 115200 | ESP TX=47 → RX, ESP RX=48 ← TX |

### Display + Telemetría (2)
| Componente | Chip | Conexión |
|-----------|------|----------|
| Display CrowPanel | `incu-display-hmi` | RX ← telemetry:TX (visual) |
| Telemetry Reporter | `incu-telemetry-reporter` | TX → display (stream CTRL simulado) |

### Indicadores (1)
| Componente | Tipo | Pin |
|-----------|------|-----|
| WS2812B LED RGB | `wokwi-neopixel` (nativo) | GPIO 7 |

## Pinout v15 (board.h HW_NUM > 14)

| Señal | GPIO | Fuente | Notas |
|-------|:----:|--------|-------|
| I2C SDA | 8 | board.h | Bus: STS35×2, SHT4x, SHTC3, INA3221×2, Humidifier (0x02) |
| I2C SCL | 9 | board.h | |
| Baby NTC | 10 | board.h | ADC, sonda piel |
| Fan PWM | 12 | board.h | PWM 32Hz (LOW_PWM_FREQUENCY) |
| Phototherapy | 13 | board.h | PWM 2000Hz |
| Actuators EN | 14 | board.h | Enable global actuadores (fan + phototherapy) |
| Heater SSR | 16 | board.h | PWM 400Hz |
| AFE4490 CS | 21 | board.h | SPI chip select |
| AFE4490 MOSI | 35 | ESP32-S3 FSPI default | |
| AFE4490 SCK | 36 | ESP32-S3 FSPI default | |
| AFE4490 MISO | 37 | ESP32-S3 FSPI default | |
| Fan TACH | 38 | board.h | Tacómetro 2 pulsos/rev |
| AFE4490 ADC_RDY | 45 | board.h | Interrupt |
| SERIAL2 TX (SIM800C) | 47 | board.h | ESP32 TX → SIM800C RX |
| SERIAL2 RX (SIM800C) | 48 | board.h | ESP32 RX ← SIM800C TX |
| Buzzer | 5 | board.h | PWM tono |
| Touch Sensor | 1 | board.h | Sensor capacitivo puerta |
| Touch SEL | 2 | board.h | Selección modo touch |
| LED (WS2812B) | 7 | board.h | NeoPixel status |

### Display: Nota sobre comunicación

En el hardware real v15, el CrowPanel se comunica por **USB Host** (CDC ACM vía chip CH340C), gestionado por `CommTask.cpp`. Wokwi no soporta USB Host, por lo que en la simulación el display recibe datos del chip `incu-telemetry-reporter` como aproximación visual.

## Quick Start (mode2)

`mode2` en este repo es el flujo v15 más fiel para HMI interactivo en Wokwi:
- escenario `examples/full-incubator-demo-v15/`
- chip `incu-display-hmi` con navegación táctil virtual en footer (PREV/OK/NEXT)
- controles Wokwi `touchX`, `touchY`, `touchTap` (sin switches físicos)
- firmware real v15 cargado desde `../../Incunest_v15/Firmware/MotherBoard/firmware.bin`

### Ejecución end-to-end (copy/paste)

```bash
cd /path/to/IncuNest-WorkWi-Chips
./tools/build-chips.sh -B
cd examples/full-incubator-demo-v15
test -f wokwi.toml && test -f diagram.json && test -f ../../Incunest_v15/Firmware/MotherBoard/firmware.bin
code .
# En VS Code: F1 → "Wokwi: Start Simulator"
```

### Controles táctiles virtuales (`touchX`/`touchY`/`touchTap`)

Wokwi no expone eventos nativos de click/touch sobre el framebuffer de un custom chip, así que el toque se emula con controles.

1. Ajusta `touchX` (0..479) y `touchY` (0..319) en los sliders del chip.
2. Dispara el tap cambiando `touchTap` de **0→1**.
3. Regresa `touchTap` a **0** para poder generar el siguiente tap.

Para navegación, usa el footer (`touchY >= 296`) y estas zonas en X:

| Zona footer | `touchX` | Acción |
|-------------|----------|--------|
| PREV | 0..159 | Pantalla anterior (cíclico) |
| OK | 160..319 | BOOT→MAIN, LOCK↔UNLOCK, en ALARMS con alarmas activas alterna mute |
| NEXT | 320..479 | Pantalla siguiente (cíclico) |

> Cuando la UI está bloqueada, PREV/NEXT quedan deshabilitados hasta desbloquear con OK.

### Pantallas disponibles (mode2)

Orden de navegación: `BOOT → MAIN → SETTINGS → ALARMS → CHARTS → PULSEOXI → LOCK`.

| Pantalla | Qué muestra |
|----------|-------------|
| BOOT | Estado de arranque y conexión (`Connecting...`, detección de stream) |
| MAIN | Temperatura aire/piel, humedad, actuadores (heater/fan/photo/door), resumen de alarmas |
| SETTINGS | Snapshot de configuración: modo AIR/SKIN, setpoints, idioma, timeout, estado de enlace |
| ALARMS | Centro de alarmas (`CTRL,ALM`) con severidad/estado; desde aquí OK alterna mute si hay alarmas |
| CHARTS | Barras en vivo (air/skin/humidity) y placeholder de histórico |
| PULSEOXI | Placeholders de SpO₂/pulso + referencias disponibles (skin temp, puerta, alarm code) |
| LOCK | Estado de bloqueo/desbloqueo de navegación |

### Limitaciones conocidas vs CrowPanel real (v15)

- **USB Host no simulado:** el hardware real usa USB Host CDC ACM (CH340C, `CommTask.cpp`), no disponible en Wokwi.
- **Canal HMI simplificado:** en `mode2` el display consume stream simulado de `incu-telemetry-reporter`; no hay retorno cableado del display hacia la motherboard.
- **UI aproximada:** se usa framebuffer 480×320 y toque emulado con `touchX/touchY/touchTap`; no hay touch nativo ni panel RGB táctil 800×480 + GT911/LVGL completo.
- **Datos parciales:** `CHARTS` no incluye histórico real y `PULSEOXI` queda en placeholder hasta publicar esos campos en el stream.
