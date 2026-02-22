#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Telemetry Reporter Chip
 *
 * Reads attribute values set via Wokwi chip controls (sliders)
 * and periodically sends them as JSON over a UART TX pin.
 *
 * Connect TX → ESP32 RX2 (GPIO16). The firmware echoes Serial2
 * to USB Serial, which appears on RFC2217 port 4000.
 * The wokwi-bridge.mjs then forwards to the 3D viewer.
 */

typedef struct {
  uint32_t chamber_temp_attr;
  uint32_t skin_temp_attr;
  uint32_t humidity_attr;
  uint32_t fan_rpm_attr;
  uint32_t heater_duty_attr;
  uint32_t door_open_attr;
  uint32_t alarm_attr;
  uint32_t timer_id;
  uart_dev_t uart;
  pin_t door_pin;
  bool door_pin_connected;
} chip_state_t;

static void send_str(chip_state_t *chip, const char *s) {
  uart_write(chip->uart, (uint8_t *)s, strlen(s));
}

static void on_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;

  float chamber = attr_read_float(chip->chamber_temp_attr);
  float skin    = attr_read_float(chip->skin_temp_attr);
  float hum     = attr_read_float(chip->humidity_attr);
  float fan     = attr_read_float(chip->fan_rpm_attr);
  float heater  = attr_read_float(chip->heater_duty_attr);
  // DOOR_IN pin (slide switch): HIGH=closed, LOW=open; fallback to attr slider
  float door;
  if (chip->door_pin_connected) {
    door = (pin_read(chip->door_pin) == LOW) ? 1.0f : 0.0f;
  } else {
    door = attr_read_float(chip->door_open_attr);
  }
  float alarm   = attr_read_float(chip->alarm_attr);

  char buf[192];
  snprintf(buf, sizeof(buf),
    "{\"temp\":%.1f,\"skin\":%.1f,\"hum\":%.1f,\"fan\":%.0f,\"heater\":%.0f,\"door\":%d,\"alarm\":%d}\n",
    chamber, skin, hum, fan, heater,
    (int)door, (int)alarm);

  send_str(chip, buf);
  /* Also print to simulation console for debugging */
  printf("%s", buf);
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  memset(chip, 0, sizeof(chip_state_t));

  chip->chamber_temp_attr = attr_init_float("chamberTemp", 0.0f);
  chip->skin_temp_attr    = attr_init_float("skinTemp", 0.0f);
  chip->humidity_attr     = attr_init_float("humidity", 0.0f);
  chip->fan_rpm_attr      = attr_init_float("fanRpm", 0.0f);
  chip->heater_duty_attr  = attr_init_float("heaterDuty", 0.0f);
  chip->door_open_attr    = attr_init_float("doorOpen", 0.0f);
  chip->alarm_attr        = attr_init_float("alarm", 0.0f);
  chip->door_pin          = pin_init("DOOR_IN", INPUT_PULLUP);
  chip->door_pin_connected = true;

  const uart_config_t uart_cfg = {
    .tx = pin_init("TX", OUTPUT),
    .baud_rate = 115200,
  };
  chip->uart = uart_init(&uart_cfg);

  const timer_config_t timer_cfg = {
    .callback = on_timer,
    .user_data = chip,
  };
  chip->timer_id = timer_init(&timer_cfg);
  timer_start(chip->timer_id, 500000, true);
}
