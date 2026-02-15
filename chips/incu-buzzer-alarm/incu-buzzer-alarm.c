#include "wokwi-api.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  pin_t in;
  timer_t timer;
  buffer_t fb;
  uint32_t width;
  uint32_t height;
  uint64_t last_edge_ns;
  uint32_t *pixels;
} chip_state_t;

static void on_pin_change(void *user_data, pin_t pin, uint32_t value) {
  chip_state_t *chip = (chip_state_t *)user_data;
  (void)pin;
  (void)value;
  chip->last_edge_ns = get_sim_nanos();
}

static void fill_display(chip_state_t *chip, uint32_t rgba_color) {
  uint32_t total = chip->width * chip->height;
  for (uint32_t i = 0; i < total; i++) {
    chip->pixels[i] = rgba_color;
  }
  buffer_write(chip->fb, 0, chip->pixels, total * sizeof(uint32_t));
}

static void on_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  uint64_t now = get_sim_nanos();
  bool active = (now - chip->last_edge_ns) < 120000000ULL;
  fill_display(chip, active ? 0x3030F0FF : 0x202020FF);
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->in = pin_init("IN", INPUT);
  chip->last_edge_ns = 0;
  chip->timer = 0;

  chip->fb = framebuffer_init(&chip->width, &chip->height);
  chip->pixels = malloc(chip->width * chip->height * sizeof(uint32_t));
  fill_display(chip, 0x202020FF);

  const pin_watch_config_t watch = {
      .edge = BOTH,
      .pin_change = on_pin_change,
      .user_data = chip,
  };
  pin_watch(chip->in, &watch);

  const timer_config_t timer_config = {
      .callback = on_timer,
      .user_data = chip,
  };
  chip->timer = timer_init(&timer_config);
  timer_start(chip->timer, 20000, true);
}
