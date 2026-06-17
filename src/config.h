#pragma once
// Capsule Radar — build & user configuration.
// Target board: Waveshare ESP32-S3-Touch-LCD-1.28 (GC9A01 240x240 round IPS).
// Pins verified against ESP32-S3-Touch-LCD-1.28-Demo (DEV_Config.h, Setup207_GC9A01.h, LVGL_Arduino.ino).

#define FW_VERSION "1.2.9"   // shown on the web config page + Stats screen; bump on release

// ---------- Home location (default: Dénia, Spain) ----------
// Overridable at runtime via the web config page (stored in NVS).
#define HOME_LAT_DEFAULT   38.8409
#define HOME_LON_DEFAULT    0.1059

// ---------- Radar ----------
#define RANGE_KM_DEFAULT    30.0f          // display range (outer ring). Query is wider, see ADSB_QUERY_KM
#define ADSB_QUERY_KM       50.0f          // feed query radius (> display: off-range traffic shows as edge arrows)
static const float RANGE_STEPS_KM[] = {10.0f, 20.0f, 30.0f, 50.0f, 100.0f};
#define POLL_INTERVAL_MS    2000           // be gentle with the free API (>=1000)
#define POLL_INTERVAL_BATTERY_MS 5000      // slower polling when running on battery
#define MOTION_INTERP       1              // 1 = glyphs glide between polls; 0 = snap to new pos
#define AC_STALE_MS         15000          // drop aircraft not refreshed in this long

// ---------- Screen (GC9A01 IPS, 240x240) ----------
#define SCREEN_W            240
#define SCREEN_H            240
#define SCREEN_CX           120
#define SCREEN_CY           120
#define RADAR_R_OUTER_PX    112            // outer ring radius in pixels (leave a small bezel margin)
#define LV_COLOR_DEPTH_BITS 16
#define LCD_SPI_HZ          40000000       // GC9A01 4-wire SPI clock (datasheet up to ~80 MHz; start safe at 40)
#define BRIGHTNESS_DEFAULT  200            // 0..255, PWM duty on PIN_LCD_BL
#define TZ_STR              "CET-1CEST,M3.5.0,M10.5.0/3"  // POSIX TZ (Spain) for local time/date
#define BRIGHTNESS_IDLE     25             // dimmed after no touch for IDLE_DIM_MS
#define IDLE_DIM_MS         20000          // dim the screen after this long without a touch

// ---------- ADS-B API (free, non-commercial) ----------
#define ADSB_PRIMARY_HOST   "api.airplanes.live"   // GET /v2/point/{lat}/{lon}/{radius_nm}
#define ADSB_FALLBACK_HOST  "api.adsb.lol"          // same readsb format
#define ADSB_USER_AGENT     "CapsuleRadar/1.0 (ESP32-S3 hobby; +https://github.com/socquique/capsule-radar)"
#define ADSB_HTTPS_INSECURE 1               // 1 = setInsecure() (hobby). 0 = use pinned root CA.
#define ADSB_MAX_AIRCRAFT   60              // hard cap parsed per poll (protect RAM in busy areas)

// ---------- Debug ----------
#define DEBUG_MEM           0               // 1 = print a [mem] heap/fps line every 5s on serial

// ---------- Pin map (Waveshare ESP32-S3-Touch-LCD-1.28) ----------
// LCD: GC9A01 over 4-wire SPI
#define PIN_LCD_MOSI        11
#define PIN_LCD_MISO        12             // unused by GC9A01 in practice
#define PIN_LCD_SCLK        10
#define PIN_LCD_CS           9
#define PIN_LCD_DC           8
#define PIN_LCD_RST         14
#define PIN_LCD_BL           2             // PWM via ledcWrite

// Shared I2C bus: CST816S touch + QMI8658 IMU
#define PIN_I2C_SDA          6
#define PIN_I2C_SCL          7

// Touch (CST816S)
#define PIN_TP_INT           5             // active-low IRQ. NB: wiki also calls GPIO5 a MOSFET switch — demo uses it for INT.
#define PIN_TP_RST          13
#define TP_MIRROR_X       false
#define TP_MIRROR_Y       false

// Battery (ETA6096 charger; voltage via ADC + 200k/100k divider on GPIO1)
#define PIN_BAT_ADC          1
#define BAT_DIVIDER_RATIO    3.0f          // (200k+100k)/100k

// Aux MOSFETs (per wiki — populated only in some variants; test before use)
#define PIN_AUX_MOSFET1      4

// I2C addresses
#define I2C_ADDR_TOUCH      0x15           // CST816S
#define I2C_ADDR_IMU        0x6B           // QMI8658 (alt 0x6A; this board uses 0x6B)

// Boot button (held on boot = captive portal reset)
#define PIN_BOOT_BUTTON      0

// Safety net: should never fire. Keeps future edits honest.
#if (PIN_LCD_SCLK < 0) || (PIN_I2C_SDA < 0)
#  error "config.h: SPI/I2C pins are back to placeholders (-1). Restore the real values."
#endif
