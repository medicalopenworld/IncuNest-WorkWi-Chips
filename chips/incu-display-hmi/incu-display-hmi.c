#include "wokwi-api.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ─── 5×7 Font (ASCII 32-127, column-major, LSB at top) ──────────
static const uint8_t font5x7[96][5] = {
  {0x00,0x00,0x00,0x00,0x00}, // ' '
  {0x00,0x00,0x5F,0x00,0x00}, // '!'
  {0x00,0x07,0x00,0x07,0x00}, // '"'
  {0x14,0x7F,0x14,0x7F,0x14}, // '#'
  {0x24,0x2A,0x7F,0x2A,0x12}, // '$'
  {0x23,0x13,0x08,0x64,0x62}, // '%'
  {0x36,0x49,0x55,0x22,0x50}, // '&'
  {0x00,0x05,0x03,0x00,0x00}, // '''
  {0x00,0x1C,0x22,0x41,0x00}, // '('
  {0x00,0x41,0x22,0x1C,0x00}, // ')'
  {0x08,0x2A,0x1C,0x2A,0x08}, // '*'
  {0x08,0x08,0x3E,0x08,0x08}, // '+'
  {0x00,0x50,0x30,0x00,0x00}, // ','
  {0x08,0x08,0x08,0x08,0x08}, // '-'
  {0x00,0x60,0x60,0x00,0x00}, // '.'
  {0x20,0x10,0x08,0x04,0x02}, // '/'
  {0x3E,0x51,0x49,0x45,0x3E}, // '0'
  {0x00,0x42,0x7F,0x40,0x00}, // '1'
  {0x42,0x61,0x51,0x49,0x46}, // '2'
  {0x21,0x41,0x45,0x4B,0x31}, // '3'
  {0x18,0x14,0x12,0x7F,0x10}, // '4'
  {0x27,0x45,0x45,0x45,0x39}, // '5'
  {0x3C,0x4A,0x49,0x49,0x30}, // '6'
  {0x01,0x71,0x09,0x05,0x03}, // '7'
  {0x36,0x49,0x49,0x49,0x36}, // '8'
  {0x06,0x49,0x49,0x29,0x1E}, // '9'
  {0x00,0x36,0x36,0x00,0x00}, // ':'
  {0x00,0x56,0x36,0x00,0x00}, // ';'
  {0x00,0x08,0x14,0x22,0x41}, // '<'
  {0x14,0x14,0x14,0x14,0x14}, // '='
  {0x41,0x22,0x14,0x08,0x00}, // '>'
  {0x02,0x01,0x51,0x09,0x06}, // '?'
  {0x32,0x49,0x79,0x41,0x3E}, // '@'
  {0x7E,0x11,0x11,0x11,0x7E}, // 'A'
  {0x7F,0x49,0x49,0x49,0x36}, // 'B'
  {0x3E,0x41,0x41,0x41,0x22}, // 'C'
  {0x7F,0x41,0x41,0x22,0x1C}, // 'D'
  {0x7F,0x49,0x49,0x49,0x41}, // 'E'
  {0x7F,0x09,0x09,0x09,0x01}, // 'F'
  {0x3E,0x41,0x41,0x51,0x32}, // 'G'
  {0x7F,0x08,0x08,0x08,0x7F}, // 'H'
  {0x00,0x41,0x7F,0x41,0x00}, // 'I'
  {0x20,0x40,0x41,0x3F,0x01}, // 'J'
  {0x7F,0x08,0x14,0x22,0x41}, // 'K'
  {0x7F,0x40,0x40,0x40,0x40}, // 'L'
  {0x7F,0x02,0x04,0x02,0x7F}, // 'M'
  {0x7F,0x04,0x08,0x10,0x7F}, // 'N'
  {0x3E,0x41,0x41,0x41,0x3E}, // 'O'
  {0x7F,0x09,0x09,0x09,0x06}, // 'P'
  {0x3E,0x41,0x51,0x21,0x5E}, // 'Q'
  {0x7F,0x09,0x19,0x29,0x46}, // 'R'
  {0x46,0x49,0x49,0x49,0x31}, // 'S'
  {0x01,0x01,0x7F,0x01,0x01}, // 'T'
  {0x3F,0x40,0x40,0x40,0x3F}, // 'U'
  {0x1F,0x20,0x40,0x20,0x1F}, // 'V'
  {0x3F,0x40,0x38,0x40,0x3F}, // 'W'
  {0x63,0x14,0x08,0x14,0x63}, // 'X'
  {0x07,0x08,0x70,0x08,0x07}, // 'Y'
  {0x61,0x51,0x49,0x45,0x43}, // 'Z'
  {0x00,0x7F,0x41,0x41,0x00}, // '['
  {0x02,0x04,0x08,0x10,0x20}, // '\'
  {0x00,0x41,0x41,0x7F,0x00}, // ']'
  {0x04,0x02,0x01,0x02,0x04}, // '^'
  {0x40,0x40,0x40,0x40,0x40}, // '_'
  {0x00,0x01,0x02,0x04,0x00}, // '`'
  {0x20,0x54,0x54,0x54,0x78}, // 'a'
  {0x7F,0x48,0x44,0x44,0x38}, // 'b'
  {0x38,0x44,0x44,0x44,0x20}, // 'c'
  {0x38,0x44,0x44,0x48,0x7F}, // 'd'
  {0x38,0x54,0x54,0x54,0x18}, // 'e'
  {0x08,0x7E,0x09,0x01,0x02}, // 'f'
  {0x08,0x14,0x54,0x54,0x3C}, // 'g'
  {0x7F,0x08,0x04,0x04,0x78}, // 'h'
  {0x00,0x44,0x7D,0x40,0x00}, // 'i'
  {0x20,0x40,0x44,0x3D,0x00}, // 'j'
  {0x7F,0x10,0x28,0x44,0x00}, // 'k'
  {0x00,0x41,0x7F,0x40,0x00}, // 'l'
  {0x7C,0x04,0x18,0x04,0x78}, // 'm'
  {0x7C,0x08,0x04,0x04,0x78}, // 'n'
  {0x38,0x44,0x44,0x44,0x38}, // 'o'
  {0x7C,0x14,0x14,0x14,0x08}, // 'p'
  {0x08,0x14,0x14,0x18,0x7C}, // 'q'
  {0x7C,0x08,0x04,0x04,0x08}, // 'r'
  {0x48,0x54,0x54,0x54,0x20}, // 's'
  {0x04,0x3F,0x44,0x40,0x20}, // 't'
  {0x3C,0x40,0x40,0x20,0x7C}, // 'u'
  {0x1C,0x20,0x40,0x20,0x1C}, // 'v'
  {0x3C,0x40,0x30,0x40,0x3C}, // 'w'
  {0x44,0x28,0x10,0x28,0x44}, // 'x'
  {0x0C,0x50,0x50,0x50,0x3C}, // 'y'
  {0x44,0x64,0x54,0x4C,0x44}, // 'z'
  {0x00,0x08,0x36,0x41,0x00}, // '{'
  {0x00,0x00,0x7F,0x00,0x00}, // '|'
  {0x00,0x41,0x36,0x08,0x00}, // '}'
  {0x08,0x08,0x2A,0x1C,0x08}, // '~'
  {0x08,0x1C,0x2A,0x08,0x08}, // DEL
};

// ─── Color Scheme (RGBA, alpha=0xFF) ─────────────────────────────
#define COLOR_BG_DARK       0x1A1A2EFF
#define COLOR_BG_PANEL      0x16213EFF
#define COLOR_BG_HEADER     0x0F3460FF
#define COLOR_BORDER        0x334155FF
#define COLOR_TEXT_PRIMARY   0xFFFFFFFF
#define COLOR_TEXT_SECONDARY 0xB0BEC5FF
#define COLOR_TEXT_DIM       0x607D8BFF
#define COLOR_TEMP_AIR      0x42A5F5FF
#define COLOR_TEMP_SKIN     0xFF7043FF
#define COLOR_HUMIDITY      0x26C6DAFF
#define COLOR_SETPOINT      0x66BB6AFF
#define COLOR_ON_GREEN      0x4CAF50FF
#define COLOR_OFF_GRAY      0x455A64FF
#define COLOR_HEATER_ON     0xFF5722FF
#define COLOR_FAN_ON        0x29B6F6FF
#define COLOR_PHOTO_ON      0xFFEB3BFF
#define COLOR_ALARM_CRIT    0xF44336FF
#define COLOR_ALARM_WARN    0xFFA726FF
#define COLOR_ALARM_BG      0x3E1111FF
#define COLOR_CONN_OK       0x4CAF50FF
#define COLOR_CONN_LOST     0xF44336FF
#define COLOR_CONN_WAIT     0xFFC107FF

// ─── Constants ───────────────────────────────────────────────────
#define MAX_ALARMS       8
#define MAX_ALARM_DESC  48
#define UART_BUF_SIZE  512
// Logical UI grid. The framebuffer can be larger (e.g. 800x480) and is scaled.
#define DISPLAY_W      480
#define DISPLAY_H      320
#define FOOTER_Y       296
#define FOOTER_H        24
#define BUTTON_DEBOUNCE_NS 120000000ULL
#define SETTINGS_TOTAL_ROWS 9
#define SETTINGS_VISIBLE_ROWS 7
#define SETTINGS_ROW_CONTROL_MODE 0
#define SETTINGS_ROW_AIR_SETPOINT 1
#define SETTINGS_ROW_SKIN_SETPOINT 2
#define SETTINGS_ROW_HUM_SETPOINT 3
#define SETTINGS_ROW_SKIN_SENSOR 4
#define SETTINGS_ROW_LANGUAGE 5
#define SETTINGS_ROW_COMM_TIMEOUT 6
#define SETTINGS_ROW_COMM_LINK 7
#define SETTINGS_ROW_AUTO_REQUEST 8

// ─── Data Types ──────────────────────────────────────────────────
typedef enum {
  SCREEN_BOOT = 0,
  SCREEN_MAIN,
  SCREEN_SETTINGS,
  SCREEN_ALARMS,
  SCREEN_CHARTS,
  SCREEN_PULSEOXI,
  SCREEN_LOCK,
  SCREEN_COUNT
} screen_id_t;

typedef struct {
  int  id;
  int  type;   // 0=info, 1=warning, 2=critical
  char desc[MAX_ALARM_DESC];
  int  state;  // 0=inactive, 1=active, 2=ack
} alarm_entry_t;

typedef struct {
  float air_temp, skin_temp, humidity, fan_rpm, heater_duty;
  int   door_open, alarm_code;
  bool  valid;
} telemetry_t;

typedef struct {
  int   actuators_enabled, control_mode;
  float air_setpoint, skin_setpoint, hum_setpoint;
  int   phototherapy, mute;
  char  serial_number[20], hw_number[8], hw_revision[8], fw_version[16];
  int   num_alarms, skin_enabled, comm_status, photo_time_min, photo_time_sec;
  bool  valid;
} state_info_t;

typedef struct {
  int      blink_phase;
  bool     needs_redraw;
  uint64_t last_blink_ns;
} render_state_t;

typedef struct {
  uint64_t last_msg_ns;
  bool     connected;
  int      msg_count;
} conn_health_t;

typedef struct {
  uart_dev_t     uart;
  buffer_t       fb;
  timer_t        refresh_timer;
  timer_t        watchdog_timer;
  uint32_t       width, height;
  uint32_t      *pixels;
  pin_t          btn_next, btn_prev, btn_ok, btn_up, btn_down;
  char           rx_buf[UART_BUF_SIZE];
  int            rx_len;
  bool           rx_overflow;
  telemetry_t    telemetry;
  state_info_t   state;
  alarm_entry_t  alarms[MAX_ALARMS];
  int            alarm_count;
  render_state_t render;
  conn_health_t  conn;
  uint32_t       attr_control_mode, attr_language, attr_skin_enabled;
  uint32_t       attr_air_setpoint, attr_skin_setpoint, attr_hum_setpoint;
  uint32_t       attr_comm_timeout, attr_auto_request;
  uint32_t       attr_touch_x, attr_touch_y, attr_touch_tap;
  int            ui_language;
  int            ui_skin_enabled;
  int            ui_comm_timeout_ms;
  int            ui_auto_request;
  uint64_t       boot_ns;
  uint64_t       last_btn_ns;
  uint32_t       last_touch_tap;
  int            touch_x, touch_y;
  int            settings_cursor;
  int            settings_scroll;
  int            alarms_cursor;
  screen_id_t    active_screen;
  bool           ui_locked;
} chip_state_t;

// ─── Number-to-string helpers (WASI has no %f in snprintf) ───────
static void float_to_str(float val, char *buf, int decimals) {
  if (val < 0) { *buf++ = '-'; val = -val; }
  int ipart = (int)val;
  float frac = val - (float)ipart;
  char tmp[12]; int len = 0;
  if (ipart == 0) tmp[len++] = '0';
  else { while (ipart > 0) { tmp[len++] = '0' + (ipart % 10); ipart /= 10; } }
  for (int i = len - 1; i >= 0; i--) *buf++ = tmp[i];
  if (decimals > 0) {
    *buf++ = '.';
    for (int d = 0; d < decimals; d++) {
      frac *= 10.0f;
      int digit = (int)frac;
      *buf++ = '0' + digit;
      frac -= (float)digit;
    }
  }
  *buf = '\0';
}

static void int_to_str(int val, char *buf) {
  if (val == -2147483647 - 1) { strcpy(buf, "-2147483648"); return; }
  if (val < 0) { *buf++ = '-'; val = -val; }
  char tmp[12]; int len = 0;
  if (val == 0) tmp[len++] = '0';
  else { while (val > 0) { tmp[len++] = '0' + (val % 10); val /= 10; } }
  for (int i = len - 1; i >= 0; i--) *buf++ = tmp[i];
  *buf = '\0';
}

// CSV field extractor (replaces strsep, which may not exist in wasi-libc)
static char *next_csv(char **p) {
  if (!*p) return "";
  char *start = *p;
  char *c = strchr(start, ',');
  if (c) { *c = '\0'; *p = c + 1; } else { *p = NULL; }
  return start;
}

static const char *screen_name(screen_id_t scr) {
  switch (scr) {
    case SCREEN_BOOT:     return "BOOT";
    case SCREEN_MAIN:     return "MAIN";
    case SCREEN_SETTINGS: return "SETTINGS";
    case SCREEN_ALARMS:   return "ALARMS";
    case SCREEN_CHARTS:   return "CHARTS";
    case SCREEN_PULSEOXI: return "PULSEOXI";
    case SCREEN_LOCK:     return "LOCK";
    default:              return "MAIN";
  }
}

static const char *mode_name(int mode) {
  return mode ? "AIR" : "SKIN";
}

static const char *language_name(int lang) {
  switch (lang) {
    case 1: return "ES";
    case 2: return "FR";
    case 3: return "PT";
    default: return "EN";
  }
}

static void send_hmi_control(chip_state_t *s) {
  char act[12], skin_mode[12], mode[12], air[16], skin[16], hum[16];
  char photo[12], mute[12], lang[12], photo_min[12];
  int_to_str(s->state.actuators_enabled, act);
  int_to_str(s->ui_skin_enabled, skin_mode);
  int_to_str(s->state.control_mode, mode);
  float_to_str(s->state.air_setpoint, air, 1);
  float_to_str(s->state.skin_setpoint, skin, 1);
  float_to_str(s->state.hum_setpoint, hum, 0);
  int_to_str(s->state.phototherapy, photo);
  int_to_str(s->state.mute, mute);
  int_to_str(s->ui_language, lang);
  int_to_str(s->state.photo_time_min, photo_min);

  char msg[220];
  msg[0] = '\0';
  strcat(msg, "HMI,");
  strcat(msg, act); strcat(msg, ",");
  strcat(msg, skin_mode); strcat(msg, ",");
  strcat(msg, mode); strcat(msg, ",");
  strcat(msg, air); strcat(msg, ",");
  strcat(msg, skin); strcat(msg, ",");
  strcat(msg, hum); strcat(msg, ",");
  strcat(msg, photo); strcat(msg, ",");
  strcat(msg, mute); strcat(msg, ",");
  strcat(msg, lang); strcat(msg, ",");
  strcat(msg, photo_min); strcat(msg, "\n");

  uart_write(s->uart, (uint8_t *)msg, strlen(msg));
}

static void nav_prev(chip_state_t *s) {
  if (s->ui_locked && s->active_screen != SCREEN_LOCK) return;
  s->active_screen = (screen_id_t)((s->active_screen + SCREEN_COUNT - 1) % SCREEN_COUNT);
  s->render.needs_redraw = true;
}

static void nav_next(chip_state_t *s) {
  if (s->ui_locked && s->active_screen != SCREEN_LOCK) return;
  s->active_screen = (screen_id_t)((s->active_screen + 1) % SCREEN_COUNT);
  s->render.needs_redraw = true;
}

static void nav_ok(chip_state_t *s) {
  bool emit_hmi = false;
  if (s->active_screen == SCREEN_BOOT) {
    s->active_screen = SCREEN_MAIN;
  } else if (s->active_screen == SCREEN_LOCK) {
    s->ui_locked = !s->ui_locked;
    if (!s->ui_locked) s->active_screen = SCREEN_MAIN;
  } else if (s->ui_locked) {
    s->active_screen = SCREEN_LOCK;
  } else if (s->active_screen == SCREEN_SETTINGS) {
    switch (s->settings_cursor) {
      case SETTINGS_ROW_CONTROL_MODE:
        s->state.control_mode = s->state.control_mode ? 0 : 1;
        emit_hmi = true;
        break;
      case SETTINGS_ROW_AIR_SETPOINT:
        s->state.air_setpoint += 0.5f;
        if (s->state.air_setpoint > 40.0f) s->state.air_setpoint = 30.0f;
        emit_hmi = true;
        break;
      case SETTINGS_ROW_SKIN_SETPOINT:
        s->state.skin_setpoint += 0.5f;
        if (s->state.skin_setpoint > 39.0f) s->state.skin_setpoint = 34.0f;
        emit_hmi = true;
        break;
      case SETTINGS_ROW_HUM_SETPOINT:
        s->state.hum_setpoint += 5.0f;
        if (s->state.hum_setpoint > 95.0f) s->state.hum_setpoint = 40.0f;
        emit_hmi = true;
        break;
      case SETTINGS_ROW_SKIN_SENSOR:
        s->ui_skin_enabled = s->ui_skin_enabled ? 0 : 1;
        emit_hmi = true;
        break;
      case SETTINGS_ROW_LANGUAGE:
        s->ui_language = (s->ui_language + 1) % 4;
        emit_hmi = true;
        break;
      case SETTINGS_ROW_COMM_TIMEOUT:
        if (s->ui_comm_timeout_ms < 2000) s->ui_comm_timeout_ms = 2000;
        else if (s->ui_comm_timeout_ms < 3000) s->ui_comm_timeout_ms = 3000;
        else if (s->ui_comm_timeout_ms < 5000) s->ui_comm_timeout_ms = 5000;
        else s->ui_comm_timeout_ms = 1000;
        break;
      case SETTINGS_ROW_AUTO_REQUEST:
        s->ui_auto_request = s->ui_auto_request ? 0 : 1;
        break;
      case SETTINGS_ROW_COMM_LINK:
      default:
        break;
    }
  } else if (s->active_screen == SCREEN_ALARMS && s->alarm_count > 0) {
    s->state.mute = s->state.mute ? 0 : 1;
    emit_hmi = true;
  }
  if (emit_hmi) send_hmi_control(s);
  s->render.needs_redraw = true;
}

static void nav_up(chip_state_t *s) {
  if (s->ui_locked && s->active_screen != SCREEN_LOCK) return;

  if (s->active_screen == SCREEN_SETTINGS) {
    if (s->settings_cursor > 0) s->settings_cursor--;
    if (s->settings_cursor < s->settings_scroll) s->settings_scroll = s->settings_cursor;
  } else if (s->active_screen == SCREEN_ALARMS) {
    if (s->alarms_cursor > 0) s->alarms_cursor--;
  } else {
    return;
  }

  s->render.needs_redraw = true;
}

static void nav_down(chip_state_t *s) {
  if (s->ui_locked && s->active_screen != SCREEN_LOCK) return;

  if (s->active_screen == SCREEN_SETTINGS) {
    int max_scroll = SETTINGS_TOTAL_ROWS - SETTINGS_VISIBLE_ROWS;
    if (max_scroll < 0) max_scroll = 0;
    if (s->settings_cursor < SETTINGS_TOTAL_ROWS - 1) s->settings_cursor++;
    if (s->settings_cursor >= s->settings_scroll + SETTINGS_VISIBLE_ROWS) {
      s->settings_scroll = s->settings_cursor - SETTINGS_VISIBLE_ROWS + 1;
      if (s->settings_scroll > max_scroll) s->settings_scroll = max_scroll;
    }
  } else if (s->active_screen == SCREEN_ALARMS) {
    int max_cursor = s->alarm_count - 1;
    if (max_cursor < 0) return;
    if (s->alarms_cursor < max_cursor) s->alarms_cursor++;
  } else {
    return;
  }

  s->render.needs_redraw = true;
}

static void process_virtual_tap(chip_state_t *s, int x, int y) {
  if (x < 0 || x >= DISPLAY_W || y < FOOTER_Y || y >= DISPLAY_H) return;
  int zone = (x * 3) / DISPLAY_W;
  if (zone == 0)
    nav_prev(s);
  else if (zone == 1)
    nav_ok(s);
  else
    nav_next(s);
}

static void poll_touch_controls(chip_state_t *s) {
  int x = (int)attr_read(s->attr_touch_x);
  int y = (int)attr_read(s->attr_touch_y);
  uint32_t tap = attr_read(s->attr_touch_tap) ? 1 : 0;

  int max_x = s->width > 0 ? (int)s->width - 1 : DISPLAY_W - 1;
  int max_y = s->height > 0 ? (int)s->height - 1 : DISPLAY_H - 1;
  if (x < 0) x = 0;
  if (x > max_x) x = max_x;
  if (y < 0) y = 0;
  if (y > max_y) y = max_y;

  // Map from framebuffer coordinates to logical UI coordinates.
  x = (int)(((long long)x * (DISPLAY_W - 1)) / (max_x > 0 ? max_x : 1));
  y = (int)(((long long)y * (DISPLAY_H - 1)) / (max_y > 0 ? max_y : 1));
  s->touch_x = x;
  s->touch_y = y;

  if (!s->last_touch_tap && tap)
    process_virtual_tap(s, s->touch_x, s->touch_y);

  s->last_touch_tap = tap;
}

// ─── Drawing Primitives ─────────────────────────────────────────
static void draw_pixel(chip_state_t *s, int x, int y, uint32_t c) {
  if (x < 0 || x >= DISPLAY_W || y < 0 || y >= DISPLAY_H) return;

  int x0 = (int)(((long long)x * s->width) / DISPLAY_W);
  int x1 = (int)((((long long)(x + 1) * s->width) / DISPLAY_W) - 1);
  int y0 = (int)(((long long)y * s->height) / DISPLAY_H);
  int y1 = (int)((((long long)(y + 1) * s->height) / DISPLAY_H) - 1);
  if (x1 < x0) x1 = x0;
  if (y1 < y0) y1 = y0;

  for (int py = y0; py <= y1; py++) {
    for (int px = x0; px <= x1; px++) {
      s->pixels[py * s->width + px] = c;
    }
  }
}

static void fill_rect(chip_state_t *s, int x, int y, int w, int h, uint32_t c) {
  for (int j = y; j < y + h; j++)
    for (int i = x; i < x + w; i++)
      draw_pixel(s, i, j, c);
}

static void draw_char(chip_state_t *s, int x, int y, char ch, uint32_t c, int sc) {
  if (ch < 32 || ch > 127) ch = '?';
  const uint8_t *g = font5x7[ch - 32];
  for (int i = 0; i < 5; i++)
    for (int j = 0; j < 7; j++)
      if (g[i] & (1 << j))
        for (int sx = 0; sx < sc; sx++)
          for (int sy = 0; sy < sc; sy++)
            draw_pixel(s, x + i*sc + sx, y + j*sc + sy, c);
}

static void draw_string(chip_state_t *s, int x, int y, const char *str, uint32_t c, int sc) {
  while (*str) { draw_char(s, x, y, *str++, c, sc); x += 6 * sc; }
}

static int string_width(const char *str, int sc) {
  int len = 0; while (str[len]) len++;
  return len * 6 * sc;
}

static void draw_string_centered(chip_state_t *s, int cx, int y, const char *str, uint32_t c, int sc) {
  draw_string(s, cx - string_width(str, sc) / 2, y, str, c, sc);
}

static void draw_progress_bar(chip_state_t *s, int x, int y, int w, int h,
                               float val, float mn, float mx,
                               uint32_t fill, uint32_t bg) {
  fill_rect(s, x, y, w, h, bg);
  if (mx <= mn) return;
  float ratio = (val - mn) / (mx - mn);
  if (ratio < 0.0f) ratio = 0.0f;
  if (ratio > 1.0f) ratio = 1.0f;
  int fw = (int)(ratio * (float)w);
  if (fw > 0) fill_rect(s, x, y, fw, h, fill);
}

static void draw_circle(chip_state_t *s, int cx, int cy, int r, uint32_t c) {
  for (int dy = -r; dy <= r; dy++)
    for (int dx = -r; dx <= r; dx++)
      if (dx*dx + dy*dy <= r*r)
        draw_pixel(s, cx + dx, cy + dy, c);
}

// ─── JSON Parser (minimal, no malloc) ────────────────────────────
static float json_extract_float(const char *json, const char *key) {
  char search[32];
  int i = 0;
  search[i++] = '"';
  for (const char *k = key; *k && i < 28; k++) search[i++] = *k;
  search[i++] = '"'; search[i++] = ':'; search[i] = '\0';
  const char *p = strstr(json, search);
  if (!p) return -999.0f;
  return (float)strtod(p + i, NULL);
}

static void parse_json_telemetry(chip_state_t *s) {
  s->telemetry.air_temp    = json_extract_float(s->rx_buf, "temp");
  s->telemetry.skin_temp   = json_extract_float(s->rx_buf, "skin");
  s->telemetry.humidity    = json_extract_float(s->rx_buf, "hum");
  s->telemetry.fan_rpm     = json_extract_float(s->rx_buf, "fan");
  s->telemetry.heater_duty = json_extract_float(s->rx_buf, "heater");
  float door = json_extract_float(s->rx_buf, "door");
  s->telemetry.door_open   = (door > 0.5f) ? 1 : 0;
  float alm = json_extract_float(s->rx_buf, "alarm");
  s->telemetry.alarm_code  = (alm == -999.0f) ? 0 : (int)alm;
  s->telemetry.valid       = true;
}

// ─── CSV Protocol Parsers ────────────────────────────────────────
static void parse_ctrl_tel(chip_state_t *s) {
  char *p = s->rx_buf + 9; // skip "CTRL,TEL,"
  s->telemetry.air_temp  = (float)strtod(p, &p); if (*p == ',') p++;
  s->telemetry.skin_temp = (float)strtod(p, &p); if (*p == ',') p++;
  s->telemetry.humidity  = (float)strtod(p, &p);
  if (*p == ',') { p++; s->state.comm_status = atoi(p); }
  s->telemetry.valid = true;
}

static void parse_ctrl_state(chip_state_t *s) {
  char buf[UART_BUF_SIZE];
  strncpy(buf, s->rx_buf + 11, UART_BUF_SIZE - 1);
  buf[UART_BUF_SIZE - 1] = '\0';
  char *p = buf, *t;
  t = next_csv(&p); s->state.actuators_enabled = atoi(t);
  t = next_csv(&p); s->state.control_mode      = atoi(t);
  t = next_csv(&p); s->state.air_setpoint      = (float)strtod(t, NULL);
  t = next_csv(&p); s->state.skin_setpoint     = (float)strtod(t, NULL);
  t = next_csv(&p); s->state.hum_setpoint      = (float)strtod(t, NULL);
  t = next_csv(&p); s->state.phototherapy       = atoi(t);
  t = next_csv(&p); s->state.mute               = atoi(t);
  t = next_csv(&p); strncpy(s->state.serial_number, t, 19);
  t = next_csv(&p); strncpy(s->state.hw_number, t, 7);
  t = next_csv(&p); strncpy(s->state.hw_revision, t, 7);
  t = next_csv(&p); strncpy(s->state.fw_version, t, 15);
  // Optional extended fields
  if (p) { t = next_csv(&p); s->state.num_alarms     = atoi(t); }
  if (p) { t = next_csv(&p); s->state.skin_enabled   = atoi(t); }
  if (p) { t = next_csv(&p); s->state.comm_status    = atoi(t); }
  if (p) { t = next_csv(&p); float pt = (float)strtod(t, NULL);
           s->state.photo_time_min = (int)pt;
           s->state.photo_time_sec = (int)((pt - (int)pt) * 100.0f + 0.5f); }
  s->state.valid = true;
}

static void parse_ctrl_alarm(chip_state_t *s) {
  char buf[UART_BUF_SIZE];
  strncpy(buf, s->rx_buf + 9, UART_BUF_SIZE - 1);
  buf[UART_BUF_SIZE - 1] = '\0';
  char *p = buf, *t;
  t = next_csv(&p); int id = atoi(t);
  int slot = -1;
  for (int i = 0; i < MAX_ALARMS; i++) {
    if (s->alarms[i].id == id) { slot = i; break; }
    if (slot < 0 && s->alarms[i].id == 0) slot = i;
  }
  if (slot < 0) return;
  s->alarms[slot].id = id;
  // type field is text (e.g. "HEATER ERROR"); map to severity
  t = next_csv(&p);
  if (strstr(t, "ERROR") || strstr(t, "FAIL") || strstr(t, "CRIT"))
    s->alarms[slot].type = 2; // critical
  else if (strstr(t, "WARN") || strstr(t, "HIGH") || strstr(t, "LOW"))
    s->alarms[slot].type = 1; // warning
  else
    s->alarms[slot].type = 0; // info
  t = next_csv(&p); strncpy(s->alarms[slot].desc, t, MAX_ALARM_DESC - 1);
  t = next_csv(&p); s->alarms[slot].state = atoi(t);
  if (s->alarms[slot].state == 0) s->alarms[slot].id = 0;
  s->alarm_count = 0;
  for (int i = 0; i < MAX_ALARMS; i++)
    if (s->alarms[i].id != 0) s->alarm_count++;
  if (s->alarm_count <= 0) s->alarms_cursor = 0;
  else if (s->alarms_cursor >= s->alarm_count) s->alarms_cursor = s->alarm_count - 1;
}

static void parse_message(chip_state_t *s) {
  s->conn.last_msg_ns = get_sim_nanos();
  s->conn.connected = true;
  s->conn.msg_count++;
  if (s->rx_buf[0] == '{')
    parse_json_telemetry(s);
  else if (strncmp(s->rx_buf, "CTRL,TEL,", 9) == 0)
    parse_ctrl_tel(s);
  else if (strncmp(s->rx_buf, "CTRL,STATE,", 11) == 0)
    parse_ctrl_state(s);
  else if (strncmp(s->rx_buf, "CTRL,ALM,", 9) == 0)
    parse_ctrl_alarm(s);
  if (s->active_screen == SCREEN_BOOT && (s->telemetry.valid || s->state.valid))
    s->active_screen = SCREEN_MAIN;
  s->render.needs_redraw = true;
}

// ─── Rendering Functions ─────────────────────────────────────────
static void render_boot_screen(chip_state_t *s) {
  draw_string_centered(s, 240, 100, "IncuNest", COLOR_TEXT_PRIMARY, 3);
  draw_string_centered(s, 240, 140, "Neonatal Incubator", COLOR_TEXT_SECONDARY, 2);
  if (s->render.blink_phase)
    draw_string_centered(s, 240, 180, "Connecting...", COLOR_CONN_WAIT, 2);
  draw_string_centered(s, 240, 220,
                       s->conn.msg_count > 0 ? "Data stream detected" : "Waiting for motherboard",
                       COLOR_TEXT_DIM, 1);
  draw_string_centered(s, 240, 238, "NEXT/PREV: browse screens", COLOR_TEXT_DIM, 1);
  draw_string_centered(s, 240, 250, "UP/DOWN: vertical nav  OK: action", COLOR_TEXT_DIM, 1);
}

static void render_header(chip_state_t *s) {
  fill_rect(s, 0, 0, 480, 30, COLOR_BG_HEADER);
  draw_string(s, 8, 7, "IncuNest", COLOR_TEXT_PRIMARY, 2);
  // Mode indicator
  const char *mode = s->state.valid ? mode_name(s->state.control_mode) : "N/A";
  char mode_line[20]; mode_line[0] = '\0';
  strcat(mode_line, mode); strcat(mode_line, " MODE");
  draw_string_centered(s, 240, 10, mode_line, COLOR_TEXT_SECONDARY, 1);
  draw_string(s, 300, 10, screen_name(s->active_screen), COLOR_TEXT_DIM, 1);
  if (s->ui_locked)
    draw_string(s, 360, 10, "LCK", COLOR_ALARM_WARN, 1);
  // Connection LED
  uint32_t led_color = s->conn.connected ? COLOR_CONN_OK : COLOR_CONN_LOST;
  draw_circle(s, 430, 15, 5, led_color);
  const char *conn_str = s->conn.connected ? "OK" : "LOST";
  draw_string(s, 440, 10, conn_str, led_color, 1);
}

static void render_temp_panel(chip_state_t *s, int px, int py,
                               const char *label, float value, float setpoint,
                               uint32_t val_color, bool show_valid) {
  int pw = 234, ph = 113;
  fill_rect(s, px, py, pw, ph, COLOR_BG_PANEL);
  // Label
  draw_string(s, px + 8, py + 4, label, COLOR_TEXT_SECONDARY, 2);
  // Big value
  char vbuf[16];
  if (show_valid) float_to_str(value, vbuf, 1);
  else { vbuf[0]='-'; vbuf[1]='-'; vbuf[2]='.'; vbuf[3]='-'; vbuf[4]='\0'; }
  int vx = px + (pw - string_width(vbuf, 3) - 30) / 2;
  draw_string(s, vx, py + 28, vbuf, val_color, 3);
  // Degree symbol (small square) + C
  int after = vx + string_width(vbuf, 3);
  fill_rect(s, after + 2, py + 28, 4, 4, val_color);
  draw_string(s, after + 8, py + 28, "C", val_color, 3);
  // Progress bar
  draw_progress_bar(s, px + 8, py + 60, pw - 16, 8,
                    show_valid ? value : 0, 20.0f, 42.0f, val_color, COLOR_OFF_GRAY);
  // Setpoint
  char sbuf[32]; sbuf[0] = '\0';
  strcat(sbuf, "SET: ");
  char tmp[12]; float_to_str(setpoint, tmp, 1);
  strcat(sbuf, tmp); strcat(sbuf, " C");
  draw_string(s, px + 8, py + 76, sbuf, COLOR_SETPOINT, 2);
  // Range labels
  draw_string(s, px + 8, py + 98, "20C", COLOR_TEXT_DIM, 1);
  draw_string(s, px + pw - 24, py + 98, "42C", COLOR_TEXT_DIM, 1);
}

static void render_humidity_bar(chip_state_t *s) {
  fill_rect(s, 2, 148, 476, 40, COLOR_BG_PANEL);
  draw_string(s, 10, 152, "HUMIDITY", COLOR_TEXT_SECONDARY, 2);
  // Value
  char vbuf[16];
  bool valid = s->telemetry.valid && s->conn.connected;
  if (valid) float_to_str(s->telemetry.humidity, vbuf, 1);
  else { vbuf[0]='-'; vbuf[1]='-'; vbuf[2]='.'; vbuf[3]='-'; vbuf[4]='\0'; }
  strcat(vbuf, "%");
  draw_string(s, 120, 152, vbuf, COLOR_HUMIDITY, 2);
  // Setpoint
  char sbuf[32]; sbuf[0] = '\0';
  strcat(sbuf, "SET: ");
  char tmp[12]; float_to_str(s->state.hum_setpoint, tmp, 0);
  strcat(sbuf, tmp); strcat(sbuf, "%");
  draw_string(s, 230, 152, sbuf, COLOR_SETPOINT, 2);
  // Progress bar
  draw_progress_bar(s, 10, 172, 456, 8,
                    valid ? s->telemetry.humidity : 0, 0.0f, 100.0f,
                    COLOR_HUMIDITY, COLOR_OFF_GRAY);
}

static void render_actuator_cell(chip_state_t *s, int x, int y, int w,
                                  const char *label, bool on, uint32_t on_color,
                                  const char *on_text, const char *off_text, bool flash) {
  uint32_t bg = COLOR_BG_PANEL;
  if (flash && on && s->render.blink_phase) bg = COLOR_ALARM_WARN;
  fill_rect(s, x, y, w, 28, bg);
  draw_circle(s, x + 10, y + 14, 4, on ? on_color : COLOR_OFF_GRAY);
  draw_string(s, x + 18, y + 4, label, COLOR_TEXT_SECONDARY, 1);
  uint32_t tc = on ? COLOR_TEXT_PRIMARY : COLOR_TEXT_DIM;
  draw_string(s, x + 18, y + 16, on ? on_text : off_text, tc, 1);
}

static void render_actuator_row(chip_state_t *s) {
  int y = 190, cw = 119;
  bool htr = s->telemetry.heater_duty > 0.5f;
  bool fan = s->telemetry.fan_rpm > 10.0f;
  render_actuator_cell(s, 2,   y, cw, "HTR", htr, COLOR_HEATER_ON, "ON", "OFF", false);
  render_actuator_cell(s, 121, y, cw, "FAN", fan, COLOR_FAN_ON, "ON", "OFF", false);
  render_actuator_cell(s, 240, y, cw, "PHOTO", s->state.phototherapy != 0, COLOR_PHOTO_ON, "ON", "OFF", false);
  bool door = s->telemetry.door_open != 0;
  render_actuator_cell(s, 359, y, 119, "DOOR", door, COLOR_ALARM_WARN, "OPEN!", "CLOSED", door);
}

static void render_alarm_panel(chip_state_t *s) {
  // Background
  bool has_crit = false;
  for (int i = 0; i < MAX_ALARMS; i++)
    if (s->alarms[i].id && s->alarms[i].type == 2) has_crit = true;
  uint32_t hdr_bg = COLOR_BG_PANEL;
  if (has_crit && s->render.blink_phase) hdr_bg = COLOR_ALARM_BG;
  fill_rect(s, 2, 222, 476, 78, COLOR_BG_PANEL);
  fill_rect(s, 2, 222, 476, 16, hdr_bg);
  // Header text
  char hdr[32]; hdr[0] = '\0';
  strcat(hdr, "ALARMS (");
  char cnt[8]; int_to_str(s->alarm_count, cnt);
  strcat(hdr, cnt); strcat(hdr, ")");
  draw_string(s, 10, 225, hdr, COLOR_TEXT_PRIMARY, 1);
  // Alarm rows (up to 4 visible)
  int row = 0;
  for (int i = 0; i < MAX_ALARMS && row < 4; i++) {
    if (s->alarms[i].id == 0) continue;
    int ry = 240 + row * 14;
    uint32_t ac;
    if (s->alarms[i].type == 2) ac = COLOR_ALARM_CRIT;
    else if (s->alarms[i].type == 1) ac = COLOR_ALARM_WARN;
    else ac = COLOR_FAN_ON;
    draw_circle(s, 14, ry + 4, 3, ac);
    // ID
    char id_buf[24]; id_buf[0] = '\0';
    strcat(id_buf, "ALM-");
    char nb[12]; int_to_str(s->alarms[i].id, nb);
    strcat(id_buf, nb);
    draw_string(s, 22, ry, id_buf, ac, 1);
    // Type label
    const char *tl = s->alarms[i].type == 2 ? "CRIT" :
                     s->alarms[i].type == 1 ? "WARN" : "INFO";
    draw_string(s, 80, ry, tl, ac, 1);
    // Description
    draw_string(s, 120, ry, s->alarms[i].desc, COLOR_TEXT_SECONDARY, 1);
    row++;
  }
  if (s->alarm_count == 0)
    draw_string(s, 10, 248, "(no active alarms)", COLOR_TEXT_DIM, 1);
}

static void render_main_screen(chip_state_t *s) {
  bool tv = s->telemetry.valid && s->conn.connected;
  render_temp_panel(s, 2, 32, "AIR TEMPERATURE",
                    s->telemetry.air_temp, s->state.air_setpoint,
                    COLOR_TEMP_AIR, tv);
  bool skin_en = (s->ui_skin_enabled != 0);
  render_temp_panel(s, 244, 32, "SKIN TEMPERATURE",
                    s->telemetry.skin_temp, s->state.skin_setpoint,
                    COLOR_TEMP_SKIN, tv && skin_en);
  render_humidity_bar(s);
  render_actuator_row(s);
  render_alarm_panel(s);
}

static void draw_settings_row(chip_state_t *s, int y, const char *label, const char *value, uint32_t value_color) {
  draw_string(s, 14, y, label, COLOR_TEXT_SECONDARY, 1);
  draw_string(s, 248, y, value, value_color, 1);
}

static void render_settings_screen(chip_state_t *s) {
  fill_rect(s, 2, 32, 476, 264, COLOR_BG_PANEL);
  draw_string(s, 12, 40, "SETTINGS", COLOR_TEXT_PRIMARY, 2);
  draw_string(s, 190, 44, "Live configuration snapshot", COLOR_TEXT_DIM, 1);

  const char *labels[SETTINGS_TOTAL_ROWS] = {
    "Control mode",
    "Air setpoint",
    "Skin setpoint",
    "Humidity setpoint",
    "Skin sensor",
    "Language",
    "Comm timeout",
    "Comm link",
    "Auto state request"
  };
  uint32_t value_colors[SETTINGS_TOTAL_ROWS] = {
    COLOR_TEXT_PRIMARY,
    COLOR_SETPOINT,
    COLOR_SETPOINT,
    COLOR_SETPOINT,
    COLOR_TEXT_PRIMARY,
    COLOR_TEXT_PRIMARY,
    COLOR_TEXT_PRIMARY,
    s->conn.connected ? COLOR_CONN_OK : COLOR_CONN_LOST,
    COLOR_TEXT_PRIMARY
  };
  char values[SETTINGS_TOTAL_ROWS][40];
  char n[16];

  strcpy(values[0], mode_name(s->state.control_mode));
  float_to_str(s->state.air_setpoint, n, 1); values[1][0] = '\0'; strcat(values[1], n); strcat(values[1], " C");
  float_to_str(s->state.skin_setpoint, n, 1); values[2][0] = '\0'; strcat(values[2], n); strcat(values[2], " C");
  float_to_str(s->state.hum_setpoint, n, 0); values[3][0] = '\0'; strcat(values[3], n); strcat(values[3], " %");
  strcpy(values[4], s->ui_skin_enabled ? "ENABLED" : "DISABLED");
  strcpy(values[5], language_name(s->ui_language));
  int_to_str(s->ui_comm_timeout_ms, n); values[6][0] = '\0'; strcat(values[6], n); strcat(values[6], " ms");
  strcpy(values[7], s->conn.connected ? "ONLINE" : "LOST");
  strcpy(values[8], s->ui_auto_request ? "ON" : "OFF");

  if (s->settings_cursor < 0) s->settings_cursor = 0;
  if (s->settings_cursor >= SETTINGS_TOTAL_ROWS) s->settings_cursor = SETTINGS_TOTAL_ROWS - 1;
  int max_scroll = SETTINGS_TOTAL_ROWS - SETTINGS_VISIBLE_ROWS;
  if (max_scroll < 0) max_scroll = 0;
  if (s->settings_scroll < 0) s->settings_scroll = 0;
  if (s->settings_scroll > max_scroll) s->settings_scroll = max_scroll;
  if (s->settings_cursor < s->settings_scroll) s->settings_scroll = s->settings_cursor;
  if (s->settings_cursor >= s->settings_scroll + SETTINGS_VISIBLE_ROWS) {
    s->settings_scroll = s->settings_cursor - SETTINGS_VISIBLE_ROWS + 1;
    if (s->settings_scroll > max_scroll) s->settings_scroll = max_scroll;
  }

  for (int row = 0; row < SETTINGS_VISIBLE_ROWS; row++) {
    int idx = s->settings_scroll + row;
    if (idx >= SETTINGS_TOTAL_ROWS) break;
    int y = 70 + row * 20;
    if (idx == s->settings_cursor) fill_rect(s, 8, y - 2, 460, 16, COLOR_BG_HEADER);
    draw_settings_row(s, y, labels[idx], values[idx], value_colors[idx]);
  }

  char nav_line[40];
  nav_line[0] = '\0';
  strcat(nav_line, "UP/DOWN row ");
  int_to_str(s->settings_cursor + 1, n); strcat(nav_line, n); strcat(nav_line, "/");
  int_to_str(SETTINGS_TOTAL_ROWS, n); strcat(nav_line, n);
  draw_string(s, 12, 248, nav_line, COLOR_TEXT_DIM, 1);

  if (!s->state.valid)
    draw_string(s, 12, 268, "STATE frame pending: showing local defaults", COLOR_ALARM_WARN, 1);
}

static void render_alarms_screen(chip_state_t *s) {
  fill_rect(s, 2, 32, 476, 264, COLOR_BG_PANEL);
  draw_string(s, 12, 40, "ALARM CENTER", COLOR_TEXT_PRIMARY, 2);
  char hdr[64]; hdr[0] = '\0';
  strcat(hdr, "Active:");
  char n[12]; int_to_str(s->alarm_count, n); strcat(hdr, n);
  strcat(hdr, "  Mute:");
  strcat(hdr, s->state.mute ? "ON" : "OFF");
  draw_string(s, 220, 44, hdr, COLOR_TEXT_DIM, 1);

  int active_slots[MAX_ALARMS];
  int active_count = 0;
  for (int i = 0; i < MAX_ALARMS; i++) {
    if (s->alarms[i].id == 0) continue;
    active_slots[active_count++] = i;
  }
  s->alarm_count = active_count;

  if (s->alarm_count == 0) {
    s->alarms_cursor = 0;
    draw_string_centered(s, 240, 124, "No active alarms", COLOR_CONN_OK, 2);
    draw_string_centered(s, 240, 146, "CTRL,ALM data will appear here", COLOR_TEXT_DIM, 1);
    return;
  }

  if (s->alarms_cursor < 0) s->alarms_cursor = 0;
  if (s->alarms_cursor >= s->alarm_count) s->alarms_cursor = s->alarm_count - 1;

  int start = 0;
  if (s->alarms_cursor > 6) start = s->alarms_cursor - 6;
  int max_start = s->alarm_count - 9;
  if (max_start < 0) max_start = 0;
  if (start > max_start) start = max_start;

  int row = 0;
  for (int pos = start; pos < s->alarm_count && row < 9; pos++) {
    int i = active_slots[pos];
    int y = 70 + row * 24;
    if (pos == s->alarms_cursor) fill_rect(s, 6, y - 2, 468, 14, COLOR_BG_HEADER);
    uint32_t ac = s->alarms[i].type == 2 ? COLOR_ALARM_CRIT :
                  s->alarms[i].type == 1 ? COLOR_ALARM_WARN : COLOR_FAN_ON;
    char left[40]; left[0] = '\0';
    strcat(left, "ALM-");
    int_to_str(s->alarms[i].id, n); strcat(left, n);
    strcat(left, " ");
    strcat(left, s->alarms[i].type == 2 ? "CRIT" : s->alarms[i].type == 1 ? "WARN" : "INFO");
    const char *state_txt = s->alarms[i].state == 2 ? "ACK" : s->alarms[i].state == 1 ? "ACTIVE" : "OFF";
    draw_string(s, 10, y, left, ac, 1);
    draw_string(s, 102, y, state_txt, ac, 1);
    draw_string(s, 150, y, s->alarms[i].desc[0] ? s->alarms[i].desc : "(no description)", COLOR_TEXT_SECONDARY, 1);
    row++;
  }

  char nav_line[44], n2[12];
  nav_line[0] = '\0';
  strcat(nav_line, "UP/DOWN alarm ");
  int_to_str(s->alarms_cursor + 1, n2); strcat(nav_line, n2); strcat(nav_line, "/");
  int_to_str(s->alarm_count, n2); strcat(nav_line, n2);
  draw_string(s, 10, 284, nav_line, COLOR_TEXT_DIM, 1);
}

static void render_charts_screen(chip_state_t *s) {
  bool valid = s->telemetry.valid && s->conn.connected;
  fill_rect(s, 2, 32, 476, 264, COLOR_BG_PANEL);
  draw_string(s, 12, 40, "CHARTS", COLOR_TEXT_PRIMARY, 2);
  draw_string(s, 120, 44, "Live bars + trend placeholders", COLOR_TEXT_DIM, 1);

  char val[20];
  draw_string(s, 12, 78, "Air temp", COLOR_TEXT_SECONDARY, 1);
  if (valid) { float_to_str(s->telemetry.air_temp, val, 1); strcat(val, " C"); }
  else strcpy(val, "--.- C");
  draw_string(s, 96, 78, val, COLOR_TEMP_AIR, 1);
  draw_progress_bar(s, 170, 78, 290, 10, valid ? s->telemetry.air_temp : 0.0f, 20.0f, 42.0f, COLOR_TEMP_AIR, COLOR_OFF_GRAY);

  draw_string(s, 12, 114, "Skin temp", COLOR_TEXT_SECONDARY, 1);
  if (valid) { float_to_str(s->telemetry.skin_temp, val, 1); strcat(val, " C"); }
  else strcpy(val, "--.- C");
  draw_string(s, 96, 114, val, COLOR_TEMP_SKIN, 1);
  draw_progress_bar(s, 170, 114, 290, 10, valid ? s->telemetry.skin_temp : 0.0f, 20.0f, 42.0f, COLOR_TEMP_SKIN, COLOR_OFF_GRAY);

  draw_string(s, 12, 150, "Humidity", COLOR_TEXT_SECONDARY, 1);
  if (valid) { float_to_str(s->telemetry.humidity, val, 1); strcat(val, " %"); }
  else strcpy(val, "--.- %");
  draw_string(s, 96, 150, val, COLOR_HUMIDITY, 1);
  draw_progress_bar(s, 170, 150, 290, 10, valid ? s->telemetry.humidity : 0.0f, 0.0f, 100.0f, COLOR_HUMIDITY, COLOR_OFF_GRAY);

  draw_string(s, 12, 196, "Trend history:", COLOR_TEXT_SECONDARY, 1);
  draw_string(s, 102, 196, "N/A in simulator (showing latest sample only)", COLOR_ALARM_WARN, 1);
  draw_string(s, 12, 216, "Future MB stream keys:", COLOR_TEXT_SECONDARY, 1);
  draw_string(s, 140, 216, "air_hist skin_hist hum_hist", COLOR_TEXT_DIM, 1);
}

static void render_pulseoxi_screen(chip_state_t *s) {
  fill_rect(s, 2, 32, 476, 264, COLOR_BG_PANEL);
  draw_string(s, 12, 40, "PULSE OXIMETRY", COLOR_TEXT_PRIMARY, 2);
  draw_string(s, 190, 44, "SpO2/PR not present on CTRL stream", COLOR_TEXT_DIM, 1);

  fill_rect(s, 24, 72, 204, 108, COLOR_BG_HEADER);
  fill_rect(s, 252, 72, 204, 108, COLOR_BG_HEADER);
  draw_string_centered(s, 126, 88, "SpO2", COLOR_TEXT_SECONDARY, 2);
  draw_string_centered(s, 126, 122, "-- %", COLOR_ALARM_WARN, 3);
  draw_string_centered(s, 354, 88, "PULSE", COLOR_TEXT_SECONDARY, 2);
  draw_string_centered(s, 354, 122, "--- bpm", COLOR_ALARM_WARN, 2);

  char line[64], n[20];
  line[0] = '\0';
  strcat(line, "Skin temp ref: ");
  if (s->telemetry.valid) { float_to_str(s->telemetry.skin_temp, n, 1); strcat(line, n); strcat(line, " C"); }
  else strcat(line, "--.- C");
  draw_string(s, 24, 198, line, COLOR_TEXT_SECONDARY, 1);

  line[0] = '\0';
  strcat(line, "Door state: ");
  strcat(line, s->telemetry.door_open ? "OPEN" : "CLOSED");
  strcat(line, "   Alarm code: ");
  int_to_str(s->telemetry.alarm_code, n); strcat(line, n);
  draw_string(s, 24, 214, line, COLOR_TEXT_SECONDARY, 1);
  draw_string(s, 24, 234, "Placeholder shown until MB publishes SpO2/pulse telemetry", COLOR_TEXT_DIM, 1);
}

static void render_lock_screen(chip_state_t *s) {
  fill_rect(s, 2, 32, 476, 264, COLOR_BG_PANEL);
  draw_string_centered(s, 240, 72, "LOCK SCREEN", COLOR_TEXT_PRIMARY, 3);
  if (s->ui_locked) {
    draw_string_centered(s, 240, 126, "CONTROLS LOCKED", COLOR_ALARM_WARN, 2);
    draw_string_centered(s, 240, 154, "NEXT/PREV disabled", COLOR_TEXT_SECONDARY, 1);
    draw_string_centered(s, 240, 170, "Press OK to unlock", COLOR_SETPOINT, 1);
  } else {
    draw_string_centered(s, 240, 126, "Controls currently unlocked", COLOR_CONN_OK, 2);
    draw_string_centered(s, 240, 154, "Press OK to lock navigation", COLOR_TEXT_SECONDARY, 1);
    draw_string_centered(s, 240, 170, "NEXT/PREV keeps browsing screens", COLOR_TEXT_SECONDARY, 1);
  }
  char line[40]; line[0] = '\0';
  strcat(line, "Active alarms: ");
  char n[12]; int_to_str(s->alarm_count, n); strcat(line, n);
  draw_string_centered(s, 240, 212, line, COLOR_TEXT_DIM, 1);
}

static void render_footer(chip_state_t *s) {
  fill_rect(s, 0, FOOTER_Y, DISPLAY_W, FOOTER_H, COLOR_BG_HEADER);
  // Serial number
  char line[80]; line[0] = '\0';
  strcat(line, "SN:");
  strcat(line, s->state.serial_number[0] ? s->state.serial_number : "---");
  strcat(line, "  HW:");
  strcat(line, s->state.hw_number[0] ? s->state.hw_number : "---");
  strcat(line, "  FW:");
  strcat(line, s->state.fw_version[0] ? s->state.fw_version : "---");
  draw_string(s, 8, 299, line, COLOR_TEXT_DIM, 1);
  // Uptime
  uint64_t up_s = (get_sim_nanos() - s->boot_ns) / 1000000000ULL;
  int h = (int)(up_s / 3600); int m = (int)((up_s % 3600) / 60); int sec = (int)(up_s % 60);
  char ut[16];
  ut[0] = '0' + (h/10); ut[1] = '0' + (h%10); ut[2] = ':';
  ut[3] = '0' + (m/10); ut[4] = '0' + (m%10); ut[5] = ':';
  ut[6] = '0' + (sec/10); ut[7] = '0' + (sec%10); ut[8] = '\0';
  draw_string(s, 400, 299, ut, COLOR_TEXT_DIM, 1);
  line[0] = '\0';
  strcat(line, "[");
  strcat(line, screen_name(s->active_screen));
  strcat(line, "] PREV/NEXT=SCREEN  UP/DOWN=VERT  OK=ACTION");
  if (s->ui_locked) strcat(line, " (LOCKED)");
  draw_string(s, 8, 309, line, COLOR_TEXT_DIM, 1);
}

static void render_comm_lost_overlay(chip_state_t *s) {
  // Semi-transparent dark overlay (just darken center area)
  fill_rect(s, 120, 130, 240, 60, 0x000000CC);
  if (s->render.blink_phase)
    draw_string_centered(s, 240, 150, "COMM LOST", COLOR_ALARM_CRIT, 3);
}

static void render_display(chip_state_t *s) {
  fill_rect(s, 0, 0, DISPLAY_W, DISPLAY_H, COLOR_BG_DARK);
  if (s->active_screen == SCREEN_BOOT) {
    render_boot_screen(s);
    render_footer(s);
    return;
  }
  render_header(s);
  switch (s->active_screen) {
    case SCREEN_MAIN:     render_main_screen(s); break;
    case SCREEN_SETTINGS: render_settings_screen(s); break;
    case SCREEN_ALARMS:   render_alarms_screen(s); break;
    case SCREEN_CHARTS:   render_charts_screen(s); break;
    case SCREEN_PULSEOXI: render_pulseoxi_screen(s); break;
    case SCREEN_LOCK:     render_lock_screen(s); break;
    case SCREEN_BOOT:
    default:              render_boot_screen(s); break;
  }
  render_footer(s);
  if (!s->conn.connected && s->conn.msg_count > 0)
    render_comm_lost_overlay(s);
}

// ─── Callbacks ───────────────────────────────────────────────────
static void on_button_change(void *user_data, pin_t pin, uint32_t value) {
  chip_state_t *s = (chip_state_t *)user_data;
  if (value != LOW) return;
  uint64_t now = get_sim_nanos();
  if (now - s->last_btn_ns < BUTTON_DEBOUNCE_NS) return;
  s->last_btn_ns = now;

  if (pin == s->btn_next)
    nav_next(s);
  else if (pin == s->btn_prev)
    nav_prev(s);
  else if (pin == s->btn_ok)
    nav_ok(s);
  else if (pin == s->btn_up)
    nav_up(s);
  else if (pin == s->btn_down)
    nav_down(s);
}

static void on_uart_rx(void *user_data, uint8_t byte) {
  chip_state_t *s = (chip_state_t *)user_data;
  if (byte == '\n' || byte == '\r') {
    if (s->rx_overflow) {
      s->rx_len = 0;
      s->rx_overflow = false;
      return;
    }
    if (s->rx_len > 0) {
      s->rx_buf[s->rx_len] = '\0';
      parse_message(s);
      s->rx_len = 0;
    }
  } else {
    if (s->rx_len < UART_BUF_SIZE - 1)
      s->rx_buf[s->rx_len++] = (char)byte;
    else
      s->rx_overflow = true;
  }
}

static void on_refresh_timer(void *user_data) {
  chip_state_t *s = (chip_state_t *)user_data;
  poll_touch_controls(s);
  if (s->render.needs_redraw) {
    render_display(s);
    buffer_write(s->fb, 0, s->pixels, s->width * s->height * sizeof(uint32_t));
    s->render.needs_redraw = false;
  }
}

static void on_watchdog(void *user_data) {
  chip_state_t *s = (chip_state_t *)user_data;
  uint64_t now = get_sim_nanos();
  uint32_t timeout_ms = (uint32_t)s->ui_comm_timeout_ms;
  uint64_t timeout_ns = (uint64_t)timeout_ms * 1000000ULL;
  if (s->conn.connected && (now - s->conn.last_msg_ns > timeout_ns)) {
    s->conn.connected = false;
    s->render.needs_redraw = true;
  }
  // Blink toggle (~1 Hz via 1s watchdog checking 500ms threshold)
  if (now - s->render.last_blink_ns > 500000000ULL) {
    s->render.blink_phase ^= 1;
    s->render.last_blink_ns = now;
    s->render.needs_redraw = true;
  }
}

static void send_state_request(void *user_data) {
  chip_state_t *s = (chip_state_t *)user_data;
  const char *msg = "HMI,REQ,STATE\n";
  uart_write(s->uart, (uint8_t *)msg, strlen(msg));
}

// ─── Entry Point ─────────────────────────────────────────────────
void chip_init(void) {
  chip_state_t *s = malloc(sizeof(chip_state_t));
  if (!s) { printf("ERROR: malloc chip_state failed\n"); return; }
  memset(s, 0, sizeof(chip_state_t));

  s->fb = framebuffer_init(&s->width, &s->height);
  s->pixels = malloc(s->width * s->height * sizeof(uint32_t));
  if (!s->pixels) { printf("ERROR: malloc framebuffer failed\n"); return; }

  // Attributes
  s->attr_control_mode  = attr_init("controlMode", 0);
  s->attr_language      = attr_init("language", 0);
  s->attr_skin_enabled  = attr_init("skinEnabled", 1);
  s->attr_air_setpoint  = attr_init_float("airSetpoint", 34.0f);
  s->attr_skin_setpoint = attr_init_float("skinSetpoint", 36.5f);
  s->attr_hum_setpoint  = attr_init_float("humSetpoint", 60.0f);
  s->attr_comm_timeout  = attr_init("commTimeoutMs", 3000);
  s->attr_auto_request  = attr_init("autoRequestState", 1);
  s->attr_touch_x       = attr_init("touchX", 400);
  s->attr_touch_y       = attr_init("touchY", 468);
  s->attr_touch_tap     = attr_init("touchTap", 0);

  // Load initial setpoints from attributes
  s->state.air_setpoint  = attr_read_float(s->attr_air_setpoint);
  s->state.skin_setpoint = attr_read_float(s->attr_skin_setpoint);
  s->state.hum_setpoint  = attr_read_float(s->attr_hum_setpoint);
  s->state.control_mode  = (int)attr_read(s->attr_control_mode);
  s->state.actuators_enabled = 1;
  s->state.phototherapy = 0;
  s->state.mute = 0;
  s->state.photo_time_min = 0;
  s->state.skin_enabled = (int)attr_read(s->attr_skin_enabled);
  s->ui_language         = (int)attr_read(s->attr_language);
  s->ui_skin_enabled     = (int)attr_read(s->attr_skin_enabled);
  s->ui_comm_timeout_ms  = (int)attr_read(s->attr_comm_timeout);
  s->ui_auto_request     = (int)attr_read(s->attr_auto_request);
  s->touch_x             = (int)attr_read(s->attr_touch_x);
  s->touch_y             = (int)attr_read(s->attr_touch_y);
  s->last_touch_tap      = attr_read(s->attr_touch_tap) ? 1 : 0;

  // UART
  const uart_config_t uart_cfg = {
    .rx = pin_init("RX", INPUT),
    .tx = pin_init("TX", OUTPUT),
    .baud_rate = 115200,
    .rx_data = on_uart_rx,
    .user_data = s,
  };
  s->uart = uart_init(&uart_cfg);
  s->btn_next = pin_init("BTN_NEXT", INPUT_PULLUP);
  s->btn_prev = pin_init("BTN_PREV", INPUT_PULLUP);
  s->btn_ok   = pin_init("BTN_OK", INPUT_PULLUP);
  s->btn_up   = pin_init("BTN_UP", INPUT_PULLUP);
  s->btn_down = pin_init("BTN_DOWN", INPUT_PULLUP);
  const pin_watch_config_t btn_watch = {
    .edge = BOTH,
    .pin_change = on_button_change,
    .user_data = s,
  };
  pin_watch(s->btn_next, &btn_watch);
  pin_watch(s->btn_prev, &btn_watch);
  pin_watch(s->btn_ok, &btn_watch);
  pin_watch(s->btn_up, &btn_watch);
  pin_watch(s->btn_down, &btn_watch);

  // Boot timestamp & initial render
  s->boot_ns = get_sim_nanos();
  s->conn.last_msg_ns = s->boot_ns;
  s->active_screen = SCREEN_BOOT;
  s->render.needs_redraw = true;
  render_display(s);
  buffer_write(s->fb, 0, s->pixels, s->width * s->height * sizeof(uint32_t));

  // Refresh timer (200ms = 5 FPS, dirty-flag gated)
  const timer_config_t refresh_cfg = {
    .user_data = s,
    .callback = on_refresh_timer,
  };
  s->refresh_timer = timer_init(&refresh_cfg);
  timer_start(s->refresh_timer, 200000, true);

  // Watchdog timer (500ms — comm timeout check + blink toggle)
  const timer_config_t watchdog_cfg = {
    .user_data = s,
    .callback = on_watchdog,
  };
  s->watchdog_timer = timer_init(&watchdog_cfg);
  timer_start(s->watchdog_timer, 500000, true);

  // Auto-request state after 500ms
  if (s->ui_auto_request == 1) {
    const timer_config_t req_cfg = {
      .user_data = s,
      .callback = send_state_request,
    };
    timer_t req_timer = timer_init(&req_cfg);
    timer_start(req_timer, 500000, false);
  }
}
