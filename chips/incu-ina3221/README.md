# incu-ina3221

Modelo virtual del sensor de corriente INA3221 (3 canales) por I2C.

## Pines

- `VCC`
- `GND`
- `SCL`
- `SDA`

## Controles

- `address` (`64`=`0x40`, `65`=`0x41`)
- `ch1Current`, `ch2Current`, `ch3Current` (A)
- `busVoltage` (V)
