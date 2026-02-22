#include "wokwi-api.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define FB_WIDTH  64
#define FB_HEIGHT 96

// Colors in 0xAABBGGRR format (Little Endian: A=bits 31-24, B=23-16, G=15-8, R=7-0)
#define COLOR_BG            0xFF30180A  // dark navy background
#define COLOR_BOTTLE_EDGE   0xFFB4A08C  // clear plastic wall (warm gray-blue)
#define COLOR_INSIDE_EMPTY  0xFFF0E1D2  // air inside bottle (light gray-blue)
#define COLOR_WATER         0xFFC87828  // rich blue water
#define COLOR_HIGHLIGHT     0xFFFCF9F5  // reflection on plastic
#define COLOR_CAP           0xFF50821E  // dark teal cap
#define COLOR_CAP_SHINE     0xFF6EA032  // lighter teal for cap highlight
#define COLOR_VAPOR         0xFFFFDCB4  // light blue vapor particles
#define COLOR_ON            0xFF00BB00  // green  (on)
#define COLOR_OFF           0xFF0000CC  // red    (off)

typedef struct {
  uint32_t water_level_attr;
  uint32_t address_attr;
  uint32_t speed_mode_attr;
  uint8_t duty_cycle;
  uint8_t read_index;
  uint8_t write_index;
  uint8_t write_buf[4];
  pin_t duty_override;
  pin_t override_en;
  pin_t humidity_out;
  uint32_t last_water_attr;
  float water_level_sim;
  float humidity_boost_pct;

  buffer_t fb;
  uint32_t pixels[FB_WIDTH * FB_HEIGHT];
  uint32_t anim_frame;
} chip_state_t;

static void draw_rect(chip_state_t *chip, int x, int y, int w, int h, uint32_t color) {
  for (int i = 0; i < h; i++) {
    for (int j = 0; j < w; j++) {
      int px = x + j;
      int py = y + i;
      if (px >= 0 && px < FB_WIDTH && py >= 0 && py < FB_HEIGHT) {
        chip->pixels[py * FB_WIDTH + px] = color;
      }
    }
  }
}

static float clampf(float x, float min_v, float max_v) {
  if (x < min_v) return min_v;
  if (x > max_v) return max_v;
  return x;
}

static int read_speed_mode(chip_state_t *chip) {
  int mode = (int)attr_read(chip->speed_mode_attr);
  if (mode < 0) mode = 0;
  if (mode > 2) mode = 2;
  return mode;
}

static float speed_mode_factor(int mode) {
  // 0=normal, 1=rapido, 2=acelerado (exponential progression)
  if (mode == 1) return 5.0f;
  if (mode == 2) return 25.0f;
  return 1.0f;
}

static uint8_t effective_duty_cycle(chip_state_t *chip) {
  if (pin_read(chip->override_en) == HIGH) {
    float v = clampf(pin_adc_read(chip->duty_override), 0.0f, 3.3f);
    int duty = (int)((v / 3.3f) * 95.0f + 0.5f);
    if (duty < 0) duty = 0;
    if (duty > 95) duty = 95;
    return (uint8_t)duty;
  }
  return chip->duty_cycle;
}

static void on_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  chip->anim_frame++;
  uint8_t duty_cycle = effective_duty_cycle(chip);
  int speed_mode = read_speed_mode(chip);
  float mode_factor = speed_mode_factor(speed_mode);

  uint32_t water_attr = attr_read(chip->water_level_attr);
  if (water_attr > 100) water_attr = 100;
  if (water_attr != chip->last_water_attr) {
    // Manual refill/drain via slider updates simulated tank level.
    chip->last_water_attr = water_attr;
    chip->water_level_sim = (float)water_attr;
  }

  const float dt_s = 0.05f; // timer period: 50ms
  if (duty_cycle > 0 && chip->water_level_sim > 0.0f) {
    // Non-linear response to duty (faster visible changes in high-speed modes).
    const float max_use_pct_per_s = 0.30f;
    float duty_norm = (float)duty_cycle / 95.0f;
    float duty_intensity = duty_norm * (2.0f - duty_norm); // 0..1, convex
    float use_rate = duty_intensity * max_use_pct_per_s;
    chip->water_level_sim -= use_rate * dt_s * mode_factor;
    if (chip->water_level_sim < 0.0f) chip->water_level_sim = 0.0f;
  }

  float target_boost = 0.0f;
  if (chip->water_level_sim > 0.0f && duty_cycle > 0) {
    float water_factor = clampf(chip->water_level_sim / 100.0f, 0.0f, 1.0f);
    float duty_norm = (float)duty_cycle / 95.0f;
    float duty_intensity = duty_norm * (2.0f - duty_norm);
    target_boost = duty_intensity * 35.0f * water_factor;
  }
  float boost_alpha = 0.10f * mode_factor;
  if (boost_alpha > 0.98f) boost_alpha = 0.98f;
  chip->humidity_boost_pct += (target_boost - chip->humidity_boost_pct) * boost_alpha;
  if (chip->humidity_boost_pct < 0.05f) chip->humidity_boost_pct = 0.0f;
  float humidity_out_v = (chip->humidity_boost_pct / 100.0f) * 3.3f;
  pin_dac_write(chip->humidity_out, clampf(humidity_out_v, 0.0f, 3.3f));

  uint8_t water_level = (uint8_t)(chip->water_level_sim + 0.5f);
  if (water_level > 100) water_level = 100;

  // Clear background
  for (int i = 0; i < FB_WIDTH * FB_HEIGHT; i++) {
    chip->pixels[i] = COLOR_BG;
  }

  // =============================================
  // CAP: x=21..42 (22px), y=1..7 (7px) — now at TOP
  // =============================================
  draw_rect(chip, 21,  1, 22, 7, COLOR_CAP);
  draw_rect(chip, 23,  2,  3, 4, COLOR_CAP_SHINE);

  // =============================================
  // NECK: x=24..39 (16px), y=8..21 (14px)
  // =============================================
  draw_rect(chip, 24,  8, 16, 14, COLOR_BOTTLE_EDGE);
  draw_rect(chip, 26,  8, 12, 14, COLOR_INSIDE_EMPTY);

  // =============================================
  // SHOULDER: y=22..33 (12 rows)
  // Widens from neck (x=24..39) to body (x=14..50)
  // =============================================
  for (int row = 0; row < 12; row++) {
    int yr    = 22 + row;
    int left  = 24 - (row * 10) / 11;
    int right = 39 + row;
    draw_rect(chip, left, yr, right - left + 1, 1, COLOR_BOTTLE_EDGE);
    int il = left + 2;
    int ir = right - 2;
    if (il <= ir) {
      draw_rect(chip, il, yr, ir - il + 1, 1, COLOR_INSIDE_EMPTY);
    }
  }

  // =============================================
  // BOTTLE BODY: x=14..50 (37px), y=34..93 (60px)
  // Inner: x=16..48, y=34..91  (33x58 px)
  // =============================================
  draw_rect(chip, 14, 34, 37, 60, COLOR_BOTTLE_EDGE);
  draw_rect(chip, 16, 34, 33, 58, COLOR_INSIDE_EMPTY);

  // Water fills from the BOTTOM of the body upward
  int body_top = 34;
  int inner_h  = 58;
  int water_h  = (water_level * inner_h) / 100;
  int water_y_top = body_top + (inner_h - water_h);
  int water_y_bottom = body_top + inner_h - 1;
  if (water_h > 0) {
    draw_rect(chip, 16, water_y_top, 33, water_h, COLOR_WATER);
  }

  // Internal vertical cylinder (same color as cap), from cap area to near bottom.
  draw_rect(chip, 30, 7, 5, 82, COLOR_CAP);

  // Reflection highlight on the left side of the body
  draw_rect(chip, 18, 34, 2, 58, COLOR_HIGHLIGHT);

  // =============================================
  // VAPOR: particles around the center of current water volume.
  // As water level drops, this center moves downward automatically.
  // =============================================
  if (duty_cycle > 0 && water_h > 0) {
    int num_particles = (duty_cycle / 15) + 1;
    if (num_particles > 4) num_particles = 4;
    int water_center_x = 16 + (33 / 2);
    int water_center_y = water_y_top + (water_h / 2);
    for (int i = 0; i < num_particles; i++) {
      int x_jitter = (int)((chip->anim_frame + i * 5) % 5) - 2;
      int y_jitter = (int)((chip->anim_frame + i * 3) % 5) - 2;
      int px = water_center_x - 1 + (i - (num_particles / 2)) * 3 + x_jitter;
      int py = water_center_y + y_jitter;
      if (px < 16) px = 16;
      if (px > 46) px = 46; // 46..48 keeps 3px particle inside water body
      if (py < water_y_top) py = water_y_top;
      if (py > water_y_bottom - 1) py = water_y_bottom - 1;
      draw_rect(chip, px, py, 3, 2, COLOR_VAPOR);
    }
  }

  // =============================================
  // STATUS DOT: top-right corner (r=3)
  // Green = humidifier ON (duty_cycle > 0), Red = OFF
  // =============================================
  uint32_t dot = (duty_cycle > 0 && water_level > 0) ? COLOR_ON : COLOR_OFF;
  for (int dy = -3; dy <= 3; dy++)
    for (int dx = -3; dx <= 3; dx++)
      if (dx*dx + dy*dy <= 9) {
        int px = 54 + dx, py = 10 + dy;
        if (px >= 0 && px < FB_WIDTH && py >= 0 && py < FB_HEIGHT)
          chip->pixels[py * FB_WIDTH + px] = dot;
      }

  buffer_write(chip->fb, 0, chip->pixels, sizeof(chip->pixels));
}

static bool on_i2c_connect(void *user_data, uint32_t address, bool read) {
  chip_state_t *chip = (chip_state_t *)user_data;
  (void)address;
  if (read) {
    chip->read_index = 0;
  } else {
    chip->write_index = 0;
  }
  return true;
}

static uint8_t on_i2c_read(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  uint8_t water_level = (uint8_t)(chip->water_level_sim + 0.5f);
  if (water_level > 100) water_level = 100;
  uint8_t duty_cycle = effective_duty_cycle(chip);
  switch (chip->read_index++) {
    case 0:
      return duty_cycle;
    case 1:
      return water_level;
    case 2:
      return chip->water_level_sim <= 0.01f ? 0x02 : 0x00; /* 0x02 = empty tank */
    default:
      return 0xFF;
  }
}

static bool on_i2c_write(void *user_data, uint8_t data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  if (chip->write_index < sizeof(chip->write_buf)) {
    chip->write_buf[chip->write_index++] = data;
  }

  if (chip->write_index == 1) {
    chip->duty_cycle = data > 95 ? 95 : data;
  } else if (chip->write_index >= 2 && chip->write_buf[0] == 0x01) {
    chip->duty_cycle = chip->write_buf[1] > 95 ? 95 : chip->write_buf[1];
  }
  return true;
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->water_level_attr = attr_init("waterLevel", 80);
  chip->address_attr = attr_init("address", 0x02);
  chip->speed_mode_attr = attr_init("speedMode", 0);
  chip->duty_cycle = 0;
  chip->read_index = 0;
  chip->write_index = 0;
  chip->anim_frame = 0;
  chip->duty_override = pin_init("DUTY_OVERRIDE", ANALOG);
  chip->override_en = pin_init("OVERRIDE_EN", INPUT_PULLDOWN);
  chip->humidity_out = pin_init("HUMIDITY_OUT", ANALOG);
  chip->last_water_attr = attr_read(chip->water_level_attr);
  if (chip->last_water_attr > 100) chip->last_water_attr = 100;
  chip->water_level_sim = (float)chip->last_water_attr;
  chip->humidity_boost_pct = 0.0f;

  uint32_t fb_w = 0, fb_h = 0;
  chip->fb = framebuffer_init(&fb_w, &fb_h);

  // Draw initial frame immediately so chip is not blank on load
  on_timer(chip);

  const timer_config_t timer_config = {
    .user_data = chip,
    .callback = on_timer,
  };
  timer_t timer = timer_init(&timer_config);
  timer_start(timer, 50000, true); // 20 FPS (50ms)

  uint32_t address = attr_read(chip->address_attr);
  // IncuNest humidifier uses 0x02 (reserved range but valid for this device)
  if (address == 0 || address > 0x77) {
    address = 0x02;
  }

  const i2c_config_t i2c_config = {
      .address = address,
      .scl = pin_init("SCL", INPUT_PULLUP),
      .sda = pin_init("SDA", INPUT_PULLUP),
      .connect = on_i2c_connect,
      .read = on_i2c_read,
      .write = on_i2c_write,
      .user_data = chip,
  };
  i2c_init(&i2c_config);
}
