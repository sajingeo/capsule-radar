#pragma once
// Common capacitive-touch API. The implementation is selected at compile time:
//   BOARD_LCD_128     -> touch_cst816s.cpp (CST816S, 6-byte read at reg 0x01)
//   BOARD_AMOLED_175  -> touch_cst9217.cpp (CST9217, 10-byte read at reg 0xD000)
// Display code includes this; the driver files compile their bodies conditionally.
#include <stdint.h>

bool touch_begin();                          // I2C + hardware reset; logs comms status
bool touch_read(uint16_t *x, uint16_t *y);   // true if currently pressed (x,y in screen px)
