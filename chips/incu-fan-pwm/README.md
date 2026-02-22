# incu-fan-pwm

Ventilador virtual con entrada PWM, salida de tacómetro y representación visual animada.

## Pines

- `VCC`
- `GND`
- `PWM_IN`
- `TACH_OUT`
- `EN`

## Control

- `maxRpm` (RPM máximo)

## Representación Visual (Framebuffer)

El chip utiliza la API de Framebuffer de Wokwi (`"display"`) para dibujar una representación animada del ventilador en tiempo real.

- **Resolución:** 64x64 píxeles.
- **Animación:** Las aspas del ventilador giran a una velocidad proporcional a las RPM actuales calculadas a partir de la señal PWM.
- **Colores (Formato Little Endian `0xAABBGGRR`):**
  - Fondo: Azul oscuro (`0xFF30180A`) para simular transparencia sobre la PCB.
  - Marco y Centro: Gris oscuro (`0xFF404040`).
  - Aspas: Gris claro (`0xFFEEEEEE`).
- **Rendimiento:** Utiliza el algoritmo de línea de Bresenham y tablas de búsqueda (LUT) precalculadas para senos y cosenos, evitando cálculos de punto flotante durante el renderizado a 20 FPS.
