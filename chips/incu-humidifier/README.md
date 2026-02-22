# incu-humidifier

Esclavo I2C virtual para control de humidificador. Incluye visualización gráfica animada mediante la API Framebuffer de Wokwi.

## Pines

| Pin  | Dirección | Descripción              |
|------|-----------|--------------------------|
| `VCC`| —         | Alimentación 3.3V        |
| `GND`| —         | Referencia de tierra     |
| `SCL`| Entrada   | Reloj I2C                |
| `SDA`| Bidireccional | Datos I2C            |
| `DUTY_OVERRIDE` | Entrada analógica | Duty externo (0..3.3V = 0..95%) |
| `OVERRIDE_EN` | Entrada digital | Habilita modo override |
| `HUMIDITY_OUT` | Salida analógica | Humedad generada (0..3.3V = 0..100% RH extra) |

## Controles (Atributos de simulación)

| Control      | Tipo  | Rango  | Descripción                                   |
|--------------|-------|--------|-----------------------------------------------|
| `waterLevel` | range | 0–100  | Nivel inicial/refill del depósito (%)         |
| `address`    | range | 32–127 | Dirección I2C del dispositivo (decimal)       |
| `speedMode`  | range | 0–2    | Modo de simulación: 0=Normal, 1=Rapido, 2=Acelerado |

## Protocolo I2C

### Escritura (Master → Chip)

| Bytes enviados | Acción                                                         |
|----------------|----------------------------------------------------------------|
| `[duty]`       | Establece duty cycle del humidificador (0–95%)                 |
| `[0x01, duty]` | Comando explícito para establecer duty cycle (0–95%)           |

El duty cycle máximo se limita a 95 para proteger el hardware.

Si `OVERRIDE_EN=HIGH`, el chip usa `DUTY_OVERRIDE` como duty efectivo (fallback I2C desactivado).

### Lectura (Master ← Chip)

| Byte | Valor devuelto                                 |
|------|------------------------------------------------|
| 0    | Duty cycle actual (0–95)                       |
| 1    | Nivel de agua simulado dinámico (0–100)        |
| 2    | Estado: `0x02` si depósito vacío, `0x00` si OK |

## Modelo físico simplificado

- El depósito se vacía progresivamente en función del duty (más duty → más consumo de agua).
- Si cambias `waterLevel` en el slider, se interpreta como refill/manual set del nivel de tanque.
- La salida `HUMIDITY_OUT` representa la humedad extra generada por vapor y cae cuando baja duty o se vacía el depósito.
- `speedMode` acelera la dinámica para pruebas:
  - `Normal` (0): factor 1x
  - `Rapido` (1): factor 5x
  - `Acelerado` (2): factor 25x

## Visualización Gráfica (Framebuffer 64×64)

El chip renderiza un panel visual animado a 20 FPS directamente sobre el componente en Wokwi:

- **Fondo azul oscuro (navy `#0A1830`)** — Wokwi no soporta transparencia alpha en el framebuffer; los píxeles siempre reemplazan el canvas negro del display. Se usa azul marino como alternativa que integra bien con la paleta del tanque.
- **Tanque azul** — borde azul oscuro (`#0055AA`) con cuerpo azul medio (`#55AAFF`) e interior azul muy claro (`#DDEEFF`).
- **Nivel de agua** — rectángulo azul brillante (`#2288FF`) que sube o baja dinámicamente según el valor del atributo `waterLevel`. Al 100% el depósito aparece lleno; al 0% completamente vacío.
- **Partículas de vapor animadas** — cuando `duty_cycle > 0` y hay agua disponible, aparecen pequeñas partículas de vapor (color azul claro) subiendo desde la boca del depósito. La cantidad de partículas es proporcional a la potencia del humidificador (`duty_cycle / 10`).

### Paleta de colores

| Constante         | Valor AABBGGRR  | Descripción               |
|-------------------|-----------------|---------------------------|
| `COLOR_BG`          | `0xFF30180A` | Fondo azul muy oscuro (navy) |
| `COLOR_TANK_BORDER` | `0xFFAA5500` | Borde azul oscuro         |
| `COLOR_TANK_FILL`   | `0xFFFFAA55` | Cuerpo del tanque         |
| `COLOR_TANK_INSIDE` | `0xFFFFEEDD` | Interior vacío del tanque |
| `COLOR_WATER`       | `0xFFFF8822` | Nivel de agua             |
| `COLOR_VAPOR`       | `0xFFFFDDAA` | Partículas de vapor       |

> **Nota:** Wokwi utiliza el formato de color `0xAABBGGRR` (Little Endian) cuando se usa la propiedad `"display"` en el archivo `chip.json`. El canal alfa (A) siempre debe ser `0xFF` para que el píxel sea opaco.
