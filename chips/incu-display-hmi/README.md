# IncuNest Display HMI

Simula la pantalla CrowPanel 7.0" HMI V3.0 (ESP32-S3, 800×480) que se comunica con la MotherBoard por UART.

## Especificaciones del Hardware Real

| Parámetro | Valor |
|-----------|-------|
| Modelo | Elecrow CrowPanel 7.0" ESP32 HMI |
| Versión | V3.0 |
| MCU | ESP32-S3-WROOM-1-N4R8 (4MB Flash, 8MB PSRAM) |
| Resolución | 800×480 TFT-LCD (panel TN) |
| Driver | EK9716BD3 + EK73002ACGB (RGB paralelo) |
| Touch | GT911 capacitivo (I2C: SDA=19, SCL=20) |
| IO Expander | PCA9557 (V3.0 timing control) |
| Framework | LVGL 8.3.3 |
| Alimentación | DC 5V 2A |

> **Fuente:** [CrowPanel-7.0-HMI-ESP32-Display-800x480](https://github.com/Elecrow-RD/CrowPanel-7.0-HMI-ESP32-Display-800x480) (V3.0)

## Simulación en Wokwi

Al no soportar Wokwi displays RGB paralelo, este chip simula la pantalla usando la API framebuffer (800×480) y el protocolo UART real del firmware IncuNest.

### Pines

| Pin | Dirección | Conectar a | Descripción |
|-----|-----------|-----------|-------------|
| RX | Input | TX del ESP32 | Recibe CTRL,TEL/STATE/ALM |
| TX | Output | RX del ESP32 | Envía HMI,REQ,STATE |
| BTN_NEXT | Input (pull-up) | Pulsador a GND | Cambia a la siguiente pantalla |
| BTN_PREV | Input (pull-up) | Pulsador a GND | Cambia a la pantalla anterior |
| BTN_OK | Input (pull-up) | Pulsador a GND | Acción contextual / lock |
| BTN_UP | Input (pull-up) | Pulsador a GND | Navegación vertical arriba (SETTINGS/ALARMS) |
| BTN_DOWN | Input (pull-up) | Pulsador a GND | Navegación vertical abajo (SETTINGS/ALARMS) |
| VCC | Power | 3.3V | Alimentación |
| GND | Power | GND | Tierra |

### Protocolo UART (115200 baud, 8N1)

**Recibe (MB → Display):**
- `CTRL,TEL,<airTemp>,<skinTemp>,<humidity>[,<commStatus>]\n`
- `CTRL,STATE,<act>,<mode>,<airSet>,<skinSet>,<humSet>,...\n`
- `CTRL,ALM,<id>,<type>,<desc>,<state>\n`

**Envía (Display → MB):**
- `HMI,REQ,STATE\n` — Al iniciar
- `HMI,<act>,<skinMode>,<mode>,<airTemp>,<skinTemp>,<hum>,<photo>,<mute>,<lang>,<photoMin>\n`

### Atributos Configurables

| Atributo | Default | Descripción |
|----------|---------|-------------|
| controlMode | 0 | 0=SKIN, 1=AIR (firmware convention) |
| language | 0 | 0=EN, 1=ES, 2=FR, 3=PT |
| skinEnabled | 1 | Habilitar sensor piel |
| airSetpoint | 34.0 | Temperatura aire objetivo (°C) |
| skinSetpoint | 36.5 | Temperatura piel objetivo (°C) |
| humSetpoint | 60.0 | Humedad objetivo (%) |
| commTimeoutMs | 3000 | Timeout comunicación (ms) |
| autoRequestState | 1 | Solicitar estado al iniciar |

### Interfaz Visual

El display renderiza un dashboard médico con navegación interactiva (`BTN_PREV`, `BTN_NEXT`, `BTN_OK`, `BTN_UP`, `BTN_DOWN`):
- **Intro/Boot**
- **Header**: Modo (AIR/SKIN) + indicador conexión
- **Main**: Panel temperatura, humedad, actuadores y alarmas
- **Settings**: Snapshot de setpoints/estado
- **Alarms**: Lista ampliada de alarmas activas
- **Charts**: Barras en vivo + placeholder de histórico
- **PulseOxi**: Placeholders de SpO₂/pulso + datos disponibles
- **Lock**: Bloqueo/desbloqueo de navegación
- **Footer**: S/N, HW, FW, uptime y hint de navegación

`BTN_UP/BTN_DOWN` hacen navegación vertical en pantalla:
- **SETTINGS**: selección de fila con scroll.
- **ALARMS**: selección de alarma activa.

`BTN_OK` en **SETTINGS** aplica cambios en la fila seleccionada (incluyendo ciclo de `Language`).

### Ejemplo diagram.json

```json
{
  "type": "chip-incu-display-hmi",
  "id": "display",
  "top": 680,
  "left": 350,
  "attrs": {
    "controlMode": "0",
    "airSetpoint": "34.0"
  }
}
```

### Compilación

```bash
# Docker (recomendado):
docker run --rm -v "$PWD/chips:/src" wokwi/builder-clang-wasm:latest make -B

# Script auxiliar (autodetecta Docker → LLVM nativo):
./tools/build-chips.sh -B
```

> Sin Docker: ver README.md raíz sección "Sin Docker (fallback nativo)".
