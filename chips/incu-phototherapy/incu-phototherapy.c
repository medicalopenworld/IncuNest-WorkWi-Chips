#include "wokwi-api.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define COLOR_BG_OFF 0x111111FF
#define COLOR_BG_ON  0x09152BFF
#define COLOR_TEXT   0xE6E6E6FF
#define COLOR_BLUE   0x0066FFFF
#define COLOR_DIM    0x2C3A4DFF

typedef struct {
  char c;
  uint8_t rows[5];
} glyph_t;

static const glyph_t FONT[] = {
    {' ', {0, 0, 0, 0, 0}}, {'%', {5, 1, 2, 4, 5}},
    {'0', {7, 5, 5, 5, 7}}, {'1', {2, 6, 2, 2, 7}}, {'2', {7, 1, 7, 4, 7}},
    {'3', {7, 1, 7, 1, 7}}, {'4', {5, 5, 7, 1, 1}}, {'5', {7, 4, 7, 1, 7}},
    {'6', {7, 4, 7, 5, 7}}, {'7', {7, 1, 1, 1, 1}}, {'8', {7, 5, 7, 5, 7}},
    {'9', {7, 5, 7, 1, 7}}, {'F', {7, 4, 7, 4, 4}}, {'H', {5, 5, 7, 5, 5}},
    {'N', {5, 7, 7, 7, 5}}, {'O', {7, 5, 5, 5, 7}}, {'P', {7, 5, 7, 4, 4}},
    {'T', {7, 2, 2, 2, 2}},
};

typedef struct {
  pin_t pwm_in;
  pin_t en;
  timer_t timer;
  buffer_t fb;
  uint32_t width;
  uint32_t height;
  uint32_t *pixels;
  uint64_t last_rise_ns;
  uint64_t last_high_ns;
  uint64_t last_pwm_edge_ns;
  uint64_t last_update_ns;
  uint64_t on_time_ns;
  uint16_t duty_permille;
  uint16_t shown_permille;
  bool en_level;
  bool active;
  bool shown_active;
  bool dirty;
} chip_state_t;

static void fill_rect(chip_state_t *chip, int x, int y, int w, int h, uint32_t color) {
  for (int yy = 0; yy < h; yy++) {
    int py = y + yy;
    if (py < 0 || py >= (int)chip->height) continue;
    for (int xx = 0; xx < w; xx++) {
      int px = x + xx;
      if (px >= 0 && px < (int)chip->width) {
        chip->pixels[py * chip->width + px] = color;
      }
    }
  }
}

static void fill_circle(chip_state_t *chip, int cx, int cy, int r, uint32_t color) {
  for (int y = -r; y <= r; y++) {
    for (int x = -r; x <= r; x++) {
      if (x * x + y * y <= r * r) {
        int px = cx + x;
        int py = cy + y;
        if (px >= 0 && px < (int)chip->width && py >= 0 && py < (int)chip->height) {
          chip->pixels[py * chip->width + px] = color;
        }
      }
    }
  }
}

static const uint8_t *glyph_rows(char c) {
  for (uint32_t i = 0; i < sizeof(FONT) / sizeof(FONT[0]); i++) {
    if (FONT[i].c == c) return FONT[i].rows;
  }
  return FONT[0].rows;
}

static void draw_char(chip_state_t *chip, int x, int y, char c, uint32_t color, int scale) {
  const uint8_t *rows = glyph_rows(c);
  for (int ry = 0; ry < 5; ry++) {
    for (int rx = 0; rx < 3; rx++) {
      if (rows[ry] & (1 << (2 - rx))) {
        fill_rect(chip, x + rx * scale, y + ry * scale, scale, scale, color);
      }
    }
  }
}

static void draw_text(chip_state_t *chip, int x, int y, const char *text, uint32_t color, int scale) {
  int px = x;
  while (*text) {
    draw_char(chip, px, y, *text, color, scale);
    px += 4 * scale;
    text++;
  }
}

static int u32_to_str(uint32_t value, char *out) {
  char tmp[10];
  int len = 0;
  do {
    tmp[len++] = (char)('0' + (value % 10));
    value /= 10;
  } while (value && len < 10);
  for (int i = 0; i < len; i++) out[i] = tmp[len - 1 - i];
  out[len] = '\0';
  return len;
}

static void render(chip_state_t *chip) {
  const uint32_t bg = chip->active ? COLOR_BG_ON : COLOR_BG_OFF;
  const uint32_t led = chip->active ? COLOR_BLUE : COLOR_DIM;
  const uint32_t indicator = chip->active ? COLOR_BLUE : COLOR_TEXT;
  uint32_t total = chip->width * chip->height;
  for (uint32_t i = 0; i < total; i++) chip->pixels[i] = bg;

  draw_text(chip, 10, 8, "PHOTO", COLOR_TEXT, 2);
  fill_rect(chip, 8, 24, 28, 36, COLOR_DIM);
  fill_circle(chip, 22, 42, 10, led);
  draw_text(chip, 54, 24, chip->active ? "ON" : "OFF", indicator, 4);

  char duty_text[6];
  uint32_t duty_pct = (chip->duty_permille + 5U) / 10U;
  int len = u32_to_str(duty_pct, duty_text);
  duty_text[len++] = '%';
  duty_text[len] = '\0';
  int duty_x = (int)(chip->width - (uint32_t)(len * 12)) / 2;
  draw_text(chip, duty_x, 60, duty_text, COLOR_TEXT, 3);

  buffer_write(chip->fb, 0, chip->pixels, total * sizeof(uint32_t));
  chip->shown_active = chip->active;
  chip->shown_permille = chip->duty_permille;
  chip->dirty = false;
}

static void on_pin_change(void *user_data, pin_t pin, uint32_t value) {
  chip_state_t *chip = (chip_state_t *)user_data;
  uint64_t now = get_sim_nanos();
  if (pin == chip->en) {
    bool en_now = value == HIGH;
    if (en_now != chip->en_level) {
      chip->en_level = en_now;
      chip->dirty = true;
    }
    return;
  }

  chip->last_pwm_edge_ns = now;
  if (value == HIGH) {
    if (chip->last_rise_ns && now > chip->last_rise_ns) {
      uint64_t period = now - chip->last_rise_ns;
      uint64_t high = chip->last_high_ns > period ? period : chip->last_high_ns;
      uint16_t new_permille = (uint16_t)((high * 1000ULL + period / 2ULL) / period);
      if (new_permille > 1000) new_permille = 1000;
      if (new_permille != chip->duty_permille) {
        chip->duty_permille = new_permille;
        chip->dirty = true;
      }
    }
    chip->last_rise_ns = now;
  } else if (chip->last_rise_ns && now > chip->last_rise_ns) {
    chip->last_high_ns = now - chip->last_rise_ns;
  }
}

static void on_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  uint64_t now = get_sim_nanos();
  if (chip->last_update_ns && chip->active) chip->on_time_ns += (now - chip->last_update_ns);
  chip->last_update_ns = now;

  if (chip->last_pwm_edge_ns == 0 || (now - chip->last_pwm_edge_ns) > 3000000ULL) {
    if (chip->duty_permille != 0) {
      chip->duty_permille = 0;
      chip->dirty = true;
    }
  }

  bool new_active = chip->en_level && chip->duty_permille > 0;
  if (new_active != chip->active) {
    chip->active = new_active;
    chip->dirty = true;
  }
  if (chip->active != chip->shown_active || chip->duty_permille != chip->shown_permille) chip->dirty = true;
  if (chip->dirty) render(chip);
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->pwm_in = pin_init("PWM_IN", INPUT);
  chip->en = pin_init("EN", INPUT);
  chip->timer = 0;
  chip->fb = framebuffer_init(&chip->width, &chip->height);
  chip->pixels = malloc(chip->width * chip->height * sizeof(uint32_t));
  chip->last_rise_ns = 0;
  chip->last_high_ns = 0;
  chip->last_pwm_edge_ns = 0;
  chip->last_update_ns = 0;
  chip->on_time_ns = 0;
  chip->duty_permille = 0;
  chip->shown_permille = 0;
  chip->en_level = pin_read(chip->en) == HIGH;
  chip->active = false;
  chip->shown_active = true;
  chip->dirty = true;

  const pin_watch_config_t watch = {.edge = BOTH, .pin_change = on_pin_change, .user_data = chip};
  pin_watch(chip->pwm_in, &watch);
  pin_watch(chip->en, &watch);

  const timer_config_t timer_cfg = {.callback = on_timer, .user_data = chip};
  chip->timer = timer_init(&timer_cfg);
  timer_start(chip->timer, 20000, true);
  on_timer(chip);
}
