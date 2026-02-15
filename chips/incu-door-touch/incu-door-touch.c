#include "wokwi-api.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  pin_t out;
  pin_t sel;
  uint32_t door_open_attr;
  timer_t timer;
} chip_state_t;

static void on_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  uint32_t door_open = attr_read(chip->door_open_attr);
  bool selected = pin_read(chip->sel) == HIGH;
  float voltage = 0.4f;

  if (selected) {
    voltage = door_open ? 0.8f : 3.2f;
  } else {
    voltage = door_open ? 0.5f : 1.3f;
  }

  pin_dac_write(chip->out, voltage);
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->out = pin_init("OUT", ANALOG);
  chip->sel = pin_init("SEL", INPUT);
  chip->door_open_attr = attr_init("doorOpen", 0);
  chip->timer = 0;

  const timer_config_t timer_config = {
      .callback = on_timer,
      .user_data = chip,
  };
  chip->timer = timer_init(&timer_config);
  timer_start(chip->timer, 50000, true);
}
