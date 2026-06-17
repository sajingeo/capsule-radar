#pragma once
// Minimal Arduino driver for the CST816S capacitive touch (I2C). Device-only.
// Protocol: read 6 bytes from reg 0x01 — see Waveshare ESP32-S3-Touch-LCD-1.28 demo.
#include <stdint.h>

bool touch_begin();                       // I2C + hardware reset; logs comms status
bool touch_read(uint16_t *x, uint16_t *y); // true if currently pressed (x,y in screen px)
