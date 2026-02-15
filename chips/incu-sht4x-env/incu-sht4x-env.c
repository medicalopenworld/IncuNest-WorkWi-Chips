#include "wokwi-api.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  uint32_t temperature_attr;
  uint32_t humidity_attr;
  uint8_t response[6];
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

static void prepare_measurement(chip_state_t *chip) {
  float t = attr_read_float(chip->temperature_attr);
  float h = attr_read_float(chip->humidity_attr);

  uint16_t t_raw = (uint16_t)(((t + 45.0f) * 65535.0f) / 175.0f);
  uint16_t h_raw = (uint16_t)(((h + 6.0f) * 65535.0f) / 125.0f);

  chip->response[0] = (uint8_t)(t_raw >> 8);
  chip->response[1] = (uint8_t)(t_raw & 0xFF);
  chip->response[2] = crc8(chip->response, 2);
  chip->response[3] = (uint8_t)(h_raw >> 8);
  chip->response[4] = (uint8_t)(h_raw & 0xFF);
  chip->response[5] = crc8(chip->response + 3, 2);
  chip->response_index = 0;
}

static bool on_i2c_connect(void *user_data, uint32_t address, bool read) {
  chip_state_t *chip = (chip_state_t *)user_data;
  (void)address;
  if (read) {
    prepare_measurement(chip);
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
  chip->temperature_attr = attr_init_float("temperature", 25.0f);
  chip->humidity_attr = attr_init_float("humidity", 50.0f);
  chip->response_index = 0;

  const i2c_config_t i2c_config = {
      .address = 0x44,
      .scl = pin_init("SCL", INPUT_PULLUP),
      .sda = pin_init("SDA", INPUT_PULLUP),
      .connect = on_i2c_connect,
      .read = on_i2c_read,
      .write = on_i2c_write,
      .user_data = chip,
  };
  i2c_init(&i2c_config);
}
