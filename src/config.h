#pragma once
// Capsule Radar — multi-board build & user configuration.
//
// Selected at build time by a -DBOARD_* flag set per env in platformio.ini:
//   -DBOARD_LCD_128=1     -> Waveshare ESP32-S3-Touch-LCD-1.28 (240x240 GC9A01, ETA6096, no audio/RTC)
//   -DBOARD_AMOLED_175=1  -> Waveshare ESP32-S3-Touch-AMOLED-1.75 (466x466 CO5300, AXP2101 PMIC, ES8311 codec, PCF85063 RTC)
//
// Everything that differs between the two boards (pins, screen size, UI scale,
// peripheral I2C addresses, ADS-B feed protocol) is gated below.

#define FW_VERSION "1.3.0"   // bumped for the multi-board split

#if !defined(BOARD_LCD_128) && !defined(BOARD_AMOLED_175)
#  error "config.h: no board selected. Add -DBOARD_LCD_128=1 or -DBOARD_AMOLED_175=1 in platformio.ini."
#endif

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

// ---------- ADS-B API (free, non-commercial) ----------
#define ADSB_PRIMARY_HOST   "api.airplanes.live"
#define ADSB_FALLBACK_HOST  "api.adsb.lol"
#define ADSB_USER_AGENT     "CapsuleRadar/1.0 (ESP32-S3 hobby; +https://github.com/sajingeo/capsule-radar)"
#define ADSB_HTTPS_INSECURE 1
#define ADSB_MAX_AIRCRAFT   60

// AMOLED has 8 MB OPI PSRAM and runs HTTPS reliably. The LCD-128 (R2 / 2 MB QSPI
// PSRAM) cannot reliably allocate the contiguous internal-RAM block mbedTLS needs
// for a TLS handshake under load, so it falls back to plain HTTP for the feed.
// Both api.airplanes.live and api.adsb.lol serve the same JSON over HTTP.
#if defined(BOARD_AMOLED_175)
#  define ADSB_USE_HTTPS    1
#else
#  define ADSB_USE_HTTPS    0
#endif

// ---------- Debug ----------
#define DEBUG_MEM           0

// ---------- Common timezone / brightness ----------
#define TZ_STR              "CET-1CEST,M3.5.0,M10.5.0/3"  // POSIX TZ (Spain)
#define BRIGHTNESS_DEFAULT  200
#define BRIGHTNESS_IDLE     25
#define IDLE_DIM_MS         20000
#define LV_COLOR_DEPTH_BITS 16

// =====================================================================
// Board-specific block: pins, screen, UI scale, I2C addresses
// =====================================================================
#if defined(BOARD_LCD_128)
// ---- Waveshare ESP32-S3-Touch-LCD-1.28 (GC9A01 240x240 over 4-wire SPI) ----

// Screen
#define SCREEN_W            240
#define SCREEN_H            240
#define SCREEN_CX           120
#define SCREEN_CY           120
#define RADAR_R_OUTER_PX    112
#define LCD_SPI_HZ          40000000       // start safe; can push toward 80 MHz once stable

// LCD pins (GC9A01 4-wire SPI; verified against ESP32-S3-Touch-LCD-1.28-Demo)
#define PIN_LCD_MOSI        11
#define PIN_LCD_MISO        12
#define PIN_LCD_SCLK        10
#define PIN_LCD_CS           9
#define PIN_LCD_DC           8
#define PIN_LCD_RST         14
#define PIN_LCD_BL           2             // PWM via ledcAttach

// Shared I2C
#define PIN_I2C_SDA          6
#define PIN_I2C_SCL          7

// Touch (CST816S)
#define PIN_TP_INT           5
#define PIN_TP_RST          13
#define TP_MIRROR_X       false
#define TP_MIRROR_Y       false
#define I2C_ADDR_TOUCH      0x15

// IMU
#define I2C_ADDR_IMU        0x6B          // alt 0x6A; auto-probed at runtime

// Battery (ETA6096 — no I2C fuel gauge; voltage on ADC GPIO1 via 200k/100k divider)
#define PIN_BAT_ADC          1
#define BAT_DIVIDER_RATIO    3.0f

// Aux MOSFET (per wiki; only some board variants populate it)
#define PIN_AUX_MOSFET1      4

// Boot button
#define PIN_BOOT_BUTTON      0

// UI scale (240x240 layout)
#define UI_FONT_TITLE       lv_font_montserrat_16
#define UI_FONT_HUD         lv_font_montserrat_10
#define UI_FONT_BODY        lv_font_montserrat_12
#define UI_FONT_SMALL       lv_font_montserrat_10
#define UI_FONT_TINY        lv_font_montserrat_8

#define UI_TILE_DIA         236
#define UI_CARD_W           200
#define UI_CARD_H           78
#define UI_CARD_Y           38
#define UI_CARD_PAD         6
#define UI_CARD_TITLE_Y     0
#define UI_CARD_LR_Y        16
#define UI_CARD_R_X         96
#define UI_CARD_ROUTE_Y     50

#define UI_PHOTO_OFFSET     16
#define UI_PHOTO_FALLBACK_Y -54

#define UI_TILE_TITLE_Y     10

#define UI_ZOOM_W           70
#define UI_ZOOM_H           22
#define UI_ZOOM_BOTTOM      -16

#define UI_HUD_WIFI_X       -68
#define UI_HUD_TOP_Y        22
#define UI_HUD_BAR_W        2
#define UI_HUD_BAR_BASE     3
#define UI_HUD_BAR_STEP     2
#define UI_HUD_BAR_GAP      3
#define UI_HUD_WIFI_BOX_W   14
#define UI_HUD_WIFI_BOX_H   10
#define UI_HUD_COUNT_X      -42
#define UI_HUD_CLOCK_X       0
#define UI_HUD_BATT_X       50
#define UI_HUD_DATE_Y       34

#define UI_LIST_W           200
#define UI_LIST_H           192
#define UI_LIST_Y           12

#define UI_STATS_LBL_Y      -8
#define UI_STATS_NET_W      200
#define UI_STATS_NET_Y      68
#define UI_STATS_VER_Y      88

#define UI_SPLASH_DIA0      110
#define UI_SPLASH_DIA1       74
#define UI_SPLASH_DIA2       40
#define UI_SPLASH_SWEEP      110
#define UI_SPLASH_SWEEP_Y    -4
#define UI_SPLASH_TITLE_Y    70
#define UI_SPLASH_SUB_Y      90

// Radar scope
#define RADAR_RING0          28
#define RADAR_RING1          56
#define RADAR_RING2          84
#define RADAR_CROSSHAIR_HALF 112
#define RADAR_TAP_RADIUS     22
#define RADAR_GLYPH_PAD      14
#define RADAR_GLYPH_PAD_R    80
#define RADAR_GLYPH_PAD_DRG  18
#define RADAR_RANGELBL_X     48
#define RADAR_RANGELBL_Y     -4
#define RADAR_ROSE_PAD       6
#define UI_FONT_ROSE_BIG     lv_font_montserrat_16
#define UI_FONT_ROSE_SMALL   lv_font_montserrat_12

#elif defined(BOARD_AMOLED_175)
// ---- Waveshare ESP32-S3-Touch-AMOLED-1.75 (CO5300 466x466 over QSPI) ----

// Screen
#define SCREEN_W            466
#define SCREEN_H            466
#define SCREEN_CX           233
#define SCREEN_CY           233
#define RADAR_R_OUTER_PX    218
#define LCD_COL_OFFSET      6              // CO5300 column-gap on this panel
#define LCD_ROW_OFFSET      0
#define LCD_QSPI_HZ         80000000

// LCD pins (CO5300 QSPI)
#define PIN_LCD_CS          12
#define PIN_LCD_RST         39
#define PIN_LCD_SCLK        38
#define PIN_LCD_D0           4
#define PIN_LCD_D1           5
#define PIN_LCD_D2           6
#define PIN_LCD_D3           7

// Shared I2C
#define PIN_I2C_SDA         15
#define PIN_I2C_SCL         14

// Touch (CST9217)
#define PIN_TP_INT          11
#define PIN_TP_RST          40
#define TP_MIRROR_X        true
#define TP_MIRROR_Y        true
#define I2C_ADDR_TOUCH      0x5A

// IMU / RTC / PMIC / codec (all on the shared I2C bus)
#define I2C_ADDR_IMU        0x6B
#define I2C_ADDR_RTC        0x51
#define I2C_ADDR_PMIC       0x34

// ES8311 codec over I2S
#define PIN_I2S_MCLK        42
#define PIN_I2S_BCLK         9
#define PIN_I2S_LRCLK       45
#define PIN_I2S_DOUT         8
#define PIN_I2S_DIN         10
#define PIN_AUDIO_PA        46

// Boot button
#define PIN_BOOT_BUTTON      0

// UI scale (466x466 layout — original AMOLED design)
#define UI_FONT_TITLE       lv_font_montserrat_28
#define UI_FONT_HUD         lv_font_montserrat_14
#define UI_FONT_BODY        lv_font_montserrat_16
#define UI_FONT_SMALL       lv_font_montserrat_14
#define UI_FONT_TINY        lv_font_montserrat_12

#define UI_TILE_DIA         462
#define UI_CARD_W           300
#define UI_CARD_H           118
#define UI_CARD_Y           66
#define UI_CARD_PAD         12
#define UI_CARD_TITLE_Y     0
#define UI_CARD_LR_Y        26
#define UI_CARD_R_X         150
#define UI_CARD_ROUTE_Y     76

#define UI_PHOTO_OFFSET     28
#define UI_PHOTO_FALLBACK_Y -104

#define UI_TILE_TITLE_Y     22

#define UI_ZOOM_W           104
#define UI_ZOOM_H           36
#define UI_ZOOM_BOTTOM      -34

#define UI_HUD_WIFI_X       -94
#define UI_HUD_TOP_Y        50
#define UI_HUD_BAR_W        3
#define UI_HUD_BAR_BASE     4
#define UI_HUD_BAR_STEP     3
#define UI_HUD_BAR_GAP      5
#define UI_HUD_WIFI_BOX_W   21
#define UI_HUD_WIFI_BOX_H   14
#define UI_HUD_COUNT_X      -34
#define UI_HUD_CLOCK_X      30
#define UI_HUD_BATT_X       92
#define UI_HUD_DATE_Y       70

#define UI_LIST_W           300
#define UI_LIST_H           372
#define UI_LIST_Y           22

#define UI_STATS_LBL_Y      -16
#define UI_STATS_NET_W      320
#define UI_STATS_NET_Y      132
#define UI_STATS_VER_Y      170

#define UI_SPLASH_DIA0      210
#define UI_SPLASH_DIA1      142
#define UI_SPLASH_DIA2       78
#define UI_SPLASH_SWEEP      210
#define UI_SPLASH_SWEEP_Y    -8
#define UI_SPLASH_TITLE_Y   118
#define UI_SPLASH_SUB_Y     150

// Radar scope
#define RADAR_RING0          50
#define RADAR_RING1         104
#define RADAR_RING2         160
#define RADAR_CROSSHAIR_HALF 211
#define RADAR_TAP_RADIUS     40
#define RADAR_GLYPH_PAD      22
#define RADAR_GLYPH_PAD_R   148
#define RADAR_GLYPH_PAD_DRG  30
#define RADAR_RANGELBL_X     92
#define RADAR_RANGELBL_Y     -8
#define RADAR_ROSE_PAD       12
#define UI_FONT_ROSE_BIG     lv_font_montserrat_28
#define UI_FONT_ROSE_SMALL   lv_font_montserrat_16

#endif // BOARD_*

// Safety net: should never fire if a board flag is set.
#if (PIN_LCD_SCLK < 0) || (PIN_I2C_SDA < 0)
#  error "config.h: SPI/I2C pins are back to placeholders (-1). Restore the real values."
#endif
