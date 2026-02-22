# IncuNest Display HMI — Complete Technical Design

> Wokwi custom chip simulating the CrowPanel 7.0" (800×480) HMI for the IncuNest neonatal incubator.
> Since Wokwi doesn't support RGB parallel displays, this chip uses the framebuffer API to render a pixel-accurate medical dashboard.

---

## 1. Display Resolution & Memory Budget

### Resolution Decision

| Option | Pixels | Memory (RGBA) | Wokwi WASM Impact |
|--------|--------|---------------|-------------------|
| 320×240 (current) | 76,800 | 307,200 B (300 KB) | ✅ Safe |
| **480×320 (recommended)** | **153,600** | **614,400 B (600 KB)** | ✅ Feasible |
| 800×480 (real HW) | 384,000 | 1,536,000 B (1.5 MB) | ⚠️ Heavy |

**Recommendation: 480×320 pixels.**

Wokwi WASM chips run with linear memory default of 1–4 MB. At 600 KB for the framebuffer + ~50 KB for chip state + font data, we use < 700 KB total — well within the default 1 MB WASM memory page limit without custom memory configuration. The 800×480 option is technically feasible but wastes resolution on Wokwi's simulator viewport (display widgets render at ~300-400px wide on screen).

### Pixel Format

```
RGBA 32-bit: 0xRRGGBBAA
- R: bits 24-31
- G: bits 16-23
- B: bits 8-15
- A: bits 0-7 (0xFF = opaque)
```

All colors below include `0xFF` alpha channel.

---

## 2. Color Scheme — Medical Device Palette

Designed for IEC 60601 medical device compliance aesthetics: high contrast, accessible, professional.

```c
// ─── Background & Structure ───────────────────────
#define COLOR_BG_DARK       0x1A1A2EFF  // Main background (dark navy)
#define COLOR_BG_PANEL      0x16213EFF  // Panel backgrounds
#define COLOR_BG_HEADER     0x0F3460FF  // Header bar
#define COLOR_BORDER        0x334155FF  // Panel borders

// ─── Text ─────────────────────────────────────────
#define COLOR_TEXT_PRIMARY   0xFFFFFFFF  // Primary text (white)
#define COLOR_TEXT_SECONDARY 0xB0BEC5FF  // Secondary/labels (gray)
#define COLOR_TEXT_DIM       0x607D8BFF  // Disabled/dim text

// ─── Telemetry Values ─────────────────────────────
#define COLOR_TEMP_AIR      0x42A5F5FF  // Air temp (cool blue)
#define COLOR_TEMP_SKIN     0xFF7043FF  // Skin temp (warm orange)
#define COLOR_HUMIDITY      0x26C6DAFF  // Humidity (cyan)
#define COLOR_SETPOINT      0x66BB6AFF  // Setpoint values (green)

// ─── Status Indicators ────────────────────────────
#define COLOR_ON_GREEN      0x4CAF50FF  // Active/OK
#define COLOR_OFF_GRAY      0x455A64FF  // Inactive
#define COLOR_HEATER_ON     0xFF5722FF  // Heater active (red-orange)
#define COLOR_FAN_ON        0x29B6F6FF  // Fan active (light blue)
#define COLOR_PHOTO_ON      0xFFEB3BFF  // Phototherapy (yellow)

// ─── Alarms ───────────────────────────────────────
#define COLOR_ALARM_CRIT    0xF44336FF  // Critical alarm (red)
#define COLOR_ALARM_WARN    0xFFA726FF  // Warning alarm (amber)
#define COLOR_ALARM_INFO    0x29B6F6FF  // Info alarm (blue)
#define COLOR_ALARM_BG      0x3E1111FF  // Alarm panel background

// ─── Connection ───────────────────────────────────
#define COLOR_CONN_OK       0x4CAF50FF  // Connected (green)
#define COLOR_CONN_LOST     0xF44336FF  // Disconnected (red)
#define COLOR_CONN_WAIT     0xFFC107FF  // Waiting (amber pulse)
```

---

## 3. Screen Layout (480×320 pixels)

### 3.1 Zone Map

```
┌─────────────────────────────────────────────────────────────────┐
│ HEADER BAR                                              y:0-29  │
│  IncuNest ▪ AIR MODE ▪ ● Connected         480 × 30px          │
├────────────────────────────────┬────────────────────────────────┤
│ LEFT PANEL (AIR TEMP)         │ RIGHT PANEL (SKIN TEMP)  y:32  │
│  x:0-235                      │  x:245-479                     │
│                                │                                │
│   AIR TEMP                     │   SKIN TEMP                   │
│   ┌─────────┐                  │   ┌─────────┐                 │
│   │  34.2°C │ ← detected      │   │  36.6°C │                 │
│   └─────────┘                  │   └─────────┘                 │
│   SET: 34.0°C                  │   SET: 36.5°C                 │
│                                │                                │
│                         y:145  │                         y:145  │
├────────────────────────────────┴────────────────────────────────┤
│ HUMIDITY BAR                                         y:148-187  │
│  ◈ HUMIDITY  58.0%   SET: 60%         480 × 40px               │
├────────────────────────────────────────────────────────────────┤
│ ACTUATOR STATUS ROW                                  y:190-219  │
│  🔥 HEATER:ON  💨 FAN:ON  ☀ PHOTO:OFF  🚪 DOOR:CLOSED        │
│                                           480 × 30px            │
├────────────────────────────────────────────────────────────────┤
│ ALARM PANEL                                          y:222-299  │
│  ⚠ ALARMS (0)                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ (no active alarms)                                        │  │
│  │                                                           │  │
│  │ [scrollable area: up to 4 visible alarm lines]            │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                           480 × 78px            │
├────────────────────────────────────────────────────────────────┤
│ FOOTER BAR                                           y:302-319  │
│  SN:--- HW:--- FW:---  ▪ uptime: 00:00:00   480 × 18px        │
└────────────────────────────────────────────────────────────────┘
```

### 3.2 Pixel-Level Zone Definitions

```c
// Zone boundaries (x, y, width, height)
typedef struct { int x, y, w, h; } zone_t;

#define ZONE_HEADER       { 0,   0, 480,  30 }
#define ZONE_TEMP_AIR     { 2,  32, 234, 113 }
#define ZONE_TEMP_SKIN    { 244, 32, 234, 113 }
#define ZONE_HUMIDITY     { 2, 148, 476,  40 }
#define ZONE_ACTUATORS    { 2, 190, 476,  30 }
#define ZONE_ALARMS       { 2, 222, 476,  78 }
#define ZONE_FOOTER       { 0, 302, 480,  18 }
```

### 3.3 Font Scaling

| Element | Font Scale | Char Size | Line Height | Use |
|---------|-----------|-----------|-------------|-----|
| Large value | 3× | 15×21 px | 24 px | Temperature readings |
| Medium label | 2× | 10×14 px | 16 px | Section headers, setpoints |
| Small text | 1× | 5×7 px | 9 px | Footer, alarm details |

### 3.4 Temperature Panel Detail (Air — left half)

```
x:2, y:32 — 234×113 panel on COLOR_BG_PANEL background
┌──────────────────────────────┐
│ ▲ AIR TEMPERATURE            │  y:34  label (2×, COLOR_TEXT_SECONDARY)
│                              │
│      3 4 . 2 °C              │  y:56  value (3×, COLOR_TEMP_AIR)
│                              │
│  ┌────────────────────────┐  │  y:88  progress bar (220×8)
│  │████████████░░░░░░░░░░░│  │        fill=value mapped 20-42°C
│  └────────────────────────┘  │
│  SET: 34.0°C                 │  y:106 setpoint (2×, COLOR_SETPOINT)
│  20°C              42°C      │  y:124 range labels (1×, COLOR_TEXT_DIM)
└──────────────────────────────┘
```

The skin panel mirrors this layout at x:244 using `COLOR_TEMP_SKIN`.

### 3.5 Actuator Status Row Detail

```
y:190-219, four equally-spaced cells (119px each)
┌──────────┬──────────┬──────────┬──────────┐
│ 🔥 HTR   │ 💨 FAN   │ ☀ PHOTO  │ 🚪 DOOR  │
│   ON     │   ON     │   OFF    │  CLOSED  │
└──────────┴──────────┴──────────┴──────────┘
Each cell:  icon indicator (8×8 filled circle) + text label
ON state:   circle = COLOR_HEATER_ON/FAN_ON/PHOTO_ON, text = white
OFF state:  circle = COLOR_OFF_GRAY, text = COLOR_TEXT_DIM
```

### 3.6 Alarm Panel Detail

```
y:222-299, 476px wide
┌─ ALARMS (N) ─────────────────────────────────────────┐
│ Row 0: [●] ALM-001 CRITICAL  Over-temperature        │  y:240
│ Row 1: [▲] ALM-003 WARNING   Low humidity             │  y:254
│ Row 2:                                                 │  y:268
│ Row 3:                                                 │  y:282
└───────────────────────────────────────────────────────┘

Icon: ● (filled circle) for CRITICAL = COLOR_ALARM_CRIT
      ▲ (triangle) for WARNING = COLOR_ALARM_WARN
      ℹ (info) for INFO = COLOR_ALARM_INFO
Max 4 visible rows; alarm_scroll_offset for >4 alarms.
```

---

## 4. Complete `chip_state_t` Structure

```c
#define MAX_ALARMS       8
#define MAX_ALARM_DESC  48
#define UART_BUF_SIZE  512
#define MAX_DISPLAY_W  480
#define MAX_DISPLAY_H  320

// ─── Alarm Entry ──────────────────────────────────
typedef struct {
  int      id;                        // Alarm ID (0 = unused)
  int      type;                      // 0=info, 1=warning, 2=critical
  char     desc[MAX_ALARM_DESC];      // Description text
  int      state;                     // 0=inactive, 1=active, 2=acknowledged
} alarm_entry_t;

// ─── Parsed Telemetry (from CTRL,TEL / JSON) ─────
typedef struct {
  float    air_temp;                  // Chamber air temperature (°C)
  float    skin_temp;                 // Baby skin temperature (°C)
  float    humidity;                  // Relative humidity (%)
  float    fan_rpm;                   // Fan speed (RPM)
  float    heater_duty;              // Heater duty (0-100%)
  int      door_open;                // 0=closed, 1=open
  int      alarm_code;               // Current alarm code
  bool     valid;                    // At least one message received
} telemetry_t;

// ─── Parsed State (from CTRL,STATE) ───────────────
typedef struct {
  int      actuators_enabled;         // 0=OFF, 1=ON
  int      control_mode;             // 0=AIR, 1=SKIN
  float    air_setpoint;             // Air target (°C)
  float    skin_setpoint;            // Skin target (°C)
  float    hum_setpoint;             // Humidity target (%)
  int      phototherapy;              // 0=OFF, 1=ON
  int      mute;                     // 0=unmuted, 1=muted
  char     serial_number[20];        // Device S/N
  char     hw_number[8];             // Hardware revision number
  char     hw_revision[8];           // Hardware revision letter
  char     fw_version[16];           // Firmware version string
  int      num_alarms;               // Active alarm count
  int      skin_enabled;             // Skin sensor present
  int      comm_status;              // 0=OK, 1=error
  int      photo_time_min;           // Phototherapy timer (minutes)
  bool     valid;                    // At least one STATE received
} state_info_t;

// ─── Display Rendering State ──────────────────────
typedef struct {
  int      alarm_scroll_offset;      // Scroll position for alarm list
  int      blink_phase;              // 0 or 1, toggled every 500ms
  bool     needs_redraw;             // Dirty flag for display update
  uint64_t last_blink_ns;           // Timestamp of last blink toggle
} render_state_t;

// ─── Connection Health ────────────────────────────
typedef struct {
  uint64_t last_rx_ns;              // Timestamp of last UART byte
  uint64_t last_msg_ns;             // Timestamp of last complete message
  bool     connected;                // true if msg within timeout
  bool     timeout_shown;            // true if timeout already rendered
  int      msg_count;               // Total messages received
} conn_health_t;

// ─── Main Chip State ──────────────────────────────
typedef struct {
  // Wokwi hardware handles
  uart_dev_t     uart;
  buffer_t       fb;
  timer_t        refresh_timer;
  timer_t        watchdog_timer;

  // Display geometry
  uint32_t       width;
  uint32_t       height;
  uint32_t      *pixels;             // Framebuffer pixel array

  // UART RX parsing
  char           rx_buf[UART_BUF_SIZE];
  int            rx_len;

  // Parsed data
  telemetry_t    telemetry;
  state_info_t   state;
  alarm_entry_t  alarms[MAX_ALARMS];
  int            alarm_count;

  // Display state
  render_state_t render;
  conn_health_t  conn;

  // Configurable attributes (Wokwi chip attrs)
  uint32_t       attr_control_mode;   // attr ID
  uint32_t       attr_language;       // attr ID
  uint32_t       attr_skin_enabled;   // attr ID
  uint32_t       attr_air_setpoint;   // attr ID
  uint32_t       attr_skin_setpoint;  // attr ID
  uint32_t       attr_hum_setpoint;   // attr ID
  uint32_t       attr_comm_timeout;   // attr ID (ms)
  uint32_t       attr_auto_request;   // attr ID (bool)

  // Uptime tracking
  uint64_t       boot_ns;            // Sim time at chip_init
} chip_state_t;
```

---

## 5. Wokwi Chip Attributes

### 5.1 chip.json Definition

```json
{
  "name": "IncuNest Display HMI",
  "author": "IncuNest",
  "pins": ["RX", "TX", "VCC", "GND"],
  "display": {
    "width": 480,
    "height": 320
  },
  "controls": []
}
```

### 5.2 Attribute Initialization in `chip_init()`

```c
chip->attr_control_mode  = attr_init("controlMode", 0);      // 0=AIR, 1=SKIN
chip->attr_language      = attr_init("language", 0);          // 0=EN, 1=ES, 2=FR, 3=PT
chip->attr_skin_enabled  = attr_init("skinEnabled", 1);       // 0=disabled, 1=enabled
chip->attr_air_setpoint  = attr_init_float("airSetpoint", 34.0);
chip->attr_skin_setpoint = attr_init_float("skinSetpoint", 36.5);
chip->attr_hum_setpoint  = attr_init_float("humSetpoint", 60.0);
chip->attr_comm_timeout  = attr_init("commTimeoutMs", 3000);  // 3 sec default
chip->attr_auto_request  = attr_init("autoRequestState", 1);  // send HMI,REQ,STATE
```

### 5.3 Usage in diagram.json

```json
{
  "type": "chip-incu-display-hmi",
  "id": "display",
  "attrs": {
    "controlMode": "0",
    "language": "1",
    "skinEnabled": "1",
    "airSetpoint": "34.0",
    "skinSetpoint": "36.5",
    "humSetpoint": "60",
    "commTimeoutMs": "3000",
    "autoRequestState": "1"
  }
}
```

---

## 6. Message Parsing State Machine

### 6.1 Current Reality: JSON Protocol

The telemetry-reporter chip currently sends JSON messages:
```
{"temp":34.2,"skin":36.6,"hum":58.0,"fan":1200,"heater":42,"door":0,"alarm":0}\n
```

### 6.2 Future-Ready: CTRL/HMI Text Protocol

The design should support both formats for forward compatibility.

### 6.3 Parser Architecture

```
                    ┌──────────┐
        byte in ──▶ │ RX_ACCUM │ accumulate until \n or \r
                    └────┬─────┘
                         │ complete line
                         ▼
                    ┌──────────┐
                    │ DISPATCH │ check first chars
                    └──┬───┬──┘
                       │   │
            ┌──────────┘   └──────────┐
            ▼                         ▼
     starts with '{'          starts with 'CTRL,'
     ┌───────────┐            ┌────────────┐
     │ JSON_PARSE│            │ CSV_DISPATCH│
     └───────────┘            └──┬──┬──┬───┘
                                 │  │  │
                    ┌────────────┘  │  └────────────┐
                    ▼               ▼                ▼
              ┌──────────┐  ┌───────────┐   ┌────────────┐
              │ CTRL,TEL │  │ CTRL,STATE│   │ CTRL,ALM   │
              └──────────┘  └───────────┘   └────────────┘
```

### 6.4 Implementation

```c
// Dispatcher — called when a complete line is in rx_buf
static void parse_message(chip_state_t *chip) {
  chip->conn.last_msg_ns = get_sim_nanos();
  chip->conn.connected = true;
  chip->conn.msg_count++;

  if (chip->rx_buf[0] == '{') {
    parse_json_telemetry(chip);       // Current format
  } else if (strncmp(chip->rx_buf, "CTRL,TEL,", 9) == 0) {
    parse_ctrl_tel(chip);             // Future CTRL,TEL
  } else if (strncmp(chip->rx_buf, "CTRL,STATE,", 11) == 0) {
    parse_ctrl_state(chip);           // Future CTRL,STATE
  } else if (strncmp(chip->rx_buf, "CTRL,ALM,", 9) == 0) {
    parse_ctrl_alarm(chip);           // Future CTRL,ALM
  }
  // Unknown messages are silently ignored

  chip->render.needs_redraw = true;
}
```

### 6.5 JSON Parser (minimal, no malloc)

```c
static float json_extract_float(const char *json, const char *key) {
  char search[32];
  snprintf(search, sizeof(search), "\"%s\":", key);
  const char *p = strstr(json, search);
  if (!p) return -999.0f;  // sentinel for "not found"
  p += strlen(search);
  return strtof(p, NULL);
}

static int json_extract_int(const char *json, const char *key) {
  float v = json_extract_float(json, key);
  return (v == -999.0f) ? -1 : (int)v;
}

static void parse_json_telemetry(chip_state_t *chip) {
  chip->telemetry.air_temp     = json_extract_float(chip->rx_buf, "temp");
  chip->telemetry.skin_temp    = json_extract_float(chip->rx_buf, "skin");
  chip->telemetry.humidity     = json_extract_float(chip->rx_buf, "hum");
  chip->telemetry.fan_rpm      = json_extract_float(chip->rx_buf, "fan");
  chip->telemetry.heater_duty  = json_extract_float(chip->rx_buf, "heater");
  chip->telemetry.door_open    = json_extract_int(chip->rx_buf, "door");
  chip->telemetry.alarm_code   = json_extract_int(chip->rx_buf, "alarm");
  chip->telemetry.valid        = true;
}
```

### 6.6 CSV Protocol Parsers (future)

```c
// CTRL,TEL,<airTemp>,<skinTemp>,<humidity>[,<commStatus>]\n
static void parse_ctrl_tel(chip_state_t *chip) {
  char *p = chip->rx_buf + 9;  // skip "CTRL,TEL,"
  chip->telemetry.air_temp  = strtof(p, &p); if (*p == ',') p++;
  chip->telemetry.skin_temp = strtof(p, &p); if (*p == ',') p++;
  chip->telemetry.humidity  = strtof(p, &p);
  chip->telemetry.valid = true;
}

// CTRL,STATE,<act>,<mode>,<airSet>,<skinSet>,<humSet>,<photo>,
//            <mute>,<sn>,<hwNum>,<hwRev>,<fwVer>
//            [,<numAlarms>,<skinE>,<commStatus>,<photoTime>]\n
static void parse_ctrl_state(chip_state_t *chip) {
  char *p = chip->rx_buf + 11; // skip "CTRL,STATE,"
  char *tok;

  tok = strsep(&p, ","); chip->state.actuators_enabled = atoi(tok);
  tok = strsep(&p, ","); chip->state.control_mode      = atoi(tok);
  tok = strsep(&p, ","); chip->state.air_setpoint      = strtof(tok, NULL);
  tok = strsep(&p, ","); chip->state.skin_setpoint     = strtof(tok, NULL);
  tok = strsep(&p, ","); chip->state.hum_setpoint      = strtof(tok, NULL);
  tok = strsep(&p, ","); chip->state.phototherapy       = atoi(tok);
  tok = strsep(&p, ","); chip->state.mute               = atoi(tok);
  tok = strsep(&p, ","); strncpy(chip->state.serial_number, tok, 19);
  tok = strsep(&p, ","); strncpy(chip->state.hw_number, tok, 7);
  tok = strsep(&p, ","); strncpy(chip->state.hw_revision, tok, 7);
  tok = strsep(&p, ","); strncpy(chip->state.fw_version, tok, 15);

  // Optional extended fields
  if (p && *p) { tok = strsep(&p, ","); chip->state.num_alarms    = atoi(tok); }
  if (p && *p) { tok = strsep(&p, ","); chip->state.skin_enabled  = atoi(tok); }
  if (p && *p) { tok = strsep(&p, ","); chip->state.comm_status   = atoi(tok); }
  if (p && *p) { tok = strsep(&p, ","); chip->state.photo_time_min = atoi(tok); }

  chip->state.valid = true;
}

// CTRL,ALM,<id>,<type>,<desc>,<state>\n
static void parse_ctrl_alarm(chip_state_t *chip) {
  char *p = chip->rx_buf + 9;  // skip "CTRL,ALM,"
  char *tok;

  int id;
  tok = strsep(&p, ","); id = atoi(tok);

  // Find existing or first empty slot
  int slot = -1;
  for (int i = 0; i < MAX_ALARMS; i++) {
    if (chip->alarms[i].id == id) { slot = i; break; }
    if (slot < 0 && chip->alarms[i].id == 0) slot = i;
  }
  if (slot < 0) return;  // table full

  chip->alarms[slot].id = id;
  tok = strsep(&p, ","); chip->alarms[slot].type  = atoi(tok);
  tok = strsep(&p, ","); strncpy(chip->alarms[slot].desc, tok, MAX_ALARM_DESC - 1);
  tok = strsep(&p, ","); chip->alarms[slot].state = atoi(tok);

  // Remove alarm if state=0 (inactive)
  if (chip->alarms[slot].state == 0) {
    chip->alarms[slot].id = 0;
  }

  // Recount
  chip->alarm_count = 0;
  for (int i = 0; i < MAX_ALARMS; i++) {
    if (chip->alarms[i].id != 0) chip->alarm_count++;
  }
}
```

---

## 7. Edge Cases & Special States

### 7.1 Boot Screen — Before First Message

```
┌─────────────────────────────────────────┐
│               IncuNest                  │  (3× scale, white, centered)
│                                         │
│        Neonatal Incubator               │  (2× scale, COLOR_TEXT_SECONDARY)
│                                         │
│       ⏳ Connecting...                  │  (2× scale, COLOR_CONN_WAIT, blinking)
│                                         │
│     Waiting for motherboard             │  (1× scale, COLOR_TEXT_DIM)
└─────────────────────────────────────────┘
```

**Behavior:** Shown until `telemetry.valid || state.valid` becomes true.
The "Connecting..." text blinks at 500ms intervals using `render.blink_phase`.

### 7.2 Communication Timeout

**Detection:** In the watchdog timer callback (every 1000ms):
```c
static void on_watchdog(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  uint64_t now = get_sim_nanos();
  uint32_t timeout_ms = attr_read(chip->attr_comm_timeout);
  uint64_t timeout_ns = (uint64_t)timeout_ms * 1000000ULL;

  if (chip->conn.connected && (now - chip->conn.last_msg_ns > timeout_ns)) {
    chip->conn.connected = false;
    chip->render.needs_redraw = true;
  }

  // Blink phase toggle (500ms)
  if (now - chip->render.last_blink_ns > 500000000ULL) {
    chip->render.blink_phase ^= 1;
    chip->render.last_blink_ns = now;
    if (!chip->conn.connected) chip->render.needs_redraw = true;
  }
}
```

**Visual indicator when disconnected:**
- Header connection LED: Solid RED circle with "DISCONNECTED" text
- All telemetry values show "--.-" instead of numbers
- Overlay text: "⚠ COMM LOST" in `COLOR_ALARM_CRIT` at center of display

### 7.3 Alarm Rendering Priority

| Type | Color | Icon | Background | Sort Priority |
|------|-------|------|------------|---------------|
| CRITICAL (2) | `COLOR_ALARM_CRIT` | ● filled circle | `0x3E1111FF` | First |
| WARNING (1) | `COLOR_ALARM_WARN` | ▲ triangle | `0x3E2E11FF` | Second |
| INFO (0) | `COLOR_ALARM_INFO` | ℹ circle | `COLOR_BG_PANEL` | Third |

Active critical alarms cause the alarm panel header to flash red/dark at 500ms.

### 7.4 Door Open Warning

When `telemetry.door_open == 1`, the DOOR cell in the actuator row:
- Background flashes `COLOR_ALARM_WARN` at 500ms
- Text reads "OPEN!" in white

### 7.5 Heater Duty Visualization

In the air temp panel, when `telemetry.heater_duty > 0`:
- Show a small bar below the setpoint: `HTR: ██████░░░░ 42%`
- Bar color: interpolated from `COLOR_SETPOINT` (0%) → `COLOR_HEATER_ON` (100%)

---

## 8. Timer Configuration

### 8.1 Refresh Timer (display redraw)

```c
// 200ms = 5 FPS — sufficient for text-based medical display
#define REFRESH_INTERVAL_US  200000

const timer_config_t refresh_cfg = {
  .user_data = chip,
  .callback = on_refresh_timer,
};
chip->refresh_timer = timer_init(&refresh_cfg);
timer_start(chip->refresh_timer, REFRESH_INTERVAL_US, true);
```

**Rationale:** Medical device displays update at 2-5 Hz. WASM pixel writes are expensive — 614,400 bytes per frame. At 5 FPS that's ~3 MB/s of buffer_write throughput, which is manageable. Higher rates (10+ FPS) waste CPU cycles for no perceptual benefit on a data dashboard.

**Optimization:** Only call `buffer_write()` when `render.needs_redraw` is true:
```c
static void on_refresh_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  if (chip->render.needs_redraw) {
    render_display(chip);
    buffer_write(chip->fb, 0, chip->pixels,
                 chip->width * chip->height * sizeof(uint32_t));
    chip->render.needs_redraw = false;
  }
}
```

### 8.2 Watchdog Timer (connection health + blink)

```c
// 1000ms — checks comm timeout, toggles blink phase
#define WATCHDOG_INTERVAL_US  1000000

const timer_config_t watchdog_cfg = {
  .user_data = chip,
  .callback = on_watchdog,
};
chip->watchdog_timer = timer_init(&watchdog_cfg);
timer_start(chip->watchdog_timer, WATCHDOG_INTERVAL_US, true);
```

---

## 9. Autonomous HMI,REQ,STATE on Init

### Recommendation: YES, conditionally

**Design:**
```c
// In chip_init(), after UART is ready:
if (attr_read(chip->attr_auto_request) == 1) {
  // Delay 500ms to let motherboard boot first
  timer_t req_timer = timer_init(&(timer_config_t){
    .user_data = chip,
    .callback = send_state_request,
  });
  timer_start(req_timer, 500000, false);  // one-shot, 500ms delay
}

static void send_state_request(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  const char *msg = "HMI,REQ,STATE\n";
  uart_write(chip->uart, (uint8_t *)msg, strlen(msg));
}
```

**Rationale:**
1. The real CrowPanel HMI sends `HMI,REQ,STATE` on boot to synchronize with the motherboard.
2. The 500ms delay prevents a race condition where the display transmits before the ESP32 UART is initialized.
3. Made configurable via `autoRequestState` attribute so it can be disabled for pure-telemetry mode (current JSON protocol doesn't support this command).

**Retry logic:** If no STATE response within 3 seconds, retry up to 3 times at 2-second intervals. After 3 failures, continue displaying with telemetry-only data.

---

## 10. Drawing Primitives Required

Beyond the existing `draw_pixel`, `draw_char`, `draw_string`, `fill_display`:

```c
// Filled rectangle
static void draw_rect_fill(chip_state_t *chip, int x, int y,
                           int w, int h, uint32_t color);

// Rectangle border (1px)
static void draw_rect_border(chip_state_t *chip, int x, int y,
                             int w, int h, uint32_t color);

// Horizontal progress bar
static void draw_progress_bar(chip_state_t *chip, int x, int y,
                              int w, int h,
                              float value, float min, float max,
                              uint32_t fill_color, uint32_t bg_color);

// Filled circle (for status LEDs, alarm icons)
static void draw_circle_fill(chip_state_t *chip, int cx, int cy,
                             int r, uint32_t color);

// Horizontal line
static void draw_hline(chip_state_t *chip, int x, int y,
                       int w, uint32_t color);

// Centered string (auto-calculates x offset)
static void draw_string_centered(chip_state_t *chip, int cx, int y,
                                 const char *str, uint32_t color, int scale);
```

---

## 11. Rendering Pipeline

```c
static void render_display(chip_state_t *chip) {
  // 1. Clear entire framebuffer
  fill_display(chip, COLOR_BG_DARK);

  if (!chip->telemetry.valid && !chip->state.valid) {
    // 2a. Boot/connecting screen
    render_boot_screen(chip);
    return;
  }

  // 2b. Normal operational display
  render_header(chip);           // Title, mode, connection LED
  render_temp_panel_air(chip);   // Left panel
  render_temp_panel_skin(chip);  // Right panel
  render_humidity_bar(chip);     // Humidity section
  render_actuator_row(chip);     // Heater/Fan/Photo/Door status
  render_alarm_panel(chip);      // Alarm list
  render_footer(chip);           // S/N, HW, FW, uptime

  // 3. Comm-lost overlay (on top of everything)
  if (!chip->conn.connected && chip->conn.msg_count > 0) {
    render_comm_lost_overlay(chip);
  }
}
```

---

## 12. Complete chip_init() Flow

```c
void chip_init(void) {
  // 1. Allocate & zero state
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  memset(chip, 0, sizeof(chip_state_t));

  // 2. Initialize framebuffer
  chip->fb = framebuffer_init(&chip->width, &chip->height);
  chip->pixels = malloc(chip->width * chip->height * sizeof(uint32_t));

  // 3. Initialize attributes
  chip->attr_control_mode  = attr_init("controlMode", 0);
  chip->attr_language      = attr_init("language", 0);
  chip->attr_skin_enabled  = attr_init("skinEnabled", 1);
  chip->attr_air_setpoint  = attr_init_float("airSetpoint", 34.0f);
  chip->attr_skin_setpoint = attr_init_float("skinSetpoint", 36.5f);
  chip->attr_hum_setpoint  = attr_init_float("humSetpoint", 60.0f);
  chip->attr_comm_timeout  = attr_init("commTimeoutMs", 3000);
  chip->attr_auto_request  = attr_init("autoRequestState", 1);

  // 4. Load initial setpoints from attrs into state
  chip->state.air_setpoint  = attr_read_float(chip->attr_air_setpoint);
  chip->state.skin_setpoint = attr_read_float(chip->attr_skin_setpoint);
  chip->state.hum_setpoint  = attr_read_float(chip->attr_hum_setpoint);
  chip->state.control_mode  = attr_read(chip->attr_control_mode);

  // 5. Initialize UART
  const uart_config_t uart_cfg = {
    .rx = pin_init("RX", INPUT),
    .tx = pin_init("TX", OUTPUT),
    .baud_rate = 115200,
    .rx_data = on_uart_rx,
    .user_data = chip,
  };
  chip->uart = uart_init(&uart_cfg);

  // 6. Boot timestamp
  chip->boot_ns = get_sim_nanos();
  chip->conn.last_msg_ns = chip->boot_ns;

  // 7. Render initial boot screen
  chip->render.needs_redraw = true;
  render_display(chip);
  buffer_write(chip->fb, 0, chip->pixels,
               chip->width * chip->height * sizeof(uint32_t));

  // 8. Start timers
  const timer_config_t refresh_cfg = {
    .user_data = chip,
    .callback = on_refresh_timer,
  };
  chip->refresh_timer = timer_init(&refresh_cfg);
  timer_start(chip->refresh_timer, 200000, true);  // 200ms = 5 FPS

  const timer_config_t watchdog_cfg = {
    .user_data = chip,
    .callback = on_watchdog,
  };
  chip->watchdog_timer = timer_init(&watchdog_cfg);
  timer_start(chip->watchdog_timer, 1000000, true);  // 1s watchdog

  // 9. Auto-request state (if enabled)
  if (attr_read(chip->attr_auto_request) == 1) {
    const timer_config_t req_cfg = {
      .user_data = chip,
      .callback = send_state_request,
    };
    timer_t req_timer = timer_init(&req_cfg);
    timer_start(req_timer, 500000, false);  // one-shot 500ms
  }
}
```

---

## 13. Summary of Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Resolution | 480×320 | Good fidelity vs memory (600KB) |
| Pixel format | RGBA 32-bit | Required by Wokwi framebuffer API |
| Refresh rate | 5 FPS (200ms) | Dirty-flag gated; medical display doesn't need 30fps |
| Watchdog rate | 1 Hz (1000ms) | Sufficient for 3s timeout detection |
| Comm timeout | 3000ms (configurable) | 6× the 500ms telemetry rate |
| Auto-request | Yes, after 500ms | Matches real HMI boot behavior |
| JSON + CSV parsing | Both supported | Forward-compatible with real firmware protocol |
| Max alarms | 8 slots, 4 visible | Scrollable; sufficient for IEC 60601 alarm levels |
| Font | Existing 5×7 at 1×/2×/3× | No external font dependency needed |
| Blink rate | 500ms | Industry standard for alarm indicators |
| WASM feasibility | ✅ Confirmed | 600KB FB + 50KB state < 1MB linear memory |

---

## 14. Files to Modify

| File | Change |
|------|--------|
| `chips/incu-display-hmi/incu-display-hmi.chip.json` | Update display to 480×320, keep pins |
| `chips/incu-display-hmi/incu-display-hmi.c` | Full rewrite per this design |
| `chips/incu-display-hmi/README.md` | Update with new features/protocol |
| `examples/full-incubator-demo/diagram.json` | Add display attrs to chip config |
