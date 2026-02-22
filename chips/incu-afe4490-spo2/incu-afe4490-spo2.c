/* ── IncuNest AFE4490 SpO2 Pulse Oximeter – Wokwi custom chip ──────
 *
 *  Simulates the TI AFE4490 analog front-end (U17 on IncuNest PCB).
 *  Provides 22-bit ADC readings for Red (LED1) and IR (LED2) channels
 *  with a pulsatile waveform derived from configurable SpO2 and HR.
 *
 *  SPI mode 0 (CPOL=0, CPHA=0):
 *    Write: addr byte (bit7=0) + 3 data bytes
 *    Read:  addr byte (bit7=1) + 3 bytes returned on MISO
 *
 *  ADC_RDY pulses low every 10 ms (100 samples/s).
 * ────────────────────────────────────────────────────────────────── */

#include "wokwi-api.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ── Register map ─────────────────────────────────────────────── */
#define REG_COUNT       0x32  /* 0x00 .. 0x31 = 50 registers       */

#define REG_CONTROL0    0x00  /* SW reset, diag, timer enable       */
#define REG_LED2STC     0x01
#define REG_LED2ENDC    0x02
#define REG_LED2LEDSTC  0x03
#define REG_LED2LEDENDC 0x04
#define REG_ALED2STC    0x05
#define REG_ALED2ENDC   0x06
#define REG_LED1STC     0x07
#define REG_LED1ENDC    0x08
#define REG_LED1LEDSTC  0x09
#define REG_LED1LEDENDC 0x0A
#define REG_ALED1STC    0x0B
#define REG_ALED1ENDC   0x0C
#define REG_LED2CONVST  0x0D
#define REG_LED2CONVEND 0x0E
#define REG_ALED2CONVST 0x0F
#define REG_ALED2CONVEND 0x10
#define REG_LED1CONVST  0x11
#define REG_LED1CONVEND 0x12
#define REG_ALED1CONVST 0x13
#define REG_ALED1CONVEND 0x14
#define REG_ADCRSTSTCT0 0x15
#define REG_ADCRSTENDCT0 0x16
#define REG_ADCRSTSTCT1 0x17
#define REG_ADCRSTENDCT1 0x18
#define REG_ADCRSTSTCT2 0x19
#define REG_ADCRSTENDCT2 0x1A
#define REG_ADCRSTSTCT3 0x1B
#define REG_ADCRSTENDCT3 0x1C
#define REG_PRPCOUNT    0x1D
#define REG_CONTROL1    0x1E
#define REG_SPARE1      0x1F
#define REG_TIAGAIN     0x20
#define REG_TIA_AMB     0x21
#define REG_LEDCNTRL    0x22
#define REG_CONTROL2    0x23
#define REG_SPARE2      0x24
#define REG_SPARE3      0x25
#define REG_SPARE4      0x26
#define REG_SPARE5      0x27
#define REG_SPARE6      0x28
#define REG_ALARM       0x29
#define REG_LED2VAL     0x2A  /* IR ADC data       (read-only)     */
#define REG_ALED2VAL    0x2B  /* Ambient IR        (read-only)     */
#define REG_LED1VAL     0x2C  /* Red ADC data      (read-only)     */
#define REG_ALED1VAL    0x2D  /* Ambient Red       (read-only)     */
#define REG_LED2_ALED2  0x2E  /* IR – Ambient IR   (read-only)     */
#define REG_LED1_ALED1  0x2F  /* Red – Ambient Red (read-only)     */
#define REG_DIAG        0x30
#define REG_PDNCYCLESTC 0x31

/* ── Constants ────────────────────────────────────────────────── */
#define ADC_IR_DC       2000000u  /* IR baseline  (~half of 22-bit) */
#define ADC_RED_DC      1800000u  /* Red baseline                   */
#define ADC_AMBIENT     5000u     /* Ambient light floor            */
#define SAMPLE_RATE_US  10000u    /* 10 ms → 100 Hz                 */
#define RDY_PULSE_US    100u      /* ADC_RDY low pulse width        */

/* ── SPI transaction phase ────────────────────────────────────── */
typedef enum { PHASE_ADDR, PHASE_DATA } spi_phase_t;

/* ── Chip state ───────────────────────────────────────────────── */
typedef struct {
  pin_t       cs_pin;
  pin_t       adc_rdy_pin;
  spi_dev_t   spi;
  spi_phase_t phase;
  uint8_t     spi_buf[4];
  uint8_t     reg_addr;
  bool        is_read;
  bool        cs_active;

  uint32_t    regs[REG_COUNT];

  uint32_t    attr_spo2;
  uint32_t    attr_hr;
  uint32_t    attr_quality;

  uint32_t    sample_count;
  uint32_t    lfsr;          /* 16-bit LFSR for pseudo-random      */

  timer_t     adc_timer;
  timer_t     rdy_timer;
} chip_state_t;

/* ── Helpers ──────────────────────────────────────────────────── */

static uint32_t rand16(chip_state_t *c) {
  uint32_t s = c->lfsr;
  uint32_t bit = ((s) ^ (s >> 2) ^ (s >> 3) ^ (s >> 5)) & 1u;
  c->lfsr = (s >> 1) | (bit << 15);
  return c->lfsr & 0xFFFFu;
}

/* Piecewise-linear approximation of sin(2π·p/1000), returns –1000…+1000 */
static int32_t wave(uint32_t p1000) {
  uint32_t p = p1000 % 1000u;
  if (p <= 250u) return (int32_t)(p * 4);
  if (p <= 500u) return (int32_t)((500u - p) * 4);
  if (p <= 750u) return -(int32_t)((p - 500u) * 4);
  return -(int32_t)((1000u - p) * 4);
}

static uint32_t clamp22(int32_t v) {
  if (v < 0) return 0u;
  if (v > 0x3FFFFF) return 0x3FFFFFu;
  return (uint32_t)v;
}

/* ── ADC value generation ─────────────────────────────────────── */

static void update_adc(chip_state_t *c) {
  uint32_t spo2    = attr_read(c->attr_spo2);
  uint32_t hr      = attr_read(c->attr_hr);
  uint32_t quality = attr_read(c->attr_quality);

  /* Small random jitter */
  uint32_t rnd = rand16(c);
  int32_t s_var = (int32_t)(rnd % 3u) - 1;          /* ±1 SpO2       */
  int32_t h_var = (int32_t)((rnd >> 4) % 5u) - 2;   /* ±2 BPM        */

  int32_t eff_spo2 = (int32_t)spo2 + s_var;
  if (eff_spo2 < 70)  eff_spo2 = 70;
  if (eff_spo2 > 100) eff_spo2 = 100;

  int32_t eff_hr = (int32_t)hr + h_var;
  if (eff_hr < 40)  eff_hr = 40;
  if (eff_hr > 220) eff_hr = 220;

  /* Phase (0-999 ≡ one cardiac cycle).
   * Phase increment per sample = HR / 6  (at 100 Hz). */
  uint32_t ph_inc = (uint32_t)eff_hr / 6u;
  if (ph_inc == 0u) ph_inc = 1u;
  uint32_t phase = (c->sample_count * ph_inc) % 1000u;

  /* R = (110 − SpO2) / 25 ;  R×1000 for integer math */
  int32_t r1000 = (110 - eff_spo2) * 40;
  if (r1000 < 0) r1000 = 0;

  /* Modulation in ppm (parts-per-million), scaled by quality */
  uint32_t ir_mod  = 20000u * quality / 100u;        /* ~2 % nominal  */
  uint32_t red_mod = (uint32_t)r1000 * ir_mod / 1000u;

  int32_t w = wave(phase);

  /* AC component: dc * mod / 1e6 * w / 1000.
   * Use 64-bit intermediate to avoid overflow.              */
  int32_t ir_ac  = (int32_t)((int64_t)ADC_IR_DC  * ir_mod  / 1000000 * w / 1000);
  int32_t red_ac = (int32_t)((int64_t)ADC_RED_DC * red_mod / 1000000 * w / 1000);

  /* Sensor noise */
  int32_t n_ir  = (int32_t)(rand16(c) & 0x1FFu) - 256;
  int32_t n_red = (int32_t)(rand16(c) & 0x1FFu) - 256;

  uint32_t ir_val  = clamp22((int32_t)ADC_IR_DC  + ir_ac  + n_ir);
  uint32_t red_val = clamp22((int32_t)ADC_RED_DC + red_ac + n_red);
  uint32_t amb_ir  = clamp22((int32_t)ADC_AMBIENT + (int32_t)(rand16(c) & 0x7Fu));
  uint32_t amb_red = clamp22((int32_t)ADC_AMBIENT + (int32_t)(rand16(c) & 0x7Fu));

  c->regs[REG_LED2VAL]    = ir_val;
  c->regs[REG_ALED2VAL]   = amb_ir;
  c->regs[REG_LED1VAL]    = red_val;
  c->regs[REG_ALED1VAL]   = amb_red;
  c->regs[REG_LED2_ALED2] = clamp22((int32_t)ir_val  - (int32_t)amb_ir);
  c->regs[REG_LED1_ALED1] = clamp22((int32_t)red_val - (int32_t)amb_red);

  c->sample_count++;
}

/* ── Timer callbacks ──────────────────────────────────────────── */

static void on_adc_timer(void *ud) {
  chip_state_t *c = (chip_state_t *)ud;
  update_adc(c);
  pin_write(c->adc_rdy_pin, LOW);
  timer_start(c->rdy_timer, RDY_PULSE_US, false);
}

static void on_rdy_pulse_end(void *ud) {
  chip_state_t *c = (chip_state_t *)ud;
  pin_write(c->adc_rdy_pin, HIGH);
}

/* ── SPI callbacks ────────────────────────────────────────────── */

static void on_spi_done(void *ud, uint8_t *buf, uint32_t count) {
  chip_state_t *c = (chip_state_t *)ud;
  if (!c->cs_active) return;

  if (c->phase == PHASE_ADDR && count >= 1u) {
    c->reg_addr = buf[0] & 0x3Fu;
    c->is_read  = (buf[0] & 0x80u) != 0u;
    c->phase    = PHASE_DATA;

    if (c->is_read && c->reg_addr < REG_COUNT) {
      uint32_t v = c->regs[c->reg_addr];
      c->spi_buf[0] = (uint8_t)(v >> 16);
      c->spi_buf[1] = (uint8_t)(v >> 8);
      c->spi_buf[2] = (uint8_t)(v);
    } else {
      c->spi_buf[0] = 0u;
      c->spi_buf[1] = 0u;
      c->spi_buf[2] = 0u;
    }
    spi_start(c->spi, c->spi_buf, 3);
    return;
  }

  if (c->phase == PHASE_DATA && count >= 3u && !c->is_read) {
    /* Write — protect read-only data registers 0x2A-0x2F */
    if (c->reg_addr < REG_LED2VAL || c->reg_addr > REG_LED1_ALED1) {
      if (c->reg_addr < REG_COUNT) {
        uint32_t v = ((uint32_t)buf[0] << 16) |
                     ((uint32_t)buf[1] << 8)  |
                     ((uint32_t)buf[2]);
        c->regs[c->reg_addr] = v;

        /* SW reset: CONTROL0 bit 3 */
        if (c->reg_addr == REG_CONTROL0 && (v & 0x08u)) {
          memset(c->regs, 0, sizeof(c->regs));
          c->sample_count = 0;
        }
      }
    }
  }
}

/* ── CS pin watcher ───────────────────────────────────────────── */

static void on_cs_change(void *ud, pin_t pin, uint32_t value) {
  chip_state_t *c = (chip_state_t *)ud;
  (void)pin;

  if (value == LOW) {
    c->cs_active = true;
    c->phase     = PHASE_ADDR;
    c->spi_buf[0] = 0u;
    spi_start(c->spi, c->spi_buf, 1);
  } else {
    c->cs_active = false;
    spi_stop(c->spi);
  }
}

/* ── Chip initialisation ─────────────────────────────────────── */

void chip_init(void) {
  chip_state_t *c = malloc(sizeof(chip_state_t));
  memset(c, 0, sizeof(chip_state_t));

  /* Attributes */
  c->attr_spo2    = attr_init("spo2", 97);
  c->attr_hr      = attr_init("heartRate", 120);
  c->attr_quality = attr_init("signalQuality", 90);

  c->lfsr = 0xACE1u;

  /* Sensible power-on defaults for timing registers */
  c->regs[REG_PRPCOUNT]  = 0x001F3F;   /* ~8000 clocks → 500 Hz PRF */
  c->regs[REG_CONTROL1]  = 0x000101;   /* Timer enable               */
  c->regs[REG_TIAGAIN]   = 0x000000;
  c->regs[REG_LEDCNTRL]  = 0x011414;   /* LED current ~50 mA each    */
  c->regs[REG_CONTROL2]  = 0x000000;

  /* Pins */
  c->cs_pin      = pin_init("CS", INPUT_PULLUP);
  c->adc_rdy_pin = pin_init("ADC_RDY", OUTPUT_HIGH);

  /* SPI slave — mode 0 (CPOL=0, CPHA=0) */
  const spi_config_t spi_cfg = {
    .user_data = c,
    .sck  = pin_init("SCK",  INPUT),
    .mosi = pin_init("MOSI", INPUT),
    .miso = pin_init("MISO", OUTPUT),
    .mode = 0,
    .done = on_spi_done,
  };
  c->spi = spi_init(&spi_cfg);

  /* CS edge watcher */
  const pin_watch_config_t cs_watch = {
    .user_data   = c,
    .edge        = BOTH,
    .pin_change  = on_cs_change,
  };
  pin_watch(c->cs_pin, &cs_watch);

  /* ADC sample timer: 100 Hz */
  const timer_config_t adc_tcfg = { .user_data = c, .callback = on_adc_timer };
  c->adc_timer = timer_init(&adc_tcfg);
  timer_start(c->adc_timer, SAMPLE_RATE_US, true);

  /* ADC_RDY pulse-end timer (one-shot, started by adc_timer) */
  const timer_config_t rdy_tcfg = { .user_data = c, .callback = on_rdy_pulse_end };
  c->rdy_timer = timer_init(&rdy_tcfg);

  /* Seed initial ADC values */
  update_adc(c);
}
