#include "wokwi-api.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  uint32_t temperature_attr;
  uint32_t address_attr;
  uint8_t response[3];
  uint8_t response_index;
} chip_state_t;

static uint8_t crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0xFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
    }
  }
  return crc;
}

static void prepare_temperature_response(chip_state_t *chip) {
  float t = attr_read_float(chip->temperature_attr);
  uint16_t raw = (uint16_t)(((t + 45.0f) * 65535.0f) / 175.0f);
  chip->response[0] = (uint8_t)(raw >> 8);
  chip->response[1] = (uint8_t)(raw & 0xFF);
  chip->response[2] = crc8(chip->response, 2);
  chip->response_index = 0;
}

static bool on_i2c_connect(void *user_data, uint32_t address, bool read) {
  chip_state_t *chip = (chip_state_t *)user_data;
  (void)address;
  if (read) {
    prepare_temperature_response(chip);
  }
  return true;
}

static uint8_t on_i2c_read(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  if (chip->response_index < sizeof(chip->response)) {
    return chip->response[chip->response_index++];
  }
  return 0xFF;
}

static bool on_i2c_write(void *user_data, uint8_t data) {
  (void)user_data;
  (void)data;
  return true;
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->temperature_attr = attr_init_float("temperature", 34.0f);
  chip->address_attr = attr_init("address", 74);
  chip->response_index = 0;

  uint32_t address = attr_read(chip->address_attr);
  if (address < 0x08 || address > 0x77) {
    address = 0x4A;
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
