# Referencia de Componentes — IncuNest Simulation

Este documento detalla los componentes clave utilizados en el hardware de la incubadora IncuNest y sus contrapartes en la simulación Wokwi.

---

## 1. Sensores

### 1.1 Sensirion STS35-DIS (Temperatura Digital)
Sensor de temperatura de alta precisión utilizado para el control térmico crítico (aire y seguridad).

- **Protocolo:** I2C
- **Direcciones:** 0x4A (ADDR→GND), 0x4B (ADDR→VCC)
- **Chip de Simulación:** `incu-sts35-temp`
- **Datasheet:** [Sensirion STS35-DIS PDF](https://sensirion.com/media/documents/213E6A3B/63A5A569/Datasheet_STS3x_DIS.pdf)

| Especificación | Valor |
| :--- | :--- |
| Rango de Medición | -40°C a +125°C |
| Precisión (Típica) | ±0.1°C (20°C a 60°C) |
| Tiempo de Respuesta | 2s (tau63%) |
| Comandos Clave | Single shot (0x2400), Read status (0xF32D) |

### 1.2 Sensirion SHT4x (Temperatura y Humedad)
Sensor robusto para monitoreo ambiental dentro de la cámara de la incubadora.

- **Protocolo:** I2C
- **Dirección:** 0x44
- **Chip de Simulación:** `incu-sht4x-env`
- **Datasheet:** [Sensirion SHT4x PDF](https://sensirion.com/media/documents/33FD6951/64E3E5C5/Sensirion_Datasheet_SHT4x.pdf)

| Especificación | Valor |
| :--- | :--- |
| Rango Humedad | 0 a 100% RH |
| Precisión Humedad | ±1.8% RH |
| Precisión Temp | ±0.2°C |
| Comandos Clave | Measure high precision (0xFD), Serial (0x89) |

### 1.3 Sensirion SHTC3 (Temperatura y Humedad - Auxiliar)
Utilizado para monitoreo ambiental secundario o externo.

- **Protocolo:** I2C
- **Dirección:** 0x70
- **Chip de Simulación:** Wokwi Standard / Generic I2C
- **Datasheet:** [Sensirion SHTC3 PDF](https://sensirion.com/media/documents/643F9C8E/6164081E/Sensirion_Humidity_Sensors_SHTC3_Datasheet.pdf)

| Especificación | Valor |
| :--- | :--- |
| Modos | Normal, Low Power |
| Precisión | ±0.2°C, ±2% RH |
| Comandos Clave | Wakeup (0x3517), Sleep (0xB098), Measure (0x7866) |

### 1.4 NTC 10kΩ Thermistor (Temperatura de Piel)
Sensor analógico para monitoreo directo de la temperatura del paciente (modo piel).

- **Protocolo:** Analógico (Divisor de voltaje)
- **Conexión:** GPIO 39 (v14) / GPIO 10 (v15)
- **Chip de Simulación:** `incu-ntc-skin`
- **Especificación:** Beta=3950, R25=10kΩ

| Especificación | Valor |
| :--- | :--- |
| Rango Crítico | 20°C - 45°C |
| Resolución ADC | 12-bit (ESP32) |
| Configuración | Pull-up o Pull-down con R fija |

---

## 2. Monitoreo de Energía

### 2.1 Texas Instruments INA3221
Monitor de corriente y voltaje de triple canal para supervisar el consumo del sistema.

- **Protocolo:** I2C
- **Direcciones:** 0x40 (GND), 0x41 (VS)
- **Chip de Simulación:** `incu-ina3221`
- **Datasheet:** [TI INA3221 PDF](https://www.ti.com/lit/ds/symlink/ina3221.pdf)

| Canal | Shunt (mΩ) | Propósito |
| :--- | :--- | :--- |
| CH1 | 2mΩ | Sistema General |
| CH2 | 82mΩ | Fototerapia |
| CH3 | 100mΩ | Ventilador / Aux |

---

## 3. SpO2 / Pulse Oximetry

### 3.1 Texas Instruments AFE4490
Front-end analógico integrado para oximetría de pulso.

- **Protocolo:** SPI
- **Configuración:** CPOL=0, CPHA=0, 24-bit data
- **Chip de Simulación:** *Simulado parcialmente vía lógica SPI o módulo externo*
- **Datasheet:** [TI AFE4490 PDF](https://www.ti.com/lit/ds/symlink/afe4490.pdf)

| Registro | Dirección | Descripción |
| :--- | :--- | :--- |
| CONTROL0 | 0x00 | Control general |
| LED2VAL | 0x2A | Valor corriente LED IR |
| LED1VAL | 0x2C | Valor corriente LED Rojo |

---

## 4. Comunicación

### 4.1 SIMCom SIM800C (GSM/GPRS)
Módulo celular para telemetría y alertas remotas.

- **Protocolo:** UART (AT Commands)
- **Baudrate:** 115200
- **Chip de Simulación:** `incu-telemetry-reporter` (Emulador de respuesta AT)
- **Manual:** [SIM800C Hardware Design](https://www.elecrow.com/download/SIM800C_Hardware_Design_V1.02.pdf)

| Comandos Clave | Función |
| :--- | :--- |
| `AT+CSQ` | Calidad de señal |
| `AT+CREG?` | Estado de registro de red |
| `AT+HTTPACTION` | Iniciar método HTTP (GET/POST) |

---

## 5. Interfaz Humano-Máquina (Display)

### 5.1 Elecrow CrowPanel 7.0" HMI V3
Pantalla inteligente basada en ESP32-S3 con capacidad táctil.

- **Protocolo:** UART (Comunicación con placa madre)
- **Resolución:** 800 x 480 px
- **Chip de Simulación:** `incu-display-hmi`
- **Repositorio:** [GitHub Elecrow HMI](https://github.com/Elecrow-RD/CrowPanel-7.0-HMI-ESP32-Display-800x480)

| Característica | Detalle |
| :--- | :--- |
| Touch | Capacitivo (GT911 I2C) |
| Driver LCD | EK9716BD3 (RGB) |
| Interfaz | UART 115200 baudios |

---

## 6. Actuadores

### 6.1 CUI CMT-1285C-035 (Buzzer)
Alarma sonora piezoeléctrica para alertas críticas.

- **Tipo:** Piezoeléctrico (Self-drive oscillation capable)
- **Chip de Simulación:** `incu-buzzer-alarm`
- **Datasheet:** [CUI CMT-1285C-035 PDF](https://www.cuidevices.com/product/resource/cmt-1285c-035.pdf)

| Spec | Valor |
| :--- | :--- |
| Frecuencia | 2800 ± 500 Hz |
| SPL | 85 dB @ 10cm |
| Voltaje | 3.5 - 12 V |

### 6.2 Fan 6025 DC
Ventilador para circulación de aire y enfriamiento.

- **Control:** PWM (MOSFET Low-side)
- **Feedback:** Tacómetro (2 pulsos/rev)
- **Chip de Simulación:** `incu-fan-pwm`

### 6.3 WS2812B (LED RGB)
Indicador de estado del sistema direccionable.

- **Protocolo:** 1-wire (800kHz)
- **Chip de Simulación:** Wokwi Neopixel Standard
- **Datasheet:** [WS2812B PDF](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf)

---

## 7. Gestión de Energía (Referencia)

Estos componentes están presentes en el esquemático pero no son simulados activamente en la lógica principal de Wokwi, salvo emulación de registros I2C básicos si es necesario.

- **BQ25792**: Cargador de batería (I2C 0x6B) - [Datasheet](https://www.ti.com/lit/ds/symlink/bq25792.pdf)
- **BQ34110**: Fuel Gauge (I2C 0x55) - [Datasheet](https://www.ti.com/lit/ds/symlink/bq34110.pdf)

---

## Resumen de Mapeo (BOM vs Simulación)

| Componente | Designador (Ref) | Chip de Simulación Wokwi |
| :--- | :--- | :--- |
| STS35-DIS | U3, U4 | `incu-sts35-temp` |
| SHT4x | U5 | `incu-sht4x-env` |
| NTC Thermistor | TH1 | `incu-ntc-skin` |
| INA3221 | U8 | `incu-ina3221` |
| SIM800C | U10 | `incu-telemetry-reporter` |
| HMI Display | J_HMI | `incu-display-hmi` |
| Buzzer | BZ1 | `incu-buzzer-alarm` |
| Fan DC | FAN1 | `incu-fan-pwm` |
| Heater (SSR) | K1 | `incu-heater-ssr` |
| Humidifier | HUM1 | `incu-humidifier` |
