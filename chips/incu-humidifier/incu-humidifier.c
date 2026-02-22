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
  uint8_t duty_cycle;
  uint8_t read_index;
  uint8_t write_index;
  uint8_t write_buf[4];

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

static void on_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  chip->anim_frame++;

  uint8_t water_level = (uint8_t)attr_read(chip->water_level_attr);
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
  if (water_h > 0) {
    int wy = body_top + (inner_h - water_h);
    draw_rect(chip, 16, wy, 33, water_h, COLOR_WATER);
  }

  // Reflection highlight on the left side of the body
  draw_rect(chip, 18, 34, 2, 58, COLOR_HIGHLIGHT);

  // =============================================
  // VAPOR: particles rising from bottle BOTTOM (y > 93)
  // (bottle is upright now, vapor exits the bottom opening)
  // =============================================
  if (chip->duty_cycle > 0 && water_level > 0) {
    int num_particles = (chip->duty_cycle / 15) + 1;
    if (num_particles > 4) num_particles = 4;
    for (int i = 0; i < num_particles; i++) {
      int px = 18 + (i * 9) + ((chip->anim_frame + i * 5) % 7) - 2;
      int py = 94 + (int)((chip->anim_frame + i * 3) % 3);
      if (py < FB_HEIGHT)
        draw_rect(chip, px, py, 3, 2, COLOR_VAPOR);
    }
  }

  // =============================================
  // STATUS DOT: top-right corner (r=3)
  // Green = humidifier ON (duty_cycle > 0), Red = OFF
  // =============================================
  uint32_t dot = (chip->duty_cycle > 0) ? COLOR_ON : COLOR_OFF;
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
  uint8_t water_level = (uint8_t)attr_read(chip->water_level_attr);
  switch (chip->read_index++) {
    case 0:
      return chip->duty_cycle;
    case 1:
      return water_level;
    case 2:
      return water_level == 0 ? 0x02 : 0x00; /* 0x02 = empty tank */
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
  chip->address_attr = attr_init("address", 0x42);
  chip->duty_cycle = 0;
  chip->read_index = 0;
  chip->write_index = 0;
  chip->anim_frame = 0;

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
  if (address < 0x08 || address > 0x77) {
    address = 0x42;
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
