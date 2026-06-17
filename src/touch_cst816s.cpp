// CST816S capacitive touch over I2C — used on the ESP32-S3-Touch-LCD-1.28.
// Read 6 bytes from reg 0x01 (Waveshare ESP32-S3-Touch-LCD-1.28 demo).
#include "config.h"
#if defined(BOARD_LCD_128)

#include "touch.h"
#include <Arduino.h>
#include <Wire.h>

#define CST816S_REG_DATA 0x01
#define CST816S_DATA_LEN 6

static bool cst_read_reg(uint8_t reg, uint8_t *data, uint8_t len) {
    Wire.beginTransmission((uint8_t)I2C_ADDR_TOUCH);
    Wire.write(reg);
    if (Wire.endTransmission(true) != 0) return false;
    if (Wire.requestFrom((uint8_t)I2C_ADDR_TOUCH, len) < len) return false;
    for (uint8_t i = 0; i < len; ++i) data[i] = Wire.read();
    return true;
}

bool touch_begin() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);

    pinMode(PIN_TP_RST, OUTPUT);
    digitalWrite(PIN_TP_RST, HIGH);
    delay(50);
    digitalWrite(PIN_TP_RST, LOW);
    delay(5);
    digitalWrite(PIN_TP_RST, HIGH);
    delay(50);
    pinMode(PIN_TP_INT, INPUT_PULLUP);

    uint8_t v = 0;
    if (cst_read_reg(0x15, &v, 1)) {
        Serial.printf("[touch] CST816S responding (chip 0x%02X)\n", v);
    } else {
        Serial.println("[touch] CST816S not responding yet (will keep polling)");
    }
    return true;
}

bool touch_read(uint16_t *ox, uint16_t *oy) {
    uint8_t d[CST816S_DATA_LEN];
    if (!cst_read_reg(CST816S_REG_DATA, d, CST816S_DATA_LEN)) return false;

    const uint8_t points = d[1];
    if (points == 0) return false;

    uint16_t x = (uint16_t)((d[2] & 0x0F) << 8) | d[3];
    uint16_t y = (uint16_t)((d[4] & 0x0F) << 8) | d[5];

    if (x > SCREEN_W - 1) x = SCREEN_W - 1;
    if (y > SCREEN_H - 1) y = SCREEN_H - 1;
    if (TP_MIRROR_X) x = (SCREEN_W - 1) - x;
    if (TP_MIRROR_Y) y = (SCREEN_H - 1) - y;

    *ox = x;
    *oy = y;
    return true;
}

#endif // BOARD_LCD_128
