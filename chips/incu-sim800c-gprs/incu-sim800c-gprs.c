#include "wokwi-api.h"
#include <stdlib.h>
#include <string.h>

/* IncuNest SIM800C GPRS – Wokwi Custom Chip
 * Simulates SIMCom SIM800C GSM/GPRS module via UART AT commands. */

// 5×7 font (ASCII 32-90, column-major, LSB at top) – two entries per line
static const uint8_t font5x7[][5] = {
  {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},
  {0x00,0x07,0x00,0x07,0x00},{0x14,0x7F,0x14,0x7F,0x14},
  {0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},
  {0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},
  {0x00,0x1C,0x22,0x41,0x00},{0x00,0x41,0x22,0x1C,0x00},
  {0x08,0x2A,0x1C,0x2A,0x08},{0x08,0x08,0x3E,0x08,0x08},
  {0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},
  {0x00,0x60,0x60,0x00,0x00},{0x20,0x10,0x08,0x04,0x02},
  {0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},
  {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},
  {0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},
  {0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
  {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},
  {0x00,0x36,0x36,0x00,0x00},{0x00,0x56,0x36,0x00,0x00},
  {0x00,0x08,0x14,0x22,0x41},{0x14,0x14,0x14,0x14,0x14},
  {0x41,0x22,0x14,0x08,0x00},{0x02,0x01,0x51,0x09,0x06},
  {0x32,0x49,0x79,0x41,0x3E},{0x7E,0x11,0x11,0x11,0x7E},
  {0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
  {0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},
  {0x7F,0x09,0x09,0x09,0x01},{0x3E,0x41,0x41,0x51,0x32},
  {0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},
  {0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},
  {0x7F,0x40,0x40,0x40,0x40},{0x7F,0x02,0x04,0x02,0x7F},
  {0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
  {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},
  {0x7F,0x09,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},
  {0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},
  {0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},
  {0x63,0x14,0x08,0x14,0x63},{0x07,0x08,0x70,0x08,0x07},
  {0x61,0x51,0x49,0x45,0x43},
};

#define CMD_BUF   256
#define COL_BG    0x1A1A2EFF
#define COL_HDR   0x0F3460FF
#define COL_WHT   0xFFFFFFFF
#define COL_GRN   0x4CAF50FF
#define COL_YEL   0xFFC107FF
#define COL_RED   0xF44336FF
#define COL_DIM   0x607D8BFF
#define COL_BARBG 0x334155FF
#define COL_BAR   0x42A5F5FF

enum sim_state { ST_OFF=0, ST_ON, ST_REG, ST_GPRS, ST_HTTP };

typedef struct {
  uart_dev_t uart;  buffer_t fb;
  uint32_t w, h;    uint32_t *px;
  char cmd[CMD_BUF]; int cmd_len;
  bool echo;        enum sim_state state;
  pin_t pwrkey;     uint64_t pwrkey_ns; bool pwrkey_low;
  uint32_t a_sig, a_reg, a_gprs, a_http;
  int http_method, http_code;
  bool http_init;
  int cipsend_exp, cipsend_rcv; bool cipsend_mode;
  timer_t t_http, t_cip, t_refresh;
  bool dirty;
} chip_t;

static void int_to_str(int v, char *b) {
  if (v < 0) { *b++ = '-'; v = -v; }
  char t[12]; int n = 0;
  if (v == 0) t[n++] = '0';
  else while (v > 0) { t[n++] = '0' + (v % 10); v /= 10; }
  for (int i = n-1; i >= 0; i--) *b++ = t[i];
  *b = '\0';
}

static void tx(chip_t *s, const char *str) {
  uart_write(s->uart, (uint8_t*)str, strlen(str));
}

static void tx_resp(chip_t *s, const char *body) {
  tx(s, "\r\n"); tx(s, body); tx(s, "\r\n");
}

static void tx_ok(chip_t *s) { tx_resp(s, "OK"); }

static char up(char c) { return (c>='a' && c<='z') ? c-32 : c; }

static int prefix(const char *s, const char *p) {
  while (*p) { if (up(*s) != up(*p)) return 1; s++; p++; }
  return 0;
}

// ─── Drawing ─────────────────────────────────────────────────────
static void px_set(chip_t *s, int x, int y, uint32_t c) {
  if (x>=0 && x<(int)s->w && y>=0 && y<(int)s->h)
    s->px[y*s->w+x] = c;
}

static void fill(chip_t *s, int x, int y, int w, int h, uint32_t c) {
  for (int j=y; j<y+h; j++) for (int i=x; i<x+w; i++) px_set(s,i,j,c);
}

static void glyph(chip_t *s, int x, int y, char ch, uint32_t c) {
  if (ch>='a' && ch<='z') ch-=32;
  if (ch<32 || ch>'Z') ch='?';
  const uint8_t *g = font5x7[ch-32];
  for (int i=0;i<5;i++) for (int j=0;j<7;j++)
    if (g[i]&(1<<j)) px_set(s,x+i,y+j,c);
}

static void text(chip_t *s, int x, int y, const char *str, uint32_t c) {
  while (*str) { glyph(s,x,y,*str++,c); x+=6; }
}

static void sig_bars(chip_t *s, int x, int y, int csq) {
  int bars = (csq>=25)?5:(csq>=20)?4:(csq>=15)?3:(csq>=10)?2:(csq>=2)?1:0;
  for (int i=0; i<5; i++) {
    int bh = 3+i*2, by = y+12-bh;
    fill(s, x+i*5, by, 4, bh, (i<bars)?COL_BAR:COL_BARBG);
  }
}

static void render(chip_t *s) {
  fill(s, 0,0, s->w, s->h, COL_BG);
  if (s->state == ST_OFF) { text(s,30,26,"SIM800C OFF",COL_DIM); return; }

  fill(s, 0,0, s->w, 12, COL_HDR);
  text(s, 2,2, "SIM800C", COL_WHT);
  int csq = (int)attr_read(s->a_sig);
  sig_bars(s, 108, 0, csq);

  int reg = (int)attr_read(s->a_reg);
  text(s, 2,16, (reg==1||reg==5)?"REG:OK":"REG:NO", (reg==1||reg==5)?COL_GRN:COL_RED);

  int gprs = (int)attr_read(s->a_gprs);
  text(s, 72,16, (gprs==1 && s->state>=ST_GPRS)?"GPRS:ON":"GPRS:--",
       (gprs==1 && s->state>=ST_GPRS)?COL_GRN:COL_DIM);

  char buf[20]="CSQ:"; char n[8]; int_to_str(csq,n); strcat(buf,n);
  text(s, 2,28, buf, COL_WHT);

  const char *sl[] = {"???","PWR ON","REG'D","GPRS","HTTP"};
  text(s, 72,28, sl[s->state], COL_YEL);
  text(s, 2,40, "OP:SIMULATED", COL_DIM);
  if (s->http_init) text(s, 2,50, "HTTP:INIT", COL_GRN);
}

// ─── AT Command Processing ───────────────────────────────────────
static void process_cmd(chip_t *s) {
  char *c = s->cmd;
  if (s->echo) { tx(s, c); tx(s, "\r\n"); }
  if (s->state == ST_OFF) return;

  if (prefix(c,"AT")==0 && c[2]=='\0')                { tx_ok(s); return; }
  if (prefix(c,"ATE0")==0) { s->echo=false;             tx_ok(s); return; }
  if (prefix(c,"ATE1")==0) { s->echo=true;              tx_ok(s); return; }

  if (prefix(c,"AT+GMR")==0) {
    tx_resp(s,"Revision:SIM800C_v1.0"); tx(s,"\r\nOK\r\n"); return;
  }
  if (prefix(c,"AT+CSQ")==0) {
    char r[32]="+CSQ: "; char n[8]; int_to_str((int)attr_read(s->a_sig),n);
    strcat(r,n); strcat(r,",0");
    tx_resp(s,r); tx(s,"\r\nOK\r\n"); return;
  }
  if (prefix(c,"AT+CREG?")==0) {
    int reg=(int)attr_read(s->a_reg);
    char r[32]="+CREG: 0,"; char n[8]; int_to_str(reg,n); strcat(r,n);
    tx_resp(s,r); tx(s,"\r\nOK\r\n");
    if ((reg==1||reg==5) && s->state<ST_REG) s->state=ST_REG;
    return;
  }
  if (prefix(c,"AT+CGATT?")==0) {
    int g=(int)attr_read(s->a_gprs);
    char r[32]="+CGATT: "; char n[8]; int_to_str(g,n); strcat(r,n);
    tx_resp(s,r); tx(s,"\r\nOK\r\n");
    if (g==1 && s->state>=ST_REG) s->state=ST_GPRS;
    return;
  }
  if (prefix(c,"AT+COPS?")==0) {
    tx_resp(s,"+COPS: 0,0,\"Simulated\""); tx(s,"\r\nOK\r\n"); return;
  }
  if (prefix(c,"AT+CPIN?")==0) {
    tx_resp(s,"+CPIN: READY"); tx(s,"\r\nOK\r\n"); return;
  }
  if (prefix(c,"AT+CFUN=")==0)  { tx_ok(s); return; }
  if (prefix(c,"AT+SAPBR=")==0) { tx_ok(s); return; }
  if (prefix(c,"AT+CGATT=")==0) { tx_ok(s); return; }
  if (prefix(c,"AT+CIPMUX=")==0){ tx_ok(s); return; }

  // HTTP
  if (prefix(c,"AT+HTTPINIT")==0) {
    s->http_init=true; s->state=ST_HTTP; s->dirty=true; tx_ok(s); return;
  }
  if (prefix(c,"AT+HTTPPARA=")==0) { tx_ok(s); return; }
  if (prefix(c,"AT+HTTPACTION=")==0) {
    s->http_method = c[14]-'0';
    s->http_code = (int)attr_read(s->a_http);
    tx_ok(s);
    timer_start(s->t_http, 1500000, false);
    return;
  }
  if (prefix(c,"AT+HTTPREAD")==0) {
    const char *body="{\"status\":\"ok\"}";
    char r[48]="+HTTPREAD: "; char n[8]; int_to_str((int)strlen(body),n);
    strcat(r,n); tx_resp(s,r); tx(s,body); tx(s,"\r\nOK\r\n"); return;
  }
  if (prefix(c,"AT+HTTPTERM")==0) {
    s->http_init=false;
    if (s->state==ST_HTTP) s->state=ST_GPRS;
    s->dirty=true; tx_ok(s); return;
  }

  // TCP/IP
  if (prefix(c,"AT+CIPSTART=")==0) {
    tx_ok(s); timer_start(s->t_cip, 1000000, false); return;
  }
  if (prefix(c,"AT+CIPSEND=")==0) {
    char *p=c+11; s->cipsend_exp=0;
    while (*p>='0' && *p<='9') { s->cipsend_exp = s->cipsend_exp*10+(*p-'0'); p++; }
    s->cipsend_rcv=0; s->cipsend_mode=true;
    tx(s, "\r\n> "); return;
  }

  tx_resp(s, "ERROR");
}

// ─── Callbacks ───────────────────────────────────────────────────
static void on_rx(void *ud, uint8_t byte) {
  chip_t *s = (chip_t*)ud;
  if (s->state == ST_OFF) return;
  if (s->cipsend_mode) {
    if (++s->cipsend_rcv >= s->cipsend_exp) {
      s->cipsend_mode=false; tx(s,"\r\nSEND OK\r\n");
    }
    return;
  }
  if (byte=='\r' || byte=='\n') {
    if (s->cmd_len > 0) {
      s->cmd[s->cmd_len]='\0'; process_cmd(s); s->cmd_len=0;
    }
  } else if (s->cmd_len < CMD_BUF-1) {
    s->cmd[s->cmd_len++] = (char)byte;
  }
}

static void on_http_action(void *ud) {
  chip_t *s = (chip_t*)ud;
  char r[48]="+HTTPACTION: "; char n[8];
  int_to_str(s->http_method,n); strcat(r,n); strcat(r,",");
  int_to_str(s->http_code,n);   strcat(r,n); strcat(r,",15");
  tx(s,"\r\n"); tx(s,r); tx(s,"\r\n");
}

static void on_cipstart(void *ud) { tx((chip_t*)ud, "\r\nCONNECT OK\r\n"); }

static void on_refresh(void *ud) {
  chip_t *s = (chip_t*)ud;
  s->dirty = true;
  render(s);
  buffer_write(s->fb, 0, s->px, s->w * s->h * 4);
  s->dirty = false;
}

static void on_pwrkey(void *ud, pin_t pin, uint32_t val) {
  chip_t *s = (chip_t*)ud;
  uint64_t now = get_sim_nanos();
  if (val==LOW && !s->pwrkey_low) {
    s->pwrkey_ns = now; s->pwrkey_low = true;
  } else if (val==HIGH && s->pwrkey_low) {
    s->pwrkey_low = false;
    if (now - s->pwrkey_ns >= 1000000000ULL) {
      if (s->state == ST_OFF) {
        s->state = ST_ON;
        int reg = (int)attr_read(s->a_reg);
        if (reg==1||reg==5) { s->state=ST_REG;
          if ((int)attr_read(s->a_gprs)==1) s->state=ST_GPRS;
        }
      } else {
        s->state = ST_OFF;
        s->http_init = false; s->cipsend_mode = false;
      }
      s->dirty = true;
    }
  }
}

// ─── Entry Point ─────────────────────────────────────────────────
void chip_init(void) {
  chip_t *s = malloc(sizeof(chip_t));
  if (!s) return;
  memset(s, 0, sizeof(chip_t));

  s->a_sig  = attr_init("signalStrength", 20);
  s->a_reg  = attr_init("networkRegistered", 1);
  s->a_gprs = attr_init("gprsAttached", 1);
  s->a_http = attr_init("httpResponseCode", 200);

  s->fb = framebuffer_init(&s->w, &s->h);
  s->px = malloc(s->w * s->h * 4);
  if (!s->px) return;

  const uart_config_t ucfg = {
    .rx=pin_init("RX",INPUT), .tx=pin_init("TX",OUTPUT),
    .baud_rate=115200, .rx_data=on_rx, .user_data=s,
  };
  s->uart = uart_init(&ucfg);

  s->pwrkey = pin_init("PWRKEY", INPUT_PULLUP);
  const pin_watch_config_t pw = { .user_data=s, .edge=BOTH, .pin_change=on_pwrkey };
  pin_watch(s->pwrkey, &pw);

  // Start powered on, auto-advance state
  s->state = ST_ON; s->echo = true;
  int reg = (int)attr_read(s->a_reg);
  if (reg==1||reg==5) { s->state=ST_REG;
    if ((int)attr_read(s->a_gprs)==1) s->state=ST_GPRS;
  }

  const timer_config_t tc1 = { .user_data=s, .callback=on_http_action };
  s->t_http = timer_init(&tc1);
  const timer_config_t tc2 = { .user_data=s, .callback=on_cipstart };
  s->t_cip = timer_init(&tc2);
  const timer_config_t tc3 = { .user_data=s, .callback=on_refresh };
  s->t_refresh = timer_init(&tc3);
  timer_start(s->t_refresh, 500000, true);

  s->dirty = true;
  render(s);
  buffer_write(s->fb, 0, s->px, s->w * s->h * 4);
}
