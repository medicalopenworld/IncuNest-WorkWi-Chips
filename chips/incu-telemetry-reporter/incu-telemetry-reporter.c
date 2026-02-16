#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Telemetry Reporter Chip
 *
 * Reads attribute values set by the user in Wokwi's chip controls
 * and periodically prints them as JSON over the UART TX pin.
 * The wokwi-bridge.mjs picks these up and forwards to the 3D viewer.
 *
 * Connect TX to the ESP32 RX2 (or any available UART RX).
 * Alternatively, connect TX to pin that the firmware echoes to Serial.
 *
 * For simplicity, this chip uses printf() which writes to the
 * simulation console (visible in VS Code Wokwi terminal and RFC2217).
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
} chip_state_t;

static void on_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;

  float chamber = attr_read_float(chip->chamber_temp_attr);
  float skin = attr_read_float(chip->skin_temp_attr);
  float hum = attr_read_float(chip->humidity_attr);
  float fan = attr_read_float(chip->fan_rpm_attr);
  float heater = attr_read_float(chip->heater_duty_attr);
  float door = attr_read_float(chip->door_open_attr);
  float alarm = attr_read_float(chip->alarm_attr);

  /* JSON output — picked up by wokwi-bridge.mjs via RFC2217 serial */
  printf("{\"temp\":%.1f,\"skin\":%.1f,\"hum\":%.1f,\"fan\":%.0f,\"heater\":%.0f,\"door\":%d,\"alarm\":%d}\n",
         chamber, skin, hum, fan, heater,
         (int)door, (int)alarm);
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));

  chip->chamber_temp_attr = attr_init_float("chamberTemp", 34.2f);
  chip->skin_temp_attr    = attr_init_float("skinTemp", 36.6f);
  chip->humidity_attr     = attr_init_float("humidity", 58.0f);
  chip->fan_rpm_attr      = attr_init_float("fanRpm", 1200.0f);
  chip->heater_duty_attr  = attr_init_float("heaterDuty", 42.0f);
  chip->door_open_attr    = attr_init_float("doorOpen", 0.0f);
  chip->alarm_attr        = attr_init_float("alarm", 0.0f);

  const timer_config_t timer_cfg = {
    .callback = on_timer,
    .user_data = chip,
  };
  chip->timer_id = timer_init(&timer_cfg);
  /* Report every 500ms */
  timer_start(chip->timer_id, 500000, true);
}
