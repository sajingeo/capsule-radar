#pragma once
// LVGL display + capacitive-touch bring-up. The panel driver is selected at
// build time by the BOARD_* flag in config.h:
//   BOARD_LCD_128     -> GC9A01 4-wire SPI + CST816S touch
//   BOARD_AMOLED_175  -> CO5300 QSPI + CST9217 touch
// Owns the panel + LVGL display driver so main.cpp stays glue-only.
#include <stdint.h>

namespace display {

// Init the panel and LVGL (draw buffers in PSRAM) and show the M0 hello screen.
// Returns false if the panel failed to initialize.
bool begin();

// Pump LVGL: render dirty areas + run LVGL timers. Call every loop() iteration.
void loop();

// 0..255 backlight duty (LEDC PWM on PIN_LCD_BL).
void setBrightness(uint8_t v);

// ms since the last touch (LVGL inactivity timer) — for idle auto-dim.
uint32_t inactiveMs();

// Rotate the whole UI in 90° steps (0/1/2/3 = 0°/90°/180°/270°), e.g. to mount with the
// USB-C port on any side. Applied in the flush (180° in place; 90/270° via a PSRAM
// scratch buffer) and mirrored on touch input.
void setRotation(uint8_t quarters);
uint8_t rotation();

} // namespace display

uint32_t display_frames();   // total rendered frames (for FPS measurement)
