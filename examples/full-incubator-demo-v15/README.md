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
| Telemetry Reporter | `incu-telemetry-reporter` | TX → ESP32 UART0 + display |

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

## Quick Start

```bash
# 1. Compilar chips
docker run --rm -v "$PWD/chips:/src" wokwi/builder-clang-wasm:latest make -B

# 2. Abrir en VS Code → Wokwi: Start Simulator
```
