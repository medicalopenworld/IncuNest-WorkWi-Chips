#include "wokwi-api.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  pin_t pwm_in;
  pin_t temp_fb;
  uint32_t ambient_temp_attr;
  uint32_t thermal_mass_attr;
  timer_t timer;
  float duty_estimate;
  float chamber_temp;
  uint64_t last_ns;
} chip_state_t;

static float clampf(float x, float min_v, float max_v) {
  if (x < min_v) return min_v;
  if (x > max_v) return max_v;
  return x;
}

static void on_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  uint64_t now = get_sim_nanos();
  float dt = (chip->last_ns == 0) ? 0.005f : (float)(now - chip->last_ns) / 1000000000.0f;
  chip->last_ns = now;

  float sample = pin_read(chip->pwm_in) == HIGH ? 1.0f : 0.0f;
  chip->duty_estimate = chip->duty_estimate * 0.95f + sample * 0.05f;

  float ambient = attr_read_float(chip->ambient_temp_attr);
  float thermal_mass = attr_read_float(chip->thermal_mass_attr);
  if (thermal_mass < 1.0f) thermal_mass = 1.0f;

  float heater_gain = 120.0f * chip->duty_estimate;
  float loss = 4.0f * (chip->chamber_temp - ambient);
  float net_power = heater_gain - loss;
  chip->chamber_temp += (net_power / (thermal_mass * 45.0f)) * dt;

  float out_v = clampf((chip->chamber_temp / 60.0f) * 5.0f, 0.0f, 5.0f);
  pin_dac_write(chip->temp_fb, out_v);
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->pwm_in = pin_init("PWM_IN", INPUT);
  chip->temp_fb = pin_init("TEMP_FB", ANALOG);
  chip->ambient_temp_attr = attr_init_float("ambientTemp", 25.0f);
  chip->thermal_mass_attr = attr_init_float("thermalMass", 2.0f);
  chip->duty_estimate = 0.0f;
  chip->chamber_temp = attr_read_float(chip->ambient_temp_attr);
  chip->last_ns = 0;

  const timer_config_t timer_config = {
      .callback = on_timer,
      .user_data = chip,
  };
  chip->timer = timer_init(&timer_config);
  timer_start(chip->timer, 5000, true);
}
