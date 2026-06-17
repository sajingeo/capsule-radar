// CST816S capacitive touch over I2C (Arduino). Ported from Waveshare's
// ESP32-S3-Touch-LCD-1.28 demo: read 6 bytes from reg 0x01, decode 12-bit X/Y
// from data[2..5]. Single-touch is enough here.
#include "touch_cst816s.h"
#include "config.h"
#include <Arduino.h>
#include <Wire.h>

#define CST816S_REG_DATA 0x01
#define CST816S_DATA_LEN 6

// Read `len` bytes from an 8-bit register on the CST816S.
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

    // hardware reset (HIGH 50 ms, LOW 5 ms, HIGH 50 ms — per Waveshare demo)
    pinMode(PIN_TP_RST, OUTPUT);
    digitalWrite(PIN_TP_RST, HIGH);
    delay(50);
    digitalWrite(PIN_TP_RST, LOW);
    delay(5);
    digitalWrite(PIN_TP_RST, HIGH);
    delay(50);
    pinMode(PIN_TP_INT, INPUT_PULLUP);

    // probe: try to read one byte from the chip-id register
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
