#include "wokwi-api.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  uint32_t address_attr;
  uint32_t ch1_current_attr;
  uint32_t ch2_current_attr;
  uint32_t ch3_current_attr;
  uint32_t bus_voltage_attr;
  uint8_t pointer_register;
  uint8_t write_index;
  bool send_high_byte;
  uint16_t active_register_word;
} chip_state_t;

static int16_t clamp_i16(int32_t value, int16_t min_v, int16_t max_v) {
  if (value < min_v) return min_v;
  if (value > max_v) return max_v;
  return (int16_t)value;
}

static uint16_t ina_shunt_register(float current_a) {
  /* Simplified model with 10mOhm shunt and 40uV LSB */
  float shunt_voltage = current_a * 0.01f;
  int32_t raw = (int32_t)(shunt_voltage / 0.00004f);
  int16_t clamped = clamp_i16(raw, -4096, 4095);
  return (uint16_t)(((uint16_t)clamped << 3) & 0xFFF8);
}

static uint16_t ina_bus_register(float voltage_v) {
  int32_t raw = (int32_t)(voltage_v / 0.008f);
  if (raw < 0) raw = 0;
  if (raw > 8191) raw = 8191;
  return (uint16_t)((raw << 3) & 0xFFF8);
}

static uint16_t register_value(chip_state_t *chip, uint8_t reg) {
  float i1 = attr_read_float(chip->ch1_current_attr);
  float i2 = attr_read_float(chip->ch2_current_attr);
  float i3 = attr_read_float(chip->ch3_current_attr);
  float vbus = attr_read_float(chip->bus_voltage_attr);

  switch (reg) {
    case 0x00:
      return 0x7127; /* Config register (default-like) */
    case 0x01:
      return ina_shunt_register(i1);
    case 0x02:
      return ina_bus_register(vbus);
    case 0x03:
      return ina_shunt_register(i2);
    case 0x04:
      return ina_bus_register(vbus);
    case 0x05:
      return ina_shunt_register(i3);
    case 0x06:
      return ina_bus_register(vbus);
    default:
      return 0x0000;
  }
}

static bool on_i2c_connect(void *user_data, uint32_t address, bool read) {
  chip_state_t *chip = (chip_state_t *)user_data;
  (void)address;
  chip->write_index = 0;
  chip->send_high_byte = true;
  if (read) {
    chip->active_register_word = register_value(chip, chip->pointer_register);
  }
  return true;
}

static uint8_t on_i2c_read(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  if (chip->send_high_byte) {
    chip->send_high_byte = false;
    return (uint8_t)(chip->active_register_word >> 8);
  }

  uint8_t out = (uint8_t)(chip->active_register_word & 0xFF);
  chip->pointer_register++;
  chip->active_register_word = register_value(chip, chip->pointer_register);
  chip->send_high_byte = true;
  return out;
}

static bool on_i2c_write(void *user_data, uint8_t data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  if (chip->write_index == 0) {
    chip->pointer_register = data;
  }
  chip->write_index++;
  return true;
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->address_attr = attr_init("address", 65);
  chip->ch1_current_attr = attr_init_float("ch1Current", 0.0f);
  chip->ch2_current_attr = attr_init_float("ch2Current", 0.0f);
  chip->ch3_current_attr = attr_init_float("ch3Current", 0.0f);
  chip->bus_voltage_attr = attr_init_float("busVoltage", 12.0f);
  chip->pointer_register = 0x00;
  chip->write_index = 0;
  chip->send_high_byte = true;
  chip->active_register_word = 0x7127;

  uint32_t address = attr_read(chip->address_attr);
  if (address < 0x08 || address > 0x77) {
    address = 0x41;
  }

  const i2c_config_t i2c_config = {
      .address = address,
      .scl = pin_init("SCL", INPUT_PULLUP),
      .sda = pin_init("SDA", INPUT_PULLUP),
      .connect = on_i2c_connect,
      .read = on_i2c_read,
      .write = on_i2c_write,
      .user_data = chip,
  };
  i2c_init(&i2c_config);
}
