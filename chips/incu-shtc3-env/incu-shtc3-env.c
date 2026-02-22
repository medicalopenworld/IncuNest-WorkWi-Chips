#include "wokwi-api.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  uint32_t temperature_attr;
  uint32_t humidity_attr;
  uint8_t cmd_buf[2];
  uint8_t cmd_len;
  uint8_t tx[6];
  uint8_t tx_len;
  uint8_t tx_index;
  bool asleep;
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

static uint16_t clamp_u16(float value) {
  if (value < 0.0f) return 0;
  if (value > 65535.0f) return 65535;
  return (uint16_t)(value + 0.5f);
}

static void prepare_measurement(chip_state_t *chip, bool rh_first) {
  float t = attr_read_float(chip->temperature_attr);
  float h = attr_read_float(chip->humidity_attr);
  uint16_t t_raw = clamp_u16(((t + 45.0f) * 65536.0f) / 175.0f);
  uint16_t h_raw = clamp_u16((h * 65536.0f) / 100.0f);

  if (rh_first) {
    chip->tx[0] = (uint8_t)(h_raw >> 8);
    chip->tx[1] = (uint8_t)h_raw;
    chip->tx[2] = crc8(chip->tx, 2);
    chip->tx[3] = (uint8_t)(t_raw >> 8);
    chip->tx[4] = (uint8_t)t_raw;
    chip->tx[5] = crc8(chip->tx + 3, 2);
  } else {
    chip->tx[0] = (uint8_t)(t_raw >> 8);
    chip->tx[1] = (uint8_t)t_raw;
    chip->tx[2] = crc8(chip->tx, 2);
    chip->tx[3] = (uint8_t)(h_raw >> 8);
    chip->tx[4] = (uint8_t)h_raw;
    chip->tx[5] = crc8(chip->tx + 3, 2);
  }
  chip->tx_len = 6;
  chip->tx_index = 0;
}

static void prepare_id(chip_state_t *chip) {
  const uint16_t id = 0x0807;
  chip->tx[0] = (uint8_t)(id >> 8);
  chip->tx[1] = (uint8_t)id;
  chip->tx[2] = crc8(chip->tx, 2);
  chip->tx_len = 3;
  chip->tx_index = 0;
}

static bool handle_command(chip_state_t *chip, uint16_t cmd) {
  if (cmd == 0x3517) {
    chip->asleep = false;
    return true;
  }
  if (chip->asleep) {
    return false;
  }
  switch (cmd) {
    case 0xB098:
      chip->asleep = true;
      return true;
    case 0x7866:
    case 0x7CA2:
      prepare_measurement(chip, false);
      return true;
    case 0x609C:
    case 0x6458:
      prepare_measurement(chip, true);
      return true;
    case 0xEFC8:
      prepare_id(chip);
      return true;
    case 0x805D:
      chip->asleep = false;
      chip->tx_len = 0;
      chip->tx_index = 0;
      return true;
    default:
      chip->tx_len = 0;
      chip->tx_index = 0;
      return true;
  }
}

static bool on_i2c_connect(void *user_data, uint32_t address, bool read) {
  chip_state_t *chip = (chip_state_t *)user_data;
  if (address != 0x70) return false;
  if (read && chip->asleep) return false;
  if (!read) chip->cmd_len = 0;
  return true;
}

static uint8_t on_i2c_read(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  if (chip->tx_index < chip->tx_len) {
    return chip->tx[chip->tx_index++];
  }
  return 0xFF;
}

static bool on_i2c_write(void *user_data, uint8_t data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  chip->cmd_buf[chip->cmd_len++] = data;
  if (chip->cmd_len < 2) return true;
  uint16_t cmd = ((uint16_t)chip->cmd_buf[0] << 8) | chip->cmd_buf[1];
  chip->cmd_len = 0;
  return handle_command(chip, cmd);
}

static void on_i2c_disconnect(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  chip->cmd_len = 0;
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->temperature_attr = attr_init_float("temperature", 25.0f);
  chip->humidity_attr = attr_init_float("humidity", 45.0f);
  chip->cmd_len = 0;
  chip->tx_len = 0;
  chip->tx_index = 0;
  chip->asleep = false;

  const i2c_config_t i2c_config = {
      .address = 0x70,
      .scl = pin_init("SCL", INPUT_PULLUP),
      .sda = pin_init("SDA", INPUT_PULLUP),
      .connect = on_i2c_connect,
      .read = on_i2c_read,
      .write = on_i2c_write,
      .disconnect = on_i2c_disconnect,
      .user_data = chip,
  };
  i2c_init(&i2c_config);
}
