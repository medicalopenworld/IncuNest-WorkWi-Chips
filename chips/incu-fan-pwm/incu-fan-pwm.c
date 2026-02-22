#include "wokwi-api.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define FB_WIDTH  64
#define FB_HEIGHT 64

// Colors in 0xAABBGGRR format (Little Endian RGBA8888)
#define COLOR_BG    0xFF30180A  // dark navy background
#define COLOR_FRAME 0xFF505050  // outer ring
#define COLOR_BLADE 0xFFDDDDDD  // blade light gray
#define COLOR_HUB   0xFF606060  // hub gray
#define COLOR_ON    0xFF00BB00  // green (on)
#define COLOR_OFF   0xFF0000CC  // red   (off)

// Sine LUT: 0-255 -> -127..127
static const int8_t sin_lut[256] = {
    0,3,6,9,12,15,18,21,24,28,31,34,37,40,43,46,
    48,51,54,57,60,63,65,68,71,73,76,78,81,83,85,88,
    90,92,94,96,98,100,102,104,106,108,109,111,112,114,115,117,
    118,119,120,121,122,123,124,124,125,126,126,127,127,127,127,127,
    127,127,127,127,127,127,126,126,125,124,124,123,122,121,120,119,
    118,117,115,114,112,111,109,108,106,104,102,100,98,96,94,92,
    90,88,85,83,81,78,76,73,71,68,65,63,60,57,54,51,
    48,46,43,40,37,34,31,28,24,21,18,15,12,9,6,3,
    0,-3,-6,-9,-12,-15,-18,-21,-24,-28,-31,-34,-37,-40,-43,-46,
    -48,-51,-54,-57,-60,-63,-65,-68,-71,-73,-76,-78,-81,-83,-85,-88,
    -90,-92,-94,-96,-98,-100,-102,-104,-106,-108,-109,-111,-112,-114,-115,-117,
    -118,-119,-120,-121,-122,-123,-124,-124,-125,-126,-126,-127,-127,-127,-127,-127,
    -127,-127,-127,-127,-127,-127,-126,-126,-125,-124,-124,-123,-122,-121,-120,-119,
    -118,-117,-115,-114,-112,-111,-109,-108,-106,-104,-102,-100,-98,-96,-94,-92,
    -90,-88,-85,-83,-81,-78,-76,-73,-71,-68,-65,-63,-60,-57,-54,-51,
    -48,-46,-43,-40,-37,-34,-31,-28,-24,-21,-18,-15,-12,-9,-6,-3
};
#define SIN(a) sin_lut[(uint8_t)(a)]
#define COS(a) sin_lut[(uint8_t)((a)+64)]

typedef struct {
  pin_t pwm_in;
  pin_t tach_out;
  pin_t en;
  uint32_t max_rpm_attr;
  timer_t timer;
  timer_t display_timer;
  float duty_estimate;
  float rpm;
  bool tach_level;
  uint64_t last_toggle_ns;

  buffer_t fb;
  uint32_t pixels[FB_WIDTH * FB_HEIGHT];
  uint32_t angle; // 0-255, wraps
} chip_state_t;

static void set_px(chip_state_t *chip, int x, int y, uint32_t col) {
  if ((unsigned)x < FB_WIDTH && (unsigned)y < FB_HEIGHT)
    chip->pixels[y * FB_WIDTH + x] = col;
}

static void draw_line(chip_state_t *chip, int x0, int y0, int x1, int y1, uint32_t col) {
  int dx = abs(x1-x0), sx = x0<x1 ? 1:-1;
  int dy = -abs(y1-y0), sy = y0<y1 ? 1:-1;
  int err = dx+dy;
  for(;;) {
    set_px(chip, x0, y0, col);
    if (x0==x1 && y0==y1) break;
    int e2 = 2*err;
    if (e2>=dy){err+=dy; x0+=sx;}
    if (e2<=dx){err+=dx; y0+=sy;}
  }
}

static void draw_circle(chip_state_t *chip, int xc, int yc, int r, uint32_t col) {
  int x=0, y=r, d=3-2*r;
  while (y>=x) {
    set_px(chip,xc+x,yc+y,col); set_px(chip,xc-x,yc+y,col);
    set_px(chip,xc+x,yc-y,col); set_px(chip,xc-x,yc-y,col);
    set_px(chip,xc+y,yc+x,col); set_px(chip,xc-y,yc+x,col);
    set_px(chip,xc+y,yc-x,col); set_px(chip,xc-y,yc-x,col);
    x++; d = d>0 ? d+4*(x-y--)+10 : d+4*x+6;
  }
}

static void on_display_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;

  /* ---- RPM update from PWM signal ---- */
  float max_rpm = (float)attr_read(chip->max_rpm_attr);
  bool enabled = (pin_read(chip->en) == HIGH);
  float target_rpm = (enabled && chip->duty_estimate > 0.01f)
      ? (200.0f + chip->duty_estimate * (max_rpm - 200.0f))
      : 0.0f;
  chip->rpm += (target_rpm - chip->rpm) * 0.10f;
  if (chip->rpm < 30.0f) chip->rpm = 0.0f;

  // Clear background
  for (int i = 0; i < FB_WIDTH * FB_HEIGHT; i++) chip->pixels[i] = COLOR_BG;

  // Outer ring (2px)
  draw_circle(chip, 32, 32, 30, COLOR_FRAME);
  draw_circle(chip, 32, 32, 29, COLOR_FRAME);

  // 7 curved blades: each blade is 3 lines (center + ±offset)
  for (int i = 0; i < 7; i++) {
    uint8_t base = (uint8_t)(chip->angle + (uint8_t)(i * 36)); // 256/7 ≈ 36
    // Primary span: from hub edge (r=8) to just inside ring (r=26)
    int x1 = 32 + (COS(base) * 8) / 127;
    int y1 = 32 + (SIN(base) * 8) / 127;
    // Tip is swept sideways for the curved look
    uint8_t tip_a = base + 20;
    int x2 = 32 + (COS(tip_a) * 26) / 127;
    int y2 = 32 + (SIN(tip_a) * 26) / 127;
    draw_line(chip, x1, y1, x2, y2, COLOR_BLADE);
    // Thicken with two parallel lines
    uint8_t a1 = base + 5, a2 = base - 5;
    draw_line(chip, 32+(COS(a1)*8)/127, 32+(SIN(a1)*8)/127,
                    32+(COS((uint8_t)(tip_a+5))*25)/127, 32+(SIN((uint8_t)(tip_a+5))*25)/127, COLOR_BLADE);
    draw_line(chip, 32+(COS(a2)*8)/127, 32+(SIN(a2)*8)/127,
                    32+(COS((uint8_t)(tip_a-5))*25)/127, 32+(SIN((uint8_t)(tip_a-5))*25)/127, COLOR_BLADE);
  }

  // Hub (filled via concentric circles)
  for (int r = 1; r <= 7; r++) draw_circle(chip, 32, 32, r, COLOR_HUB);
  set_px(chip, 32, 32, COLOR_HUB);

  // Status dot top-right (r=3)
  uint32_t dot = (chip->rpm > 30.0f) ? COLOR_ON : COLOR_OFF;
  for (int dy = -3; dy <= 3; dy++)
    for (int dx = -3; dx <= 3; dx++)
      if (dx*dx + dy*dy <= 9)
        set_px(chip, 54+dx, 10+dy, dot);

  buffer_write(chip->fb, 0, chip->pixels, sizeof(chip->pixels));

  // Advance angle: step = rpm/60 * 256 / 20FPS
  chip->angle += (uint32_t)(chip->rpm * 256.0f / 60.0f / 20.0f);
}

static void on_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  uint64_t now = get_sim_nanos();

  /* Update duty estimate for when firmware is driving PWM */
  float pwm_sample = (pin_read(chip->pwm_in) == HIGH) ? 1.0f : 0.0f;
  chip->duty_estimate = chip->duty_estimate * 0.95f + pwm_sample * 0.05f;

  /* Tachometer output only — RPM is now owned by on_display_timer */
  if (chip->rpm < 30.0f) {
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
  chip->display_timer = 0;
  chip->duty_estimate = 0.0f;
  chip->rpm = 0.0f;
  chip->tach_level = false;
  chip->last_toggle_ns = 0;
  chip->angle = 0;

  uint32_t fb_w = 0, fb_h = 0;
  chip->fb = framebuffer_init(&fb_w, &fb_h);

  // Draw initial frame immediately
  on_display_timer(chip);

  const timer_config_t timer_config = {
      .callback = on_timer,
      .user_data = chip,
  };
  chip->timer = timer_init(&timer_config);
  timer_start(chip->timer, 2000, true);

  const timer_config_t display_timer_config = {
      .callback = on_display_timer,
      .user_data = chip,
  };
  chip->display_timer = timer_init(&display_timer_config);
  timer_start(chip->display_timer, 50000, true); // 20 FPS
}
