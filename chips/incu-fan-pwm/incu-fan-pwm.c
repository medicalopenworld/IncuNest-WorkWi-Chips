#include "wokwi-api.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  pin_t pwm_in;
  pin_t tach_out;
  pin_t en;
  uint32_t max_rpm_attr;
  timer_t timer;
  float duty_estimate;
  float rpm;
  bool tach_level;
  uint64_t last_toggle_ns;
} chip_state_t;

static void on_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  uint64_t now = get_sim_nanos();

  float pwm_sample = (pin_read(chip->pwm_in) == HIGH) ? 1.0f : 0.0f;
  chip->duty_estimate = chip->duty_estimate * 0.95f + pwm_sample * 0.05f;
  bool enabled = (pin_read(chip->en) == HIGH);
  float max_rpm = (float)attr_read(chip->max_rpm_attr);

  float target_rpm = enabled ? (200.0f + chip->duty_estimate * (max_rpm - 200.0f)) : 0.0f;
  chip->rpm += (target_rpm - chip->rpm) * 0.08f;
  if (chip->rpm < 50.0f) {
    chip->rpm = 0.0f;
    chip->tach_level = false;
    pin_write(chip->tach_out, LOW);
    chip->last_toggle_ns = now;
    return;
  }

  /* 2 tach pulses per revolution, toggle every half pulse period */
  uint64_t toggle_interval_ns = (uint64_t)(60000000000.0f / (chip->rpm * 4.0f));
  if (toggle_interval_ns < 10000) {
    toggle_interval_ns = 10000;
  }

  if (now - chip->last_toggle_ns >= toggle_interval_ns) {
    chip->last_toggle_ns = now;
    chip->tach_level = !chip->tach_level;
    pin_write(chip->tach_out, chip->tach_level ? HIGH : LOW);
  }
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->pwm_in = pin_init("PWM_IN", INPUT);
  chip->tach_out = pin_init("TACH_OUT", OUTPUT_LOW);
  chip->en = pin_init("EN", INPUT_PULLUP);
  chip->max_rpm_attr = attr_init("maxRpm", 3200);
  chip->timer = 0;
  chip->duty_estimate = 0.0f;
  chip->rpm = 0.0f;
  chip->tach_level = false;
  chip->last_toggle_ns = 0;

  const timer_config_t timer_config = {
      .callback = on_timer,
      .user_data = chip,
  };
  chip->timer = timer_init(&timer_config);
  timer_start(chip->timer, 2000, true);
}
