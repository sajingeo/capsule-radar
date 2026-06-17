// LVGL display + touch bring-up. Two panel paths share one set of LVGL plumbing:
//   BOARD_LCD_128     -> Arduino_GC9A01 over 4-wire SPI; backlight via LEDC PWM
//   BOARD_AMOLED_175  -> Arduino_CO5300 over QSPI;       brightness via panel cmd 0x51
// The actual UI is built by ui_create() (shared with the native SDL sim).
#include "display.h"
#include "config.h"
#include "radar_view.h"
#include "ui.h"
#include "touch.h"

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <esp_heap_caps.h>

// --- Panel objects (typed so each board can reach its own setBrightness etc.) -
static Arduino_DataBus *s_bus = nullptr;
#if defined(BOARD_AMOLED_175)
static Arduino_CO5300  *s_gfx = nullptr;
#elif defined(BOARD_LCD_128)
static Arduino_GC9A01  *s_gfx = nullptr;

// LEDC PWM channel for the GC9A01 backlight (no panel-side brightness command).
#define BL_LEDC_FREQ     20000
#define BL_LEDC_RES_BITS 8
#endif

// --- LVGL plumbing -----------------------------------------------------------
#define LVGL_BUF_LINES 40    // partial draw-buffer height (lines); kept in fast internal RAM
static lv_disp_draw_buf_t s_draw_buf;
static lv_disp_drv_t      s_disp_drv;
static lv_indev_drv_t     s_indev_drv;
static lv_color_t        *s_buf1 = nullptr;
static lv_color_t        *s_buf2 = nullptr;

static volatile uint32_t s_frameCount = 0;   // rendered frames (last-flush), for FPS measurement
uint32_t display_frames() { return s_frameCount; }

static volatile uint8_t s_rot = 0;           // display rotation: 0/1/2/3 = 0°/90°/180°/270°
static lv_color_t *s_rotBuf = nullptr;       // PSRAM scratch for 90/270° transpose (see begin())

// LVGL -> panel, applying the chosen rotation while pushing.
//   0°   : straight through.
//   180° : reverse the flat block in place — no scratch buffer.
//   90°/270° : the block transposes (w<->h), so it can't be reversed in place; copy it
//              rotated into a PSRAM scratch buffer.
static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *px) {
    const int w = (int)(area->x2 - area->x1 + 1);
    const int h = (int)(area->y2 - area->y1 + 1);
    lv_color_t *out = px;
    int16_t  dx = area->x1, dy = area->y1;
    uint16_t dw = (uint16_t)w, dh = (uint16_t)h;

    switch (s_rot) {
        case 2:  // 180°
            for (int i = 0, j = w * h - 1; i < j; ++i, --j) { lv_color_t t = px[i]; px[i] = px[j]; px[j] = t; }
            dx = (int16_t)(SCREEN_W - 1 - area->x2);
            dy = (int16_t)(SCREEN_H - 1 - area->y2);
            break;
        case 1:  // 90° CW
            if (s_rotBuf) {
                for (int j = 0; j < h; ++j)
                    for (int i = 0; i < w; ++i)
                        s_rotBuf[i * h + (h - 1 - j)] = px[j * w + i];
                out = s_rotBuf; dw = (uint16_t)h; dh = (uint16_t)w;
                dx = (int16_t)(SCREEN_H - 1 - area->y2); dy = area->x1;
            }
            break;
        case 3:  // 270° CW
            if (s_rotBuf) {
                for (int j = 0; j < h; ++j)
                    for (int i = 0; i < w; ++i)
                        s_rotBuf[(w - 1 - i) * h + j] = px[j * w + i];
                out = s_rotBuf; dw = (uint16_t)h; dh = (uint16_t)w;
                dx = area->y1; dy = (int16_t)(SCREEN_W - 1 - area->x2);
            }
            break;
        default: break;  // 0°
    }
#if (LV_COLOR_16_SWAP != 0)
    s_gfx->draw16bitBeRGBBitmap(dx, dy, (uint16_t *)out, dw, dh);
#else
    s_gfx->draw16bitRGBBitmap(dx, dy, (uint16_t *)out, dw, dh);
#endif
    if (lv_disp_flush_is_last(drv)) s_frameCount++;
    lv_disp_flush_ready(drv);
}

#if defined(BOARD_AMOLED_175)
// CO5300 (QSPI) requires 2-pixel-aligned flush windows: even start, odd end.
// Without this, partial-area updates (e.g. the radar sweep) tear / ghost / flicker.
static void rounder_cb(lv_disp_drv_t *drv, lv_area_t *area) {
    (void)drv;
    area->x1 &= ~1;
    area->y1 &= ~1;
    area->x2 |= 1;
    area->y2 |= 1;
}
#endif

// Touch -> LVGL pointer. LVGL keeps the last point on release.
static void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    (void)drv;
    uint16_t x, y;
    if (touch_read(&x, &y)) {
        uint16_t lx = x, ly = y;                          // map physical touch -> logical (inverse rotation)
        switch (s_rot) {
            case 1: lx = y;                            ly = (uint16_t)(SCREEN_H - 1 - x); break;
            case 2: lx = (uint16_t)(SCREEN_W - 1 - x); ly = (uint16_t)(SCREEN_H - 1 - y); break;
            case 3: lx = (uint16_t)(SCREEN_W - 1 - y); ly = x;                            break;
            default: break;
        }
        data->point.x = (lv_coord_t)lx;
        data->point.y = (lv_coord_t)ly;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

namespace display {

bool begin() {
#if defined(BOARD_AMOLED_175)
    Serial.println("[display] init CO5300 QSPI...");
    s_bus = new Arduino_ESP32QSPI(PIN_LCD_CS, PIN_LCD_SCLK,
                                  PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2, PIN_LCD_D3);
    s_gfx = new Arduino_CO5300(s_bus, PIN_LCD_RST, 0 /*rotation*/,
                               SCREEN_W, SCREEN_H,
                               LCD_COL_OFFSET, LCD_ROW_OFFSET, 0, 0);
    if (!s_gfx->begin(LCD_QSPI_HZ)) {
        Serial.println("[display] gfx->begin() FAILED");
        return false;
    }
    s_gfx->fillScreen(RGB565_BLACK);
    s_gfx->setBrightness(BRIGHTNESS_DEFAULT);
#elif defined(BOARD_LCD_128)
    Serial.println("[display] init GC9A01 SPI...");
    // LEDC PWM on backlight (no panel-side brightness on GC9A01).
    ledcAttach(PIN_LCD_BL, BL_LEDC_FREQ, BL_LEDC_RES_BITS);
    ledcWrite(PIN_LCD_BL, 0);   // dark until we've drawn something

    s_bus = new Arduino_ESP32SPI(PIN_LCD_DC, PIN_LCD_CS, PIN_LCD_SCLK, PIN_LCD_MOSI, PIN_LCD_MISO);
    s_gfx = new Arduino_GC9A01(s_bus, PIN_LCD_RST, 0 /*rotation*/, true /*IPS*/);
    if (!s_gfx->begin(LCD_SPI_HZ)) {
        Serial.println("[display] gfx->begin() FAILED");
        return false;
    }
    s_gfx->fillScreen(RGB565_BLACK);
#endif

    Serial.println("[display] panel up; init LVGL...");
    lv_init();

    // Draw scratch in INTERNAL DMA RAM: rendering anti-aliased graphics into the
    // PSRAM is slow (the bottleneck, not bus bandwidth). Single partial buffer
    // fits the budget on both boards.
    const size_t buf_px = (size_t)SCREEN_W * LVGL_BUF_LINES;
    s_buf1 = (lv_color_t *)heap_caps_malloc(buf_px * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    s_buf2 = nullptr;
    if (!s_buf1) {
        Serial.println("[display] internal draw buffer failed; falling back to PSRAM");
        s_buf1 = (lv_color_t *)heap_caps_malloc(buf_px * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    }
    lv_disp_draw_buf_init(&s_draw_buf, s_buf1, s_buf2, buf_px);

    // Scratch target for 90/270° rotation. NULL is fine (rotation falls back to un-rotated).
    s_rotBuf = (lv_color_t *)heap_caps_malloc(buf_px * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

    lv_disp_drv_init(&s_disp_drv);
    s_disp_drv.hor_res  = SCREEN_W;
    s_disp_drv.ver_res  = SCREEN_H;
    s_disp_drv.flush_cb = flush_cb;
#if defined(BOARD_AMOLED_175)
    s_disp_drv.rounder_cb = rounder_cb;     // CO5300 needs 2-px-aligned windows
#endif
    s_disp_drv.draw_buf = &s_draw_buf;
    lv_disp_drv_register(&s_disp_drv);

    if (touch_begin()) {
        lv_indev_drv_init(&s_indev_drv);
        s_indev_drv.type = LV_INDEV_TYPE_POINTER;
        s_indev_drv.read_cb = touch_read_cb;
        lv_indev_drv_register(&s_indev_drv);
        Serial.println("[display] touch registered");
    }

    Serial.printf("[display] PSRAM free: %u KB\n", (unsigned)(ESP.getFreePsram() / 1024));
    ui_create();

#if defined(BOARD_LCD_128)
    setBrightness(BRIGHTNESS_DEFAULT);
#endif
    Serial.println("[display] LVGL ready");
    return true;
}

void loop() { lv_timer_handler(); }

void setBrightness(uint8_t v) {
#if defined(BOARD_AMOLED_175)
    if (s_gfx) s_gfx->setBrightness(v);
#elif defined(BOARD_LCD_128)
    ledcWrite(PIN_LCD_BL, v);
#endif
}

void setRotation(uint8_t quarters) {
    s_rot = (uint8_t)(quarters & 3);
    lv_obj_t *scr = lv_scr_act();
    if (scr) lv_obj_invalidate(scr);
}
uint8_t rotation() { return s_rot; }

uint32_t inactiveMs() { return lv_disp_get_inactive_time(NULL); }

} // namespace display
