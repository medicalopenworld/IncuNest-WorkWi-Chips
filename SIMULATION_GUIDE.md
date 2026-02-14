# 🏥 IncuNest Wokwi Simulator — Guía Completa de Simulación

> **Documento consolidado** que recopila toda la información necesaria para crear un simulador 100% virtual de la incubadora neonatal IncuNest (IN3ator) en Wokwi, integrando custom chips, VS Code, y un visor 3D en Next.js.
>
> Fuentes consultadas: Wokwi Chips API docs, código fuente IncuNest (medicalopenworld/IncuNest), Elecrow CrowPanel 7.0" HMI ESP32, documentación Wokwi VS Code Extension.

---

## 📑 Tabla de Contenidos

1. [Visión General del Proyecto](#1-visión-general-del-proyecto)
2. [Análisis del Hardware Real (IncuNest v15)](#2-análisis-del-hardware-real-incunest-v15)
3. [Arquitectura del Mono-Repo](#3-arquitectura-del-mono-repo)
4. [Catálogo de Custom Chips](#4-catálogo-de-custom-chips)
5. [Wokwi Chips API — Referencia Rápida](#5-wokwi-chips-api--referencia-rápida)
6. [Configuración con VS Code](#6-configuración-con-vs-code)
7. [Diagrama de Simulación Completo](#7-diagrama-de-simulación-completo)
8. [Aplicación Next.js — Visor 3D](#8-aplicación-nextjs--visor-3d)
9. [Pasos de Implementación](#9-pasos-de-implementación)
10. [Notas de los Modelos de IA](#10-notas-de-los-modelos-de-ia)

---

## 1. Visión General del Proyecto

### ¿Qué es IncuNest?

**IncuNest** (anteriormente IN3ator) es una incubadora neonatal open-source desarrollada por la ONG Medical Open World. Datos clave:

- **Coste:** ~€350 en componentes
- **Despliegue:** 200+ unidades en 30+ países
- **MCU principal:** ESP32 (FireBeetle32) con Arduino Framework + PlatformIO
- **Display HMI:** Elecrow CrowPanel 7.0" 800×480 con ESP32-S3 + LVGL
- **Comunicación:** WiFi, GPRS (SIM800), MQTT a ThingsBoard
- **Certificación:** IEC 60601-2-19

### ¿Qué vamos a simular?

Un entorno Wokwi 100% virtual que reproduce:
- El **ESP32 motherboard** ejecutando el firmware real (binario .bin)
- Todos los **sensores y actuadores** como custom chips de Wokwi
- El **display CrowPanel** (ESP32-S3) comunicándose por UART/Serial
- Un **visor 3D** en Next.js que renderiza el modelo STEP del ensamblaje

---

## 2. Análisis del Hardware Real (IncuNest v15)

### 2.1 Microcontroladores

| MCU | Placa | Función | Framework |
|-----|-------|---------|-----------|
| ESP32 (FireBeetle32) | MotherBoard | Control principal, PID, sensores, comunicación | Arduino/PlatformIO |
| ESP32-S3-WROOM-1-N4R8 | CrowPanel 7.0" HMI | Display LVGL 800×480, interfaz táctil | ESP-IDF/PlatformIO |
| ATmega/ESP (slave) | Placa humidificador | Control humidificador I2C | Arduino |

### 2.2 Pinout del MotherBoard (HW_NUM = 14)

```
Pin ESP32   │ Función                  │ Tipo
────────────┼──────────────────────────┼─────────
GPIO 0      │ TFT_DC (display)         │ Output
GPIO 2      │ AFE44XX_CS (SpO2)        │ SPI CS
GPIO 4      │ ENC_SWITCH               │ Input
GPIO 5      │ BUZZER                   │ PWM Output
GPIO 12     │ FAN                      │ PWM Output
GPIO 13     │ PHOTOTHERAPY             │ PWM Output
GPIO 14     │ ACTUATORS_EN             │ Output
GPIO 15     │ TFT_CS                   │ SPI CS
GPIO 16     │ SERIAL2_RX (GPRS)        │ UART RX
GPIO 17     │ SERIAL2_TX (GPRS)        │ UART TX
GPIO 21     │ I2C_SDA                  │ I2C
GPIO 22     │ I2C_SCL                  │ I2C
GPIO 25     │ ENC_A                    │ Input
GPIO 26     │ TOUCH_SENSOR_SEL         │ Output
GPIO 27     │ HEATER                   │ PWM Output
GPIO 32     │ TOUCH_SENSOR             │ Analog Input
GPIO 33     │ SCREENBACKLIGHT          │ PWM Output
GPIO 34     │ ENC_B                    │ Input
GPIO 35     │ FAN_SPEED_FEEDBACK       │ Input (tach)
GPIO 39     │ BABY_NTC_PIN             │ Analog Input
```

### 2.3 Buses I2C (dirección → dispositivo)

| Dirección | Chip | Función |
|-----------|------|---------|
| 0x40 | INA3221 (secundario) | Sensor corriente heater/USB/battery |
| 0x41 | INA3221 (principal) | Sensor corriente system/phototherapy/fan |
| 0x44 | STS3x / SHT4x | Sensor temp/hum ambiente |
| 0x4A | STS35 (main) | Sensor temp digital cámara (principal) |
| 0x4B | STS35 (redundant) | Sensor temp digital cámara (redundante) |
| 0x70 | SHTC3 | Sensor temp/hum cámara (legacy) |
| 0x20 | TCA9555 | GPIO Expander (HW ≤ 8) |
| 0x6B | BQ25792 | Cargador batería |
| 0x58 | AFE4490 | Pulsioxímetro SpO2 |
| — | Humidifier slave | Control humidificador I2C |

### 2.4 Sensores del Sistema

| Categoría | Sensor | Interfaz | Función |
|-----------|--------|----------|---------|
| Temp. piel | NTC 10K (termistor) | ADC (GPIO39) | Temperatura cutánea neonato |
| Temp. cámara | STS35 (×2) | I2C | Temperatura aire (principal + redundante) |
| Temp/Hum ambiente | SHT4x / SHTC3 | I2C | Temperatura y humedad ambiental |
| Corriente | INA3221 (×2) | I2C | 6 canales: system, heater, fan, photo, USB, battery |
| SpO2 | AFE4490 | SPI | Pulsioximetría |
| Batería | BQ25792 | I2C | Gestión carga/descarga |
| Velocidad fan | Tachómetro | Pulsos (GPIO35) | RPM ventilador |
| Puerta | Touch sensor | Capacitivo (GPIO32) | Detección apertura puerta |

### 2.5 Actuadores

| Actuador | Pin | Control | Descripción |
|----------|-----|---------|-------------|
| Heater (SSR) | GPIO27 | PWM (ch2, 2kHz) | Resistencia calefactora con SSR |
| Fan | GPIO12 | PWM (ch3, 2kHz) | Ventilador recirculación aire |
| Phototherapy | GPIO13 | PWM (ch4, 2kHz) | LEDs fototerapia ictericia |
| Buzzer | GPIO5 | PWM (ch1) | Alarma sonora |
| Backlight | GPIO33 | PWM (ch0) | Retroiluminación display |
| Actuators Enable | GPIO14 | Digital | Habilitación general actuadores |
| Humidifier | I2C slave | I2C | Humidificador ultrasónico |

### 2.6 Sistema de Alarmas

```
ALARMS_ID:
  HUMIDITY_ALARM           → Humedad fuera de rango
  TEMPERATURE_ALARM        → Temperatura fuera de rango
  AIR_THERMAL_CUTOUT       → Corte térmico aire (>38°C)
  SKIN_THERMAL_CUTOUT      → Corte térmico piel (>38°C)
  AIR_SENSOR_ISSUE         → Sensor aire defectuoso
  SKIN_SENSOR_ISSUE        → Sensor piel defectuoso
  FAN_ISSUE                → Ventilador sin respuesta
  HEATER_ISSUE             → Calefactor sin consumo
  POWER_SUPPLY             → Fallo alimentación
```

### 2.7 Control PID

- El firmware usa la librería `Arduino-PID-Library` (br3ttb)
- Control PID para temperatura (aire o piel, configurable)
- Control PID para humedad
- Parámetros almacenados en EEPROM
- Modo de control: `AIR_CONTROL` o `SKIN_CONTROL`

### 2.8 Display CrowPanel 7.0" (V3.0)

- **MCU:** ESP32-S3-WROOM-1-N4R8 (4MB Flash, 8MB PSRAM)
- **Resolución:** 800×480 TFT-LCD (panel TN)
- **Driver display:** EK9716BD3 & EK73002ACGB (RGB paralelo)
- **Touch:** Capacitivo
- **GUI Framework:** LVGL 8.3.3 (SquareLine Studio)
- **Comunicación con MotherBoard:** UART Serial
- **IO Expander:** PCA9557 (V3.0 timing control)

---

## 3. Arquitectura del Mono-Repo

```
incunest-wokwi-chips/
├── Incunest_v15/                     # (existente) Firmware/Hardware/Mechanical originales
│   ├── Firmware/
│   │   ├── MotherBoard/              # .bin precompilados
│   │   └── Humidifier/               # .bin esclavo
│   ├── Hardware/
│   │   ├── BOM/                      # Bill of Materials
│   │   ├── Gerber.zip
│   │   └── Pick Place/
│   └── Mechanical/
│       ├── IN3_structure_v15.step     # Modelo 3D completo
│       └── [IN3] BOM Assembly.xlsx
│
├── chips/                            # Custom chips para Wokwi
│   ├── incu-ntc-skin/                # Sonda NTC temperatura piel
│   │   ├── incu-ntc-skin.chip.json
│   │   ├── incu-ntc-skin.c
│   │   └── README.md
│   ├── incu-sts35-temp/              # Sensor digital STS35 (I2C)
│   │   ├── incu-sts35-temp.chip.json
│   │   ├── incu-sts35-temp.c
│   │   └── README.md
│   ├── incu-sht4x-env/              # Sensor SHT4x temp+hum (I2C)
│   │   ├── incu-sht4x-env.chip.json
│   │   ├── incu-sht4x-env.c
│   │   └── README.md
│   ├── incu-heater-ssr/              # SSR + modelo térmico
│   │   ├── incu-heater-ssr.chip.json
│   │   ├── incu-heater-ssr.c
│   │   └── README.md
│   ├── incu-fan-pwm/                 # Ventilador PWM + tachómetro
│   │   ├── incu-fan-pwm.chip.json
│   │   ├── incu-fan-pwm.c
│   │   └── README.md
│   ├── incu-humidifier/              # Humidificador I2C slave
│   │   ├── incu-humidifier.chip.json
│   │   ├── incu-humidifier.c
│   │   └── README.md
│   ├── incu-ina3221/                 # Sensor corriente 3ch (I2C)
│   │   ├── incu-ina3221.chip.json
│   │   ├── incu-ina3221.c
│   │   └── README.md
│   ├── incu-buzzer-alarm/            # Buzzer piezoeléctrico
│   │   ├── incu-buzzer-alarm.chip.json
│   │   ├── incu-buzzer-alarm.c
│   │   └── README.md
│   └── incu-door-touch/              # Sensor puerta capacitivo
│       ├── incu-door-touch.chip.json
│       ├── incu-door-touch.c
│       └── README.md
│
├── shared/                           # Código compartido (modelos térmicos)
│   ├── thermal-model.h               # Modelo térmico cámara incubadora
│   ├── ntc-tables.h                  # Tablas NTC Steinhart-Hart
│   ├── pid-reference.h               # Constantes PID de referencia
│   └── utils.h                       # Utilidades generales
│
├── examples/
│   ├── full-incubator-demo/          # Demo completa con todos los chips
│   │   ├── diagram.json
│   │   ├── wokwi.toml
│   │   └── README.md
│   ├── heater-pid-only/              # Solo heater + NTC + PID
│   │   ├── diagram.json
│   │   ├── wokwi.toml
│   │   └── README.md
│   └── sensor-suite/                 # Solo sensores I2C
│       ├── diagram.json
│       ├── wokwi.toml
│       └── README.md
│
├── viewer-3d/                        # Aplicación Next.js visor 3D
│   ├── package.json
│   ├── next.config.js
│   ├── public/
│   │   └── models/                   # Fichero STEP convertido
│   ├── src/
│   │   ├── app/
│   │   └── components/
│   │       └── IncubatorViewer.tsx    # Componente Three.js/React-Three-Fiber
│   └── README.md
│
├── package.json                      # Mono-repo config
├── LICENSE
├── README.md
└── SIMULATION_GUIDE.md               # ← ESTE DOCUMENTO
```

---

## 4. Catálogo de Custom Chips

### 4.1 `incu-ntc-skin` — Sonda NTC Temperatura Piel

**Propósito:** Simula el termistor NTC 10K conectado al ADC del ESP32 para medir la temperatura cutánea del neonato.

**Interfaz:** Analógica (pin_dac_write)

**Pines:**
| Pin | Dirección | Descripción |
|-----|-----------|-------------|
| VCC | Power | Alimentación |
| GND | Power | Tierra |
| OUT | Analog Out | Voltaje proporcional a temperatura (divisor resistivo NTC) |

**Controles interactivos:**
- `temperature`: Slider -10°C a 50°C (step 0.1)

**Lógica:**
- Convierte temperatura → resistencia NTC usando ecuación Steinhart-Hart
- Calcula voltaje de salida del divisor resistivo: `Vout = Vcc * R_fixed / (R_fixed + R_ntc)`
- Actualiza `pin_dac_write()` periódicamente (cada 100ms timer)

**Referencia firmware:** `BABY_NTC_PIN` (GPIO39), `ADC_READ_FUNCTION = MILLIVOTSREAD_ADC`

---

### 4.2 `incu-sts35-temp` — Sensor Digital STS35 (I2C)

**Propósito:** Simula el sensor de temperatura digital Sensirion STS35-DIS, usado para temperatura del aire de la cámara.

**Interfaz:** I2C (dirección configurable: 0x4A main / 0x4B redundante)

**Pines:**
| Pin | Dirección | Descripción |
|-----|-----------|-------------|
| VCC | Power | Alimentación |
| GND | Power | Tierra |
| SCL | I2C Clock | Reloj I2C |
| SDA | I2C Data | Datos I2C |

**Controles interactivos:**
- `temperature`: Slider -40°C a 125°C (step 0.1)

**Protocolo I2C:**
- Comando medición single-shot: `0x2400` (high repeatability)
- Respuesta: 2 bytes temp + 1 byte CRC
- Fórmula: `raw = (temp + 45) * 65535 / 175`

**Referencia firmware:** `ROOM_SENSOR_STS35_I2C_ADDRESS_MAIN = 0x4A`, librería `Sensirion/arduino-i2c-sts3x`

---

### 4.3 `incu-sht4x-env` — Sensor Ambiental SHT4x (I2C)

**Propósito:** Simula SHT4x (temperatura + humedad) para condiciones ambientales exteriores.

**Interfaz:** I2C (dirección 0x44)

**Pines:**
| Pin | Dirección | Descripción |
|-----|-----------|-------------|
| VCC | Power | Alimentación |
| GND | Power | Tierra |
| SCL | I2C Clock | Reloj I2C |
| SDA | I2C Data | Datos I2C |

**Controles interactivos:**
- `temperature`: Slider -40°C a 125°C (step 0.1)
- `humidity`: Slider 0% a 100% (step 1)

**Protocolo I2C:**
- Comando medición: `0xFD` (high precision)
- Respuesta: 2 bytes temp + CRC + 2 bytes hum + CRC
- Temp raw: `(temp + 45) * 65535 / 175`
- Hum raw: `(hum + 6) * 65535 / 125`

**Referencia firmware:** `AMBIENT_SENSOR_I2C_ADDRESS = 0x44`, librería `Adafruit_SHT4X`

---

### 4.4 `incu-heater-ssr` — Calefactor con SSR

**Propósito:** Simula el SSR (Solid State Relay) que controla la resistencia calefactora, incluyendo un modelo térmico simplificado.

**Interfaz:** GPIO digital (PWM input) + modelo térmico

**Pines:**
| Pin | Dirección | Descripción |
|-----|-----------|-------------|
| VCC | Power | Alimentación |
| GND | Power | Tierra |
| PWM_IN | Digital In | Señal PWM del ESP32 (GPIO27) |
| TEMP_FEEDBACK | Analog Out | Retroalimentación temperatura simulada |

**Controles interactivos:**
- `ambientTemp`: Slider 15°C a 40°C (step 0.5) — temperatura ambiente exterior
- `thermalMass`: Slider 0.1 a 10.0 (step 0.1) — inercia térmica cámara

**Lógica:**
- Lee duty cycle del PWM de entrada
- Modelo térmico 1er orden: `dT/dt = (P_heater - k*(T - T_ambient)) / C_thermal`
- Potencia máxima: `HEATER_MAX_POWER_AMPS × 12V ≈ 150W`
- Actualiza temperatura interna cada ciclo de timer (10ms)

**Referencia firmware:** `HEATER` (GPIO27), `HEATER_PWM_CHANNEL = 2`, `HEATER_MAX_POWER_AMPS = 12.5`

---

### 4.5 `incu-fan-pwm` — Ventilador PWM con Tacómetro

**Propósito:** Simula el ventilador de recirculación de aire con entrada PWM y salida de tacómetro.

**Interfaz:** PWM input + pulse output (tacómetro)

**Pines:**
| Pin | Dirección | Descripción |
|-----|-----------|-------------|
| VCC | Power | Alimentación |
| GND | Power | Tierra |
| PWM_IN | Digital In | Señal PWM velocidad (GPIO12) |
| TACH_OUT | Digital Out | Pulsos tacómetro (GPIO35) |
| EN | Digital In | Enable (ACTUATORS_EN, GPIO14) |

**Lógica:**
- Lee PWM → convierte a RPM proporcional
- Genera pulsos en TACH_OUT: 2 pulsos por revolución
- Frecuencia pulsos = RPM / 30 Hz
- RPM máximo: ~3000 (configurable vía atributo)
- `FAN_RPM_CONVERSION = 13333333` (del firmware)

**Referencia firmware:** `FAN` (GPIO12), `FAN_PWM_CHANNEL = 3`, `FAN_SPEED_FEEDBACK` (GPIO35)

---

### 4.6 `incu-humidifier` — Humidificador Ultrasónico I2C

**Propósito:** Simula la placa esclava del humidificador ultrasónico que se comunica por I2C.

**Interfaz:** I2C slave

**Pines:**
| Pin | Dirección | Descripción |
|-----|-----------|-------------|
| VCC | Power | Alimentación |
| GND | Power | Tierra |
| SCL | I2C Clock | Reloj I2C |
| SDA | I2C Data | Datos I2C |

**Controles interactivos:**
- `waterLevel`: Slider 0% a 100% (step 1) — nivel de agua del depósito

**Lógica:**
- Recibe comandos del master: duty cycle humidificación (0-95%)
- Reporta estado: nivel agua, corriente consumida
- `HUMIDIFIER_DUTY_CYCLE_MAX = 95`, `HUMIDIFIER_DUTY_CYCLE_MIN = 0`
- Interface mode: `HUMIDIFIER_I2C` (HW ≥ 9)

**Referencia firmware:** `in3ator_humidifier.h/cpp`, `HUMIDIFIER_INTERFACE = HUMIDIFIER_I2C`

---

### 4.7 `incu-ina3221` — Sensor Corriente INA3221 (I2C)

**Propósito:** Simula el INA3221 (3 canales de medición de corriente/voltaje) para monitorización de potencia.

**Interfaz:** I2C (dirección 0x41 principal / 0x40 secundario)

**Pines:**
| Pin | Dirección | Descripción |
|-----|-----------|-------------|
| VCC | Power | Alimentación |
| GND | Power | Tierra |
| SCL | I2C Clock | Reloj I2C |
| SDA | I2C Data | Datos I2C |

**Controles interactivos:**
- `ch1Current`: Slider 0 a 15A (step 0.1)
- `ch2Current`: Slider 0 a 5A (step 0.1)
- `ch3Current`: Slider 0 a 5A (step 0.1)
- `busVoltage`: Slider 0 a 30V (step 0.1)

**Canales (principal 0x41):**
- CH1: System (shunt 2mΩ)
- CH2: Phototherapy (shunt 82mΩ)
- CH3: Fan (shunt 100mΩ)

**Canales (secundario 0x40):**
- CH1: Heater (shunt 2mΩ)
- CH2: USB (shunt 100mΩ)
- CH3: Battery (shunt 27Ω)

**Referencia firmware:** librería `Beast-devices/Arduino-INA3221`

---

### 4.8 `incu-buzzer-alarm` — Buzzer Piezoeléctrico

**Propósito:** Simula el buzzer para alarmas sonoras. En Wokwi es visual (indicador de estado).

**Interfaz:** PWM input

**Pines:**
| Pin | Dirección | Descripción |
|-----|-----------|-------------|
| VCC | Power | Alimentación |
| GND | Power | Tierra |
| IN | Digital In | Señal PWM buzzer (GPIO5) |

**Lógica:**
- Detecta señal PWM en pin IN
- Muestra indicador visual (framebuffer simple) cuando está activo
- Frecuencias configuradas: `buzzerStandbyTone = 500Hz`, `buzzerAlarmTone = 500Hz`, `buzzerRotaryEncoderTone = 2200Hz`

**Referencia firmware:** `BUZZER` (GPIO5), `BUZZER_PWM_CHANNEL = 1`

---

### 4.9 `incu-door-touch` — Sensor Puerta Capacitivo

**Propósito:** Simula el sensor capacitivo de detección de apertura de puerta.

**Interfaz:** Analog output (capacitancia simulada)

**Pines:**
| Pin | Dirección | Descripción |
|-----|-----------|-------------|
| VCC | Power | Alimentación |
| GND | Power | Tierra |
| OUT | Analog Out | Señal capacitiva (GPIO32) |
| SEL | Digital In | Selector sensor (GPIO26) |

**Controles interactivos:**
- `doorOpen`: Slider 0 (cerrada) a 1 (abierta)

**Lógica:**
- Cuando `doorOpen = 0`: salida capacitancia alta (puerta cerrada, neonato detectado)
- Cuando `doorOpen = 1`: salida capacitancia baja (puerta abierta)
- Debounce interno: 30ms (`SWITCH_DEBOUNCE_TIME_MS`)

**Referencia firmware:** `TOUCH_SENSOR` (GPIO32), `TOUCH_SENSOR_SEL` (GPIO26)

---

## 5. Wokwi Chips API — Referencia Rápida

### 5.1 Estructura de un Custom Chip

Cada chip necesita dos archivos:

**`chip-name.chip.json`** — Definición de pines y controles:
```json
{
  "name": "Incu NTC Skin Probe",
  "author": "IncuNest Project",
  "pins": ["VCC", "GND", "OUT"],
  "controls": [
    {
      "id": "temperature",
      "label": "Skin Temperature (°C)",
      "type": "range",
      "min": 20,
      "max": 42,
      "step": 0.1
    }
  ]
}
```

**`chip-name.c`** — Implementación en C:
```c
#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
  pin_t out_pin;
  uint32_t temp_attr;
  timer_t timer;
} chip_state_t;

static void chip_timer_callback(void *user_data) {
  chip_state_t *chip = (chip_state_t*)user_data;
  float temp = attr_read_float(chip->temp_attr);
  // Steinhart-Hart → voltage conversion
  float voltage = temp_to_voltage(temp);
  pin_dac_write(chip->out_pin, voltage);
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->out_pin = pin_init("OUT", ANALOG);
  chip->temp_attr = attr_init_float("temperature", 36.5);

  const timer_config_t timer_cfg = {
    .callback = chip_timer_callback,
    .user_data = chip,
  };
  chip->timer = timer_init(&timer_cfg);
  timer_start(chip->timer, 100000, true); // 100ms
}
```

### 5.2 APIs Disponibles

| API | Funciones Clave | Uso en IncuNest |
|-----|----------------|-----------------|
| **GPIO** | `pin_init()`, `pin_read()`, `pin_write()`, `pin_watch()` | Fan PWM, heater, buzzer, enable |
| **Analog** | `pin_adc_read()`, `pin_dac_write()` | NTC probe, door sensor |
| **I2C** | `i2c_init()`, callbacks `connect/read/write/disconnect` | STS35, SHT4x, INA3221, humidifier |
| **SPI** | `spi_init()`, `spi_start()`, `spi_stop()` | AFE4490 (SpO2) — futuro |
| **Timer** | `timer_init()`, `timer_start()`, `timer_stop()` | Modelo térmico, tachómetro |
| **Attributes** | `attr_init()`, `attr_init_float()`, `attr_read_float()` | Controles interactivos |
| **Framebuffer** | `framebuffer_init()`, `buffer_write()` | Indicador buzzer visual |

### 5.3 Patrón I2C Típico (ejemplo STS35)

```c
#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  uint32_t temp_attr;
  uint8_t cmd_buf[2];
  uint8_t cmd_idx;
  uint8_t resp_buf[3];
  uint8_t resp_idx;
  bool reading;
} chip_state_t;

static bool on_i2c_connect(void *data, uint32_t address, bool read) {
  chip_state_t *chip = (chip_state_t*)data;
  chip->reading = read;
  chip->cmd_idx = 0;
  chip->resp_idx = 0;
  if (read) {
    // Prepare temperature response
    float temp = attr_read_float(chip->temp_attr);
    uint16_t raw = (uint16_t)((temp + 45.0f) * 65535.0f / 175.0f);
    chip->resp_buf[0] = raw >> 8;
    chip->resp_buf[1] = raw & 0xFF;
    chip->resp_buf[2] = crc8(chip->resp_buf, 2);
  }
  return true; // ACK
}

static uint8_t on_i2c_read(void *data) {
  chip_state_t *chip = (chip_state_t*)data;
  if (chip->resp_idx < 3) return chip->resp_buf[chip->resp_idx++];
  return 0xFF;
}

static bool on_i2c_write(void *data, uint8_t byte) {
  chip_state_t *chip = (chip_state_t*)data;
  if (chip->cmd_idx < 2) chip->cmd_buf[chip->cmd_idx++] = byte;
  return true; // ACK
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->temp_attr = attr_init_float("temperature", 25.0);

  const i2c_config_t i2c_cfg = {
    .address = 0x4A,
    .scl = pin_init("SCL", INPUT),
    .sda = pin_init("SDA", INPUT),
    .connect = on_i2c_connect,
    .read = on_i2c_read,
    .write = on_i2c_write,
    .user_data = chip,
  };
  i2c_init(&i2c_cfg);
}
```

---

## 6. Configuración con VS Code

### 6.1 Requisitos Previos

1. **VS Code** instalado
2. **Extensión Wokwi for VS Code** (`wokwi.wokwi-vscode`)
   - Instalar desde: https://marketplace.visualstudio.com/items?itemName=wokwi.wokwi-vscode
   - Activar licencia: `F1` → "Wokwi: Request a new License"
3. **PlatformIO** (opcional, para compilar firmware desde fuente)

### 6.2 Fichero `wokwi.toml`

```toml
[wokwi]
version = 1
# Usar firmware precompilado del ESP32 MotherBoard
firmware = "Incunest_v15/Firmware/MotherBoard/firmware.bin"
elf = ""

# Custom chips
[[chip]]
name = "incu-ntc-skin"
binary = "chips/incu-ntc-skin/incu-ntc-skin.chip.wasm"

[[chip]]
name = "incu-sts35-temp"
binary = "chips/incu-sts35-temp/incu-sts35-temp.chip.wasm"

[[chip]]
name = "incu-sht4x-env"
binary = "chips/incu-sht4x-env/incu-sht4x-env.chip.wasm"

[[chip]]
name = "incu-heater-ssr"
binary = "chips/incu-heater-ssr/incu-heater-ssr.chip.wasm"

[[chip]]
name = "incu-fan-pwm"
binary = "chips/incu-fan-pwm/incu-fan-pwm.chip.wasm"

[[chip]]
name = "incu-humidifier"
binary = "chips/incu-humidifier/incu-humidifier.chip.wasm"

[[chip]]
name = "incu-ina3221"
binary = "chips/incu-ina3221/incu-ina3221.chip.wasm"

[[chip]]
name = "incu-buzzer-alarm"
binary = "chips/incu-buzzer-alarm/incu-buzzer-alarm.chip.wasm"

[[chip]]
name = "incu-door-touch"
binary = "chips/incu-door-touch/incu-door-touch.chip.wasm"

# Serial port forwarding (para debug)
rfc2217ServerPort = 4000
```

### 6.3 Fichero `diagram.json`

```json
{
  "version": 1,
  "author": "IncuNest Project",
  "editor": "wokwi",
  "parts": [
    {
      "type": "board-esp32-devkit-c-v4",
      "id": "esp32",
      "top": 0,
      "left": 0,
      "attrs": {}
    },
    {
      "type": "chip-incu-ntc-skin",
      "id": "ntc1",
      "top": -150,
      "left": 300,
      "attrs": { "temperature": "36.5" }
    },
    {
      "type": "chip-incu-sts35-temp",
      "id": "sts35_main",
      "top": -100,
      "left": 300,
      "attrs": { "temperature": "34.0" }
    },
    {
      "type": "chip-incu-sts35-temp",
      "id": "sts35_redundant",
      "top": -50,
      "left": 300,
      "attrs": { "temperature": "34.0", "address": "75" }
    },
    {
      "type": "chip-incu-sht4x-env",
      "id": "sht4x",
      "top": 0,
      "left": 300,
      "attrs": { "temperature": "25.0", "humidity": "50" }
    },
    {
      "type": "chip-incu-heater-ssr",
      "id": "heater",
      "top": 50,
      "left": 300,
      "attrs": { "ambientTemp": "25.0", "thermalMass": "2.0" }
    },
    {
      "type": "chip-incu-fan-pwm",
      "id": "fan",
      "top": 100,
      "left": 300,
      "attrs": {}
    },
    {
      "type": "chip-incu-humidifier",
      "id": "humidifier",
      "top": 150,
      "left": 300,
      "attrs": { "waterLevel": "80" }
    },
    {
      "type": "chip-incu-ina3221",
      "id": "ina_main",
      "top": 200,
      "left": 300,
      "attrs": { "address": "65" }
    },
    {
      "type": "chip-incu-ina3221",
      "id": "ina_secondary",
      "top": 250,
      "left": 300,
      "attrs": { "address": "64" }
    },
    {
      "type": "chip-incu-buzzer-alarm",
      "id": "buzzer",
      "top": -150,
      "left": -200,
      "attrs": {}
    },
    {
      "type": "chip-incu-door-touch",
      "id": "door",
      "top": -100,
      "left": -200,
      "attrs": { "doorOpen": "0" }
    }
  ],
  "connections": [
    ["esp32:GND.1", "ntc1:GND", "black", []],
    ["esp32:3V3", "ntc1:VCC", "red", []],
    ["esp32:39", "ntc1:OUT", "green", []],

    ["esp32:GND.1", "sts35_main:GND", "black", []],
    ["esp32:3V3", "sts35_main:VCC", "red", []],
    ["esp32:21", "sts35_main:SDA", "blue", []],
    ["esp32:22", "sts35_main:SCL", "purple", []],

    ["esp32:GND.1", "sts35_redundant:GND", "black", []],
    ["esp32:3V3", "sts35_redundant:VCC", "red", []],
    ["esp32:21", "sts35_redundant:SDA", "blue", []],
    ["esp32:22", "sts35_redundant:SCL", "purple", []],

    ["esp32:GND.1", "sht4x:GND", "black", []],
    ["esp32:3V3", "sht4x:VCC", "red", []],
    ["esp32:21", "sht4x:SDA", "blue", []],
    ["esp32:22", "sht4x:SCL", "purple", []],

    ["esp32:27", "heater:PWM_IN", "orange", []],
    ["esp32:GND.1", "heater:GND", "black", []],
    ["esp32:3V3", "heater:VCC", "red", []],

    ["esp32:12", "fan:PWM_IN", "orange", []],
    ["esp32:35", "fan:TACH_OUT", "yellow", []],
    ["esp32:14", "fan:EN", "white", []],
    ["esp32:GND.1", "fan:GND", "black", []],
    ["esp32:3V3", "fan:VCC", "red", []],

    ["esp32:GND.1", "humidifier:GND", "black", []],
    ["esp32:3V3", "humidifier:VCC", "red", []],
    ["esp32:21", "humidifier:SDA", "blue", []],
    ["esp32:22", "humidifier:SCL", "purple", []],

    ["esp32:GND.1", "ina_main:GND", "black", []],
    ["esp32:3V3", "ina_main:VCC", "red", []],
    ["esp32:21", "ina_main:SDA", "blue", []],
    ["esp32:22", "ina_main:SCL", "purple", []],

    ["esp32:GND.1", "ina_secondary:GND", "black", []],
    ["esp32:3V3", "ina_secondary:VCC", "red", []],
    ["esp32:21", "ina_secondary:SDA", "blue", []],
    ["esp32:22", "ina_secondary:SCL", "purple", []],

    ["esp32:5", "buzzer:IN", "orange", []],
    ["esp32:GND.1", "buzzer:GND", "black", []],
    ["esp32:3V3", "buzzer:VCC", "red", []],

    ["esp32:32", "door:OUT", "green", []],
    ["esp32:26", "door:SEL", "white", []],
    ["esp32:GND.1", "door:GND", "black", []],
    ["esp32:3V3", "door:VCC", "red", []]
  ]
}
```

### 6.4 Flujo de Trabajo en VS Code

1. **Clonar el repositorio:**
   ```bash
   git clone https://github.com/<org>/incunest-wokwi-chips.git
   cd incunest-wokwi-chips
   ```

2. **Compilar los custom chips** (requiere wokwi-cli o wasm-pack):
   ```bash
   # Para cada chip en chips/
   cd chips/incu-ntc-skin
   # Compilar C → WASM (usando SDK de Wokwi)
   wokwi-chips-c-sdk build
   ```

3. **Abrir en VS Code:**
   - Abrir la carpeta del proyecto
   - Asegurar que `wokwi.toml` y `diagram.json` están en la raíz (o en `examples/full-incubator-demo/`)

4. **Iniciar simulación:**
   - Presionar `F1` → "Wokwi: Start Simulator"
   - El simulador cargará el firmware .bin del ESP32
   - Los custom chips aparecerán con sus controles interactivos

5. **Interactuar:**
   - Usar sliders para modificar temperatura, humedad, puerta
   - Observar respuesta del PID en tiempo real
   - Ver alarmas en consola serial (RFC2217 port 4000)

---

## 7. Diagrama de Simulación Completo

### 7.1 Esquema de Bloques

```
┌─────────────────────────────────────────────────────────────┐
│                     WOKWI SIMULATOR                         │
│                                                             │
│  ┌──────────────────┐        ┌───────────────────────────┐  │
│  │   ESP32 MCU      │        │   CUSTOM CHIPS            │  │
│  │   (firmware.bin) │        │                           │  │
│  │                  │        │   ┌─────────────────┐     │  │
│  │   GPIO39 ←───────┼────ADC─┤   │ incu-ntc-skin   │     │  │
│  │                  │        │   │ [Temp: 36.5°C]  │     │  │
│  │   I2C Bus ←──────┼──I2C───┤   ├─────────────────┤     │  │
│  │   (SDA:21        │        │   │ incu-sts35-temp  │     │  │
│  │    SCL:22)       │        │   │ [Temp: 34.0°C]  │ ×2  │  │
│  │                  │        │   ├─────────────────┤     │  │
│  │                  │        │   │ incu-sht4x-env   │     │  │
│  │                  │        │   │ [T:25° H:50%]   │     │  │
│  │                  │        │   ├─────────────────┤     │  │
│  │                  │        │   │ incu-ina3221     │ ×2  │  │
│  │                  │        │   │ [Current/Volt]  │     │  │
│  │                  │        │   ├─────────────────┤     │  │
│  │   GPIO27 ────PWM─┤────────┤   │ incu-heater-ssr  │     │  │
│  │                  │        │   │ [Thermal Model] │     │  │
│  │   GPIO12 ────PWM─┤────────┤   ├─────────────────┤     │  │
│  │   GPIO35 ←──TACH─┤────────┤   │ incu-fan-pwm    │     │  │
│  │                  │        │   │ [RPM feedback]  │     │  │
│  │   GPIO5  ────PWM─┤────────┤   ├─────────────────┤     │  │
│  │                  │        │   │ incu-buzzer      │     │  │
│  │   GPIO32 ←───ADC─┤────────┤   ├─────────────────┤     │  │
│  │   GPIO26 ────DIG─┤────────┤   │ incu-door-touch  │     │  │
│  │                  │        │   ├─────────────────┤     │  │
│  │   I2C ───────────┤────────┤   │ incu-humidifier  │     │  │
│  │                  │        │   └─────────────────┘     │  │
│  └──────────────────┘        └───────────────────────────┘  │
│                                                             │
│  ┌──────────────────┐                                       │
│  │  Serial Monitor  │  ← RFC2217 port 4000                  │
│  │  (Debug output)  │                                       │
│  └──────────────────┘                                       │
└─────────────────────────────────────────────────────────────┘
```

### 7.2 Modelo Térmico Interconectado

Los chips no son independientes — comparten el entorno térmico de la cámara:

```
              heater.PWM_duty
                    │
                    ▼
    ┌──────────────────────────┐
    │   THERMAL MODEL (shared) │
    │                          │
    │  T_chamber += ΔT_heater  │ ← P = duty × 150W
    │  T_chamber -= ΔT_loss    │ ← k × (T_chamber - T_ambient)
    │  T_chamber -= ΔT_door    │ ← if door open: extra loss
    │  T_chamber += ΔT_fan     │ ← fan mixing effect
    │                          │
    │  Output: T_chamber       │──→ sts35 sensors read this
    │  Output: T_skin          │──→ ntc probe reads this
    │  Output: H_chamber       │──→ sht4x/humidity sensors
    └──────────────────────────┘
```

> **Nota de implementación:** En una primera versión, cada chip puede ser independiente (controles manuales). En una versión avanzada, el modelo térmico puede interconectarse vía `shared/thermal-model.h`.

---

## 8. Aplicación Next.js — Visor 3D

### 8.1 Objetivo

Crear una aplicación web Next.js que:
1. Renderice el modelo 3D de la incubadora (`IN3_structure_v15.step`)
2. Muestre información en tiempo real del simulador Wokwi
3. Permita interacción visual (abrir puerta, ver temperatura como heatmap)

### 8.2 Stack Tecnológico

| Componente | Librería | Versión |
|------------|----------|---------|
| Framework | Next.js | 14+ (App Router) |
| 3D Engine | Three.js | latest |
| React Bindings | @react-three/fiber | latest |
| Controls | @react-three/drei | latest |
| STEP Loader | occt-import-js (OpenCASCADE WASM) | latest |
| UI | Tailwind CSS | latest |
| Estado | Zustand | latest |

### 8.3 Conversión del Fichero STEP

El fichero `IN3_structure_v15.step` necesita ser convertido para web:

**Opción A — Conversión previa (recomendada):**
```bash
# Usar FreeCAD CLI para convertir STEP → glTF
freecad-cmd -c "import Part; Part.open('IN3_structure_v15.step'); import importOBJ; importOBJ.export([FreeCAD.ActiveDocument.Objects], 'IN3_structure_v15.obj')"

# O usar la librería occt-import-js en un script Node:
npx convert-step-to-gltf IN3_structure_v15.step --output public/models/incubator.glb
```

**Opción B — Carga directa en runtime:**
```tsx
import occtimportjs from 'occt-import-js';

async function loadStepFile(url: string) {
  const response = await fetch(url);
  const buffer = await response.arrayBuffer();
  const result = await occtimportjs(buffer);
  // result.meshes → Three.js geometries
}
```

### 8.4 Estructura de la App

```
viewer-3d/
├── public/
│   └── models/
│       └── incubator.glb              # Modelo 3D convertido
├── src/
│   ├── app/
│   │   ├── layout.tsx
│   │   └── page.tsx                   # Página principal
│   ├── components/
│   │   ├── IncubatorViewer.tsx         # Componente Three.js principal
│   │   ├── ControlPanel.tsx            # Panel de controles (temp, hum, puerta)
│   │   ├── SensorOverlay.tsx           # Overlay con datos de sensores
│   │   └── ThermalHeatmap.tsx          # Visualización térmica
│   ├── hooks/
│   │   └── useWokwiConnection.ts       # Conexión WebSocket al simulador
│   └── stores/
│       └── incubatorStore.ts           # Estado global (Zustand)
├── package.json
├── next.config.js
├── tailwind.config.js
└── tsconfig.json
```

### 8.5 Componente Principal (ejemplo)

```tsx
'use client';
import { Canvas } from '@react-three/fiber';
import { OrbitControls, useGLTF, Environment } from '@react-three/drei';

function IncubatorModel() {
  const { scene } = useGLTF('/models/incubator.glb');
  return <primitive object={scene} scale={0.01} />;
}

export default function IncubatorViewer() {
  return (
    <div className="w-full h-screen">
      <Canvas camera={{ position: [2, 1.5, 2], fov: 50 }}>
        <ambientLight intensity={0.5} />
        <directionalLight position={[10, 10, 5]} intensity={1} />
        <IncubatorModel />
        <OrbitControls />
        <Environment preset="studio" />
      </Canvas>
    </div>
  );
}
```

---

## 9. Pasos de Implementación

### Fase 1: Infraestructura (Fundamentos)

| # | Paso | Descripción | Herramientas |
|---|------|-------------|-------------|
| 1.1 | Crear estructura de directorios | `chips/`, `shared/`, `examples/`, `viewer-3d/` | Shell/Git |
| 1.2 | Inicializar `package.json` | Mono-repo con workspaces | npm init |
| 1.3 | Configurar Wokwi SDK | Instalar toolchain para compilar chips C → WASM | wasi-sdk |
| 1.4 | Crear `Makefile` o `build.sh` | Script build para todos los chips | Make |

### Fase 2: Custom Chips (Sensores primero)

| # | Paso | Prioridad | Complejidad |
|---|------|-----------|-------------|
| 2.1 | `incu-ntc-skin` | 🔴 Alta | ⭐ Baja (solo analog) |
| 2.2 | `incu-sts35-temp` | 🔴 Alta | ⭐⭐ Media (I2C protocol) |
| 2.3 | `incu-sht4x-env` | 🟡 Media | ⭐⭐ Media (I2C + dual sensor) |
| 2.4 | `incu-heater-ssr` | 🔴 Alta | ⭐⭐⭐ Alta (modelo térmico) |
| 2.5 | `incu-fan-pwm` | 🟡 Media | ⭐⭐ Media (PWM + tach) |
| 2.6 | `incu-ina3221` | 🟡 Media | ⭐⭐⭐ Alta (I2C complejo) |
| 2.7 | `incu-humidifier` | 🟢 Baja | ⭐⭐ Media (I2C slave) |
| 2.8 | `incu-buzzer-alarm` | 🟢 Baja | ⭐ Baja (solo GPIO) |
| 2.9 | `incu-door-touch` | 🟢 Baja | ⭐ Baja (solo analog) |

### Fase 3: Integración Wokwi

| # | Paso | Descripción |
|---|------|-------------|
| 3.1 | Crear `wokwi.toml` | Configurar firmware + chips |
| 3.2 | Crear `diagram.json` | Wiring completo |
| 3.3 | Probar con firmware.bin | Cargar binario real del ESP32 |
| 3.4 | Iterar protocolos I2C | Ajustar según respuestas del firmware |
| 3.5 | Crear ejemplos parciales | heater-only, sensor-suite |

### Fase 4: Aplicación Next.js 3D

| # | Paso | Descripción |
|---|------|-------------|
| 4.1 | `npx create-next-app viewer-3d` | Scaffolding Next.js 14 |
| 4.2 | Convertir STEP → glTF | FreeCAD o occt-import-js |
| 4.3 | Componente IncubatorViewer | Three.js + react-three-fiber |
| 4.4 | Panel de controles | Sliders sincronizados con Wokwi |
| 4.5 | Conexión WebSocket | Leer datos del simulador en tiempo real |

### Fase 5: Documentación y Pulido

| # | Paso | Descripción |
|---|------|-------------|
| 5.1 | README.md del repo | Instrucciones completas |
| 5.2 | README por cada chip | Pines, parámetros, comportamiento |
| 5.3 | Capturas/GIFs demo | Documentación visual |
| 5.4 | CI/CD | Build automático de chips WASM |

---

## 10. Notas de los Modelos de IA

### 🔵 Gemini Pro — Enfoque en Arquitectura

> **Recomendación principal:** Priorizar un diseño modular donde cada custom chip sea completamente independiente y testeable por separado. El patrón mono-repo con `shared/` es ideal para reutilizar el modelo térmico entre chips.
>
> **Detalle clave:** La conversión STEP → glTF para web es el cuello de botella del visor 3D. Recomienda usar `occt-import-js` (OpenCASCADE compilado a WASM) ya que soporta STEP nativo sin conversión previa, aunque el rendimiento de la primera carga es mayor.
>
> **Riesgo identificado:** El firmware.bin precompilado puede tener dependencias de hardware (configuración de flash, partitions) que no coincidan con el ESP32 simulado en Wokwi. Recomendación: compilar desde fuente con PlatformIO para obtener un .elf que incluya símbolos de debug.

### 🟢 GPT-5.3 — Enfoque en Implementación Práctica

> **Recomendación principal:** Comenzar con los chips más simples (NTC analog, buzzer GPIO) para validar el pipeline de compilación C → WASM → Wokwi antes de abordar los I2C complejos. El "Hello World" de un chip Wokwi es crucial para entender el ciclo de desarrollo.
>
> **Detalle clave:** Para los sensores I2C (STS35, SHT4x, INA3221), es fundamental analizar las trazas I2C del firmware real. Sugerencia: usar `i2cdetect` y un logic analyzer virtual en Wokwi para ver exactamente qué comandos envía el firmware y qué respuestas espera.
>
> **Riesgo identificado:** El CrowPanel 7.0" (ESP32-S3 con display RGB paralelo) NO se puede simular directamente en Wokwi (no soporta displays RGB paralelo). Solución: simular solo el ESP32 motherboard y usar el Serial Monitor para la interfaz, o crear un chip custom que simule la comunicación UART con el display.
>
> **Nota sobre Next.js 3D:** Recomienda `@react-three/fiber` + `drei` por su ecosistema maduro. Para el STEP, usar la conversión previa a .glb (FreeCAD batch) es más fiable que runtime WASM parsing. El fichero STEP puede ser grande (>50MB), por lo que la conversión a glTF con compresión Draco reduce significativamente el tamaño.

### 🟣 Claude Opus 4.6 — Enfoque en Fidelidad de Simulación

> **Recomendación principal:** El valor diferencial del simulador está en la fidelidad del modelo térmico. Sin un modelo térmico realista, los chips son solo "generadores de números". Propone un modelo térmico centralizado en `shared/thermal-model.h` que todos los chips consulten:
>
> ```
> T_chamber(t+Δt) = T_chamber(t) 
>   + (P_heater × η_heater / C_thermal) × Δt
>   - (k_walls × (T_chamber - T_ambient)) × Δt
>   - (k_door × door_open × (T_chamber - T_ambient)) × Δt
>   + (k_fan × fan_speed × (T_target - T_chamber)) × Δt
> ```
>
> **Detalle clave:** Los parámetros PID del firmware real (`Arduino-PID-Library`) están calibrados para la respuesta térmica del hardware físico. En simulación, si el modelo térmico no coincide, el PID puede oscilar o ser inestable. Recomendación: incluir atributos de "tuning" en los chips para ajustar la constante térmica hasta conseguir estabilidad.
>
> **Riesgo identificado:** El firmware usa EEPROM para almacenar calibración de sensores y estado. Wokwi simula EEPROM/NVS del ESP32, pero la primera ejecución sin datos previos puede causar estados inesperados (ej: `EEPROM_FIRST_TURN_ON`). Recomendación: preparar una imagen EEPROM con valores de calibración preestablecidos.
>
> **Nota sobre completitud:** Los chips `incu-ina3221` deberían retroalimentarse automáticamente con las corrientes calculadas de los actuadores (heater PWM × 12.5A, fan × 0.5A, etc.) en lugar de depender solo de sliders manuales. Esto requiere comunicación entre chips, que en Wokwi se puede lograr mediante pines compartidos en el `diagram.json`.

---

## Apéndice A: Referencias

| Recurso | URL |
|---------|-----|
| IncuNest GitHub | https://github.com/medicalopenworld/IncuNest |
| Wokwi Chips API | https://docs.wokwi.com/chips-api/getting-started |
| Wokwi VS Code Extension | https://docs.wokwi.com/vscode/getting-started |
| Wokwi Project Config | https://docs.wokwi.com/vscode/project-config |
| CrowPanel 7.0" HMI | https://github.com/Elecrow-RD/CrowPanel-7.0-HMI-ESP32-Display-800x480 |
| Wokwi GPIO API | https://docs.wokwi.com/chips-api/gpio |
| Wokwi I2C API | https://docs.wokwi.com/chips-api/i2c |
| Wokwi Analog API | https://docs.wokwi.com/chips-api/analog |
| Wokwi Timer API | https://docs.wokwi.com/chips-api/time |
| Wokwi SPI API | https://docs.wokwi.com/chips-api/spi |
| Wokwi Attributes API | https://docs.wokwi.com/chips-api/attributes |
| Wokwi Framebuffer API | https://docs.wokwi.com/chips-api/framebuffer |
| Arduino PID Library | https://github.com/br3ttb/Arduino-PID-Library |
| INA3221 Library | https://github.com/beast-devices/Arduino-INA3221 |
| STS3x Library | https://github.com/Sensirion/arduino-i2c-sts3x |
| SHT4x Library | https://github.com/adafruit/Adafruit_SHT4X |
| SHTC3 Library | https://github.com/sparkfun/SparkFun_SHTC3_Arduino_Library |
| BQ25792 Library | https://github.com/andrew153d/BQ25792_Driver |
| AFE4490 Library | https://github.com/Protocentral/protocentral-afe4490-arduino |
| TCA9555 Library | https://github.com/RobTillaart/TCA9555 |
| occt-import-js | https://github.com/nicolo-ribaudo/occt-import-js |
| React Three Fiber | https://github.com/pmndrs/react-three-fiber |

## Apéndice B: Licencia

Este proyecto de simulación se distribuye bajo la misma licencia que IncuNest:
**Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0)**

Atribución: "Medicina Abierta al Mundo – IncuNest Project"

Para uso comercial, contactar: info@medicalopenworld.org
