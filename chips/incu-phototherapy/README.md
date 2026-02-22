# incu-phototherapy

Chip personalizado de Wokwi que simula la placa LED de fototerapia del IncuNest para tratamiento de ictericia neonatal.

## Pines

- `PWM_IN`: entrada PWM (GPIO13 en firmware, 2000 Hz, 8-bit)
- `EN`: habilitación global de actuadores (`ACTUATORS_EN`, GPIO14, activo en alto)
- `VCC`: alimentación
- `GND`: tierra

## Uso

1. Conecta `PWM_IN` al pin de fototerapia del ESP32.
2. Conecta `EN` al enable global de actuadores.
3. Al habilitar `EN` y aplicar PWM, el indicador se enciende en azul y muestra el duty cycle.
4. El chip calcula duty por temporización de flancos y acumula tiempo total encendido internamente.
