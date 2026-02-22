# IncuNest SIM800C GPRS

Simula el módulo SIMCom SIM800C GSM/GPRS (U2 en la PCB de IncuNest) utilizado para conectividad IoT y telemedicina. Se comunica por UART mediante comandos AT estándar.

## Especificaciones del Hardware Real

| Parámetro | Valor |
|-----------|-------|
| Modelo | SIMCom SIM800C |
| Interfaz | UART (AT commands) |
| Frecuencias | GSM 850/900/1800/1900 MHz |
| Datos | GPRS multi-slot class 12 |
| Alimentación | 3.4V – 4.4V |
| Baud rate | 115200 (default) |

## Simulación en Wokwi

Este chip implementa un subconjunto de comandos AT del SIM800C suficiente para simular el flujo de telemetría HTTP POST del firmware IncuNest.

### Pines

| Pin | Dirección | Conectar a | Descripción |
|-----|-----------|-----------|-------------|
| RX | Input | TX del ESP32 | Recibe comandos AT |
| TX | Output | RX del ESP32 | Respuestas AT |
| PWRKEY | Input | GPIO (pull-up) | Encendido/apagado (>1s LOW) |
| VCC | Power | 3.3V | Alimentación |
| GND | Power | GND | Tierra |

### Comandos AT Soportados

| Comando | Respuesta | Descripción |
|---------|-----------|-------------|
| `AT` | `OK` | Test de comunicación |
| `ATE0` / `ATE1` | `OK` | Echo off/on |
| `AT+GMR` | `Revision:SIM800C_v1.0` | Versión firmware |
| `AT+CSQ` | `+CSQ: <rssi>,0` | Calidad de señal |
| `AT+CREG?` | `+CREG: 0,<stat>` | Estado registro red |
| `AT+CGATT?` | `+CGATT: <state>` | Estado GPRS |
| `AT+COPS?` | `+COPS: 0,0,"Simulated"` | Operador |
| `AT+CPIN?` | `+CPIN: READY` | Estado SIM |
| `AT+CFUN=<n>` | `OK` | Modo funcionalidad |
| `AT+SAPBR=...` | `OK` | Configuración bearer |
| `AT+HTTPINIT` | `OK` | Iniciar sesión HTTP |
| `AT+HTTPPARA=...` | `OK` | Parámetros HTTP |
| `AT+HTTPACTION=<m>` | `OK` → `+HTTPACTION: <m>,200,15` | Ejecutar HTTP (respuesta async ~1.5s) |
| `AT+HTTPREAD` | `+HTTPREAD: 15\n{"status":"ok"}` | Leer respuesta HTTP |
| `AT+HTTPTERM` | `OK` | Terminar sesión HTTP |
| `AT+CIPMUX=<n>` | `OK` | Modo conexión TCP |
| `AT+CIPSTART=...` | `OK` → `CONNECT OK` | Conectar TCP (async ~1s) |
| `AT+CIPSEND=<n>` | `>` → `SEND OK` | Enviar datos TCP |

### Máquina de Estados

```
POWER_OFF → POWER_ON → REGISTERED → GPRS_ATTACHED → HTTP_ACTIVE
    ↑                                                      |
    └──────────── PWRKEY toggle (>1s) ─────────────────────┘
```

- Al encender, auto-avanza según atributos `networkRegistered` y `gprsAttached`
- `AT+CREG?` avanza a REGISTERED
- `AT+CGATT?` avanza a GPRS_ATTACHED
- `AT+HTTPINIT` avanza a HTTP_ACTIVE

### Atributos Configurables

| Atributo | Default | Descripción |
|----------|---------|-------------|
| signalStrength | 20 | Intensidad señal CSQ (0-31) |
| networkRegistered | 1 | Estado registro (0=no, 1=home, 5=roaming) |
| gprsAttached | 1 | GPRS adjunto (0=no, 1=sí) |
| httpResponseCode | 200 | Código HTTP simulado |

### Interfaz Visual (140×60)

- **Header**: "SIM800C" + barras de señal
- **Línea 1**: Estado registro + GPRS
- **Línea 2**: Valor CSQ + estado máquina
- **Línea 3**: Operador simulado
- **Línea 4**: Estado HTTP (si activo)

### Ejemplo diagram.json

```json
{
  "type": "chip-incu-sim800c-gprs",
  "id": "sim800c",
  "top": 400,
  "left": 600,
  "attrs": {
    "signalStrength": "25",
    "networkRegistered": "1",
    "gprsAttached": "1"
  }
}
```

### Compilación

```bash
# Docker (recomendado):
docker run --rm -v "$PWD/chips:/src" wokwi/builder-clang-wasm:latest make -B

# Script auxiliar:
./tools/build-chips.sh -B

# macOS nativo:
make CC=/opt/homebrew/opt/llvm/bin/clang \
     WASI_SYSROOT=/opt/homebrew/opt/wasi-libc/share/wasi-sysroot \
     EXTRA_FLAGS="-nodefaultlibs -lc"
```
