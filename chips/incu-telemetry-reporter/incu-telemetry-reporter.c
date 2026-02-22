#include "wokwi-api.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Telemetry Reporter Chip
 *
 * - Samples live sensor/actuator signals and emits telemetry JSON
 * - Accepts HMI return commands over UART RX
 * - Mirrors MB-like CTRL,STATE responses to the display
 * - Drives humidifier override output based on humidity setpoint feedback
 */

#define UART_BUF_SIZE 256

typedef struct {
  timer_t timer_id;
  uart_dev_t uart;

  pin_t chamber_in;      // Analog from SHT4x TEMP_OUT
  pin_t skin_in;         // Analog from NTC OUT
  pin_t humidity_in;     // Analog from SHT4x HUM_OUT
  pin_t fan_tach_in;     // Digital tach pulse
  pin_t heater_pwm_in;   // Digital heater PWM
  pin_t door_in;         // Analog door sensor output
  pin_t alarm_in;        // Digital buzzer/alarm drive
  pin_t humidifier_duty_out;    // Analog override to humidifier
  pin_t humidifier_override_en; // Enable override to humidifier

  bool tach_last_level;
  uint64_t last_tach_rise_ns;
  uint64_t last_tx_ns;
  uint64_t last_state_tx_ns;

  float chamber_temp_c;
  float skin_temp_c;
  float humidity_pct;
  float fan_rpm;
  float heater_duty_est;
  float alarm_level;
  float humidifier_duty;
  int door_open;
  int alarm_code;

  // HMI return state (Display -> MB protocol)
  int actuators_enabled;
  int skin_mode;
  int control_mode;
  float air_setpoint;
  float skin_setpoint;
  float hum_setpoint;
  int photo_mode;
  int mute_alarm;
  int language;
  int photo_minutes;
  bool hmi_link_active;

  char rx_buf[UART_BUF_SIZE];
  int rx_len;
  bool rx_overflow;
} chip_state_t;

static float clampf(float x, float min_v, float max_v) {
  if (x < min_v) return min_v;
  if (x > max_v) return max_v;
  return x;
}

static char *next_csv(char **p) {
  if (!*p) return "";
  char *start = *p;
  char *c = strchr(start, ',');
  if (c) {
    *c = '\0';
    *p = c + 1;
  } else {
    *p = NULL;
  }
  return start;
}

// SHT4x TEMP_OUT uses linear mapping: -20..80 C -> 0..3.3 V
static float decode_chamber_temp(float voltage) {
  float t = (voltage / 3.3f) * 100.0f - 20.0f;
  return clampf(t, -20.0f, 80.0f);
}

// SHT4x HUM_OUT uses linear mapping: 0..100 % -> 0..3.3 V
static float decode_humidity(float voltage) {
  float h = (voltage / 3.3f) * 100.0f;
  return clampf(h, 0.0f, 100.0f);
}

// NTC divider inverse (matches incu-ntc-skin model constants)
static float decode_skin_temp(float voltage) {
  const float vcc = 3.3f;
  const float r_fixed = 10000.0f;
  const float r0 = 10000.0f;
  const float beta = 3950.0f;
  const float t0 = 298.15f; // 25 C

  voltage = clampf(voltage, 0.02f, vcc - 0.02f);
  float r_ntc = r_fixed * (vcc / voltage - 1.0f);
  if (r_ntc <= 1.0f) return 60.0f;

  float inv_t = (1.0f / t0) + (logf(r_ntc / r0) / beta);
  float temp_c = (1.0f / inv_t) - 273.15f;
  return clampf(temp_c, -20.0f, 80.0f);
}

static void send_str(chip_state_t *chip, const char *s) {
  uart_write(chip->uart, (uint8_t *)s, strlen(s));
}

static void send_ctrl_state(chip_state_t *chip) {
  char state_line[256];
  snprintf(state_line, sizeof(state_line),
           "CTRL,STATE,%d,%d,%.1f,%.1f,%.1f,%d,%d,SIM-V15,15,A,mode2-hmi,%d,%d,1,%d.00\n",
           chip->actuators_enabled,
           chip->control_mode,
           chip->air_setpoint,
           chip->skin_setpoint,
           chip->hum_setpoint,
           chip->photo_mode,
           chip->mute_alarm,
           chip->alarm_code ? 1 : 0,
           chip->skin_mode,
           chip->photo_minutes);
  send_str(chip, state_line);
  chip->last_state_tx_ns = get_sim_nanos();
}

static void parse_hmi_control(chip_state_t *chip, const char *line) {
  char buf[UART_BUF_SIZE];
  strncpy(buf, line + 4, sizeof(buf) - 1); // Skip "HMI,"
  buf[sizeof(buf) - 1] = '\0';

  char *p = buf;
  chip->actuators_enabled = atoi(next_csv(&p)) ? 1 : 0;
  chip->skin_mode = atoi(next_csv(&p)) ? 1 : 0;
  chip->control_mode = atoi(next_csv(&p)) ? 1 : 0;
  chip->air_setpoint = clampf((float)strtod(next_csv(&p), NULL), 30.0f, 40.0f);
  chip->skin_setpoint = clampf((float)strtod(next_csv(&p), NULL), 34.0f, 39.0f);
  chip->hum_setpoint = clampf((float)strtod(next_csv(&p), NULL), 40.0f, 95.0f);
  chip->photo_mode = atoi(next_csv(&p)) ? 1 : 0;
  chip->mute_alarm = atoi(next_csv(&p)) ? 1 : 0;
  chip->language = atoi(next_csv(&p));
  chip->photo_minutes = atoi(next_csv(&p));
  if (chip->language < 0) chip->language = 0;
  if (chip->language > 3) chip->language = 3;
  if (chip->photo_minutes < 0) chip->photo_minutes = 0;

  chip->hmi_link_active = true;
  send_ctrl_state(chip);
}

static void parse_hmi_line(chip_state_t *chip, const char *line) {
  if (strcmp(line, "HMI,REQ,STATE") == 0) {
    chip->hmi_link_active = true;
    send_ctrl_state(chip);
    return;
  }
  if (strncmp(line, "HMI,", 4) == 0) {
    parse_hmi_control(chip, line);
  }
}

static void on_uart_rx(void *user_data, uint8_t byte) {
  chip_state_t *chip = (chip_state_t *)user_data;

  if (byte == '\r' || byte == '\n') {
    if (chip->rx_overflow) {
      chip->rx_overflow = false;
      chip->rx_len = 0;
      return;
    }
    if (chip->rx_len > 0) {
      chip->rx_buf[chip->rx_len] = '\0';
      parse_hmi_line(chip, chip->rx_buf);
      chip->rx_len = 0;
    }
    return;
  }

  if (chip->rx_len >= UART_BUF_SIZE - 1) {
    chip->rx_overflow = true;
    return;
  }
  chip->rx_buf[chip->rx_len++] = (char)byte;
}

static void update_fan_estimate(chip_state_t *chip, uint64_t now_ns) {
  bool level = (pin_read(chip->fan_tach_in) == HIGH);
  if (level && !chip->tach_last_level) {
    if (chip->last_tach_rise_ns != 0) {
      uint64_t dt_ns = now_ns - chip->last_tach_rise_ns;
      if (dt_ns > 0) {
        // Fan chip emits 2 tach pulses/rev.
        float inst_rpm = 60000000000.0f / ((float)dt_ns * 2.0f);
        inst_rpm = clampf(inst_rpm, 0.0f, 6000.0f);
        chip->fan_rpm = chip->fan_rpm * 0.70f + inst_rpm * 0.30f;
      }
    }
    chip->last_tach_rise_ns = now_ns;
  }
  chip->tach_last_level = level;

  // Decay when pulses stop.
  if (chip->last_tach_rise_ns == 0 ||
      (now_ns - chip->last_tach_rise_ns) > 1200000000ULL) {
    chip->fan_rpm *= 0.85f;
    if (chip->fan_rpm < 5.0f) chip->fan_rpm = 0.0f;
  }
}

static void update_humidifier_override(chip_state_t *chip) {
  if (!chip->hmi_link_active) {
    chip->humidifier_duty = 0.0f;
    pin_write(chip->humidifier_override_en, LOW);
    pin_dac_write(chip->humidifier_duty_out, 0.0f);
    return;
  }

  float duty_target = 0.0f;
  float hum_error = chip->hum_setpoint - chip->humidity_pct;
  if (chip->actuators_enabled && hum_error > 0.5f) {
    duty_target = clampf(20.0f + hum_error * 2.8f, 0.0f, 95.0f);
  }
  chip->humidifier_duty = chip->humidifier_duty * 0.85f + duty_target * 0.15f;
  if (chip->humidifier_duty < 0.5f) chip->humidifier_duty = 0.0f;

  pin_write(chip->humidifier_override_en, HIGH);
  pin_dac_write(chip->humidifier_duty_out,
                (chip->humidifier_duty / 95.0f) * 3.3f);
}

static void on_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  uint64_t now_ns = get_sim_nanos();

  chip->chamber_temp_c = decode_chamber_temp(pin_adc_read(chip->chamber_in));
  chip->skin_temp_c = decode_skin_temp(pin_adc_read(chip->skin_in));
  chip->humidity_pct = decode_humidity(pin_adc_read(chip->humidity_in));

  float heater_sample = (pin_read(chip->heater_pwm_in) == HIGH) ? 1.0f : 0.0f;
  chip->heater_duty_est = chip->heater_duty_est * 0.95f + heater_sample * 0.05f;

  update_fan_estimate(chip, now_ns);

  float door_v = pin_adc_read(chip->door_in);
  chip->door_open = (door_v < 1.0f) ? 1 : 0;

  float alarm_sample = (pin_read(chip->alarm_in) == HIGH) ? 1.0f : 0.0f;
  chip->alarm_level = chip->alarm_level * 0.90f + alarm_sample * 0.10f;
  chip->alarm_code = (chip->alarm_level > 0.20f) ? 1 : 0;

  update_humidifier_override(chip);

  if (chip->last_tx_ns != 0 && (now_ns - chip->last_tx_ns) < 500000000ULL) {
    return;
  }
  chip->last_tx_ns = now_ns;

  char tel[256];
  snprintf(tel, sizeof(tel),
           "{\"temp\":%.1f,\"skin\":%.1f,\"hum\":%.1f,\"fan\":%.0f,\"heater\":%.0f,\"door\":%d,\"alarm\":%d}\n",
           chip->chamber_temp_c,
           chip->skin_temp_c,
           chip->humidity_pct,
           chip->fan_rpm,
           chip->heater_duty_est * 100.0f,
           chip->door_open,
           chip->alarm_code);
  send_str(chip, tel);

  if (chip->hmi_link_active &&
      (chip->last_state_tx_ns == 0 || (now_ns - chip->last_state_tx_ns) >= 1000000000ULL)) {
    send_ctrl_state(chip);
  }

  printf("%s", tel);
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  memset(chip, 0, sizeof(chip_state_t));

  chip->chamber_in = pin_init("CHAMBER_IN", ANALOG);
  chip->skin_in = pin_init("SKIN_IN", ANALOG);
  chip->humidity_in = pin_init("HUMIDITY_IN", ANALOG);
  chip->fan_tach_in = pin_init("FAN_TACH_IN", INPUT);
  chip->heater_pwm_in = pin_init("HEATER_PWM_IN", INPUT);
  chip->door_in = pin_init("DOOR_IN", ANALOG);
  chip->alarm_in = pin_init("ALARM_IN", INPUT);
  chip->humidifier_duty_out = pin_init("HUMIDIFIER_DUTY_OUT", ANALOG);
  chip->humidifier_override_en = pin_init("HUMIDIFIER_OVERRIDE_EN", OUTPUT_LOW);

  chip->actuators_enabled = 1;
  chip->skin_mode = 1;
  chip->control_mode = 0;
  chip->air_setpoint = 34.0f;
  chip->skin_setpoint = 36.5f;
  chip->hum_setpoint = 60.0f;
  chip->photo_mode = 0;
  chip->mute_alarm = 0;
  chip->language = 0;
  chip->photo_minutes = 0;
  chip->hmi_link_active = false;

  const uart_config_t uart_cfg = {
      .rx = pin_init("RX", INPUT),
      .tx = pin_init("TX", OUTPUT),
      .baud_rate = 115200,
      .rx_data = on_uart_rx,
      .user_data = chip,
  };
  chip->uart = uart_init(&uart_cfg);

  const timer_config_t timer_cfg = {
      .callback = on_timer,
      .user_data = chip,
  };
  chip->timer_id = timer_init(&timer_cfg);
  timer_start(chip->timer_id, 10000, true); // 10ms sampling
}
