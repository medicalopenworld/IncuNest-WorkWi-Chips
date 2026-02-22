# Documentación — IncuNest Wokwi Simulation

## Guías

| Documento | Descripción |
|-----------|-------------|
| [simulation-guide.md](simulation-guide.md) | Guía completa de la simulación: arquitectura, protocolo UART, componentes, build |

## Informes y Análisis

| Documento | Descripción |
|-----------|-------------|
| [INFORME_FIDELIDAD_SIMULACION.md](INFORME_FIDELIDAD_SIMULACION.md) | Análisis de fidelidad simulación vs hardware real (basado en BOM v15) |
| [COMPONENT_REFERENCES.md](COMPONENT_REFERENCES.md) | Datasheets y especificaciones de los 13+ componentes simulados |

## Imágenes

| Archivo | Descripción |
|---------|-------------|
| [screenshot-overview.png](screenshot-overview.png) | Vista general de la simulación |
| [screenshot-parts.png](screenshot-parts.png) | Detalle de componentes |
| [screenshot-vscode-insiders.png](screenshot-vscode-insiders.png) | Captura en Visual Studio Code - Insiders |

## Documentación por chip

Cada chip tiene su propio README en `chips/incu-*/README.md`.
El chip `incu-display-hmi` incluye además un `DESIGN.md` con la especificación técnica del protocolo HMI.

## Referencia externa

- **Hardware upstream**: [medicalopenworld/IncuNest](https://github.com/medicalopenworld/IncuNest)
- **Display V3**: [Elecrow CrowPanel 7.0"](https://github.com/Elecrow-RD/CrowPanel-7.0-HMI-ESP32-Display-800x480)
