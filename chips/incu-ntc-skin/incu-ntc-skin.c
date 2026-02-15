#include "wokwi-api.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  pin_t out;
  uint32_t temperature_attr;
  timer_t timer;
} chip_state_t;

static float ntc_voltage_from_temp(float temp_c) {
  const float vcc = 3.3f;
  const float r_fixed = 10000.0f;
  const float r0 = 10000.0f;
  const float beta = 3950.0f;
  const float t0 = 298.15f;
  const float tk = temp_c + 273.15f;

  float r_ntc = r0 * expf(beta * ((1.0f / tk) - (1.0f / t0)));
  return vcc * (r_fixed / (r_fixed + r_ntc));
}

static void on_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  float temperature = attr_read_float(chip->temperature_attr);
  float voltage = ntc_voltage_from_temp(temperature);
  pin_dac_write(chip->out, voltage);
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->out = pin_init("OUT", ANALOG);
  chip->temperature_attr = attr_init_float("temperature", 36.5f);

  const timer_config_t timer_config = {
      .callback = on_timer,
      .user_data = chip,
  };
  chip->timer = timer_init(&timer_config);
  timer_start(chip->timer, 100000, true);
}
