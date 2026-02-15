#include "wokwi-api.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  uint32_t water_level_attr;
  uint32_t address_attr;
  uint8_t duty_cycle;
  uint8_t read_index;
  uint8_t write_index;
  uint8_t write_buf[4];
} chip_state_t;

static bool on_i2c_connect(void *user_data, uint32_t address, bool read) {
  chip_state_t *chip = (chip_state_t *)user_data;
  (void)address;
  if (read) {
    chip->read_index = 0;
  } else {
    chip->write_index = 0;
  }
  return true;
}

static uint8_t on_i2c_read(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  uint8_t water_level = (uint8_t)attr_read(chip->water_level_attr);
  switch (chip->read_index++) {
    case 0:
      return chip->duty_cycle;
    case 1:
      return water_level;
    case 2:
      return water_level == 0 ? 0x02 : 0x00; /* 0x02 = empty tank */
    default:
      return 0xFF;
  }
}

static bool on_i2c_write(void *user_data, uint8_t data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  if (chip->write_index < sizeof(chip->write_buf)) {
    chip->write_buf[chip->write_index++] = data;
  }

  if (chip->write_index == 1) {
    chip->duty_cycle = data > 95 ? 95 : data;
  } else if (chip->write_index >= 2 && chip->write_buf[0] == 0x01) {
    chip->duty_cycle = chip->write_buf[1] > 95 ? 95 : chip->write_buf[1];
  }
  return true;
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->water_level_attr = attr_init("waterLevel", 80);
  chip->address_attr = attr_init("address", 0x42);
  chip->duty_cycle = 0;
  chip->read_index = 0;
  chip->write_index = 0;

  uint32_t address = attr_read(chip->address_attr);
  if (address < 0x08 || address > 0x77) {
    address = 0x42;
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
