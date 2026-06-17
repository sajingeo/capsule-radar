// Battery readout. Two paths:
//   BOARD_AMOLED_175  -> AXP2101 PMIC over I2C (XPowersLib).
//   BOARD_LCD_128     -> ETA6096 charger; voltage on ADC (PIN_BAT_ADC) via 200k/100k divider.
// Both share the I2C / ADC bus with other peripherals — call only from the LVGL loop
// (core 1), never from the network task, to avoid contention.
#include "battery.h"
#include "config.h"
#include <Arduino.h>

#if defined(BOARD_AMOLED_175)
// =========== AMOLED: AXP2101 via XPowersLib =================================
#include <Wire.h>
#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"

static XPowersPMU PMU;
static bool s_ok = false;

bool battery_begin() {
    s_ok = PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, PIN_I2C_SDA, PIN_I2C_SCL);
    if (s_ok) {
        PMU.enableBattDetection();
        PMU.enableBattVoltageMeasure();
        Serial.println("[batt] AXP2101 ready");
    } else {
        Serial.println("[batt] AXP2101 not found");
    }
    return s_ok;
}

bool battery_present()  { return s_ok && PMU.isBatteryConnect(); }
int  battery_percent()  { return s_ok ? PMU.getBatteryPercent() : -1; }
bool battery_charging() { return s_ok && PMU.isCharging(); }

void battery_enable_codec_rail() {
    if (!s_ok) return;
    PMU.setALDO1Voltage(3300);   // ES8311 analog supply (the reference board enables this)
    PMU.enableALDO1();
    Serial.println("[batt] ALDO1 (codec AVDD) enabled @3.3V");
}

#elif defined(BOARD_LCD_128)
// =========== LCD-1.28: ETA6096 + ADC voltage divider ========================
static bool s_ok = false;
static float s_lastV = 0.0f;

bool battery_begin() {
    // Arduino-ESP32 v3 auto-configures the pin on first analogRead(); calling
    // pinMode/analogSetPinAttenuation before that breaks the peripheral manager.
    analogReadResolution(12);                 // 0..4095
    (void)analogRead(PIN_BAT_ADC);            // configure + warm-up sample
    analogSetPinAttenuation(PIN_BAT_ADC, ADC_11db);
    delay(2);
    const int raw = analogRead(PIN_BAT_ADC);
    s_lastV = (3.3f / 4095.0f) * BAT_DIVIDER_RATIO * (float)raw;
    s_ok = (s_lastV > 1.5f);
    Serial.printf("[batt] ETA6096: v=%.2fV (raw=%d) %s\n", s_lastV, raw, s_ok ? "present" : "absent");
    return s_ok;
}

static float read_voltage_filtered() {
    int acc = 0;
    for (int i = 0; i < 8; ++i) { acc += analogRead(PIN_BAT_ADC); delayMicroseconds(200); }
    const float v = (3.3f / 4095.0f) * BAT_DIVIDER_RATIO * (float)(acc / 8);
    s_lastV = (s_lastV == 0.0f) ? v : (s_lastV * 0.8f + v * 0.2f);
    return s_lastV;
}

bool battery_present() {
    if (!s_ok) return false;
    return read_voltage_filtered() > 2.8f;
}

int battery_percent() {
    if (!s_ok) return -1;
    const float v = read_voltage_filtered();
    if (v >= 4.20f) return 100;
    if (v >= 4.10f) return 90 + (int)((v - 4.10f) / 0.01f);
    if (v >= 4.00f) return 80 + (int)((v - 4.00f) * 100.0f);
    if (v >= 3.85f) return 60 + (int)((v - 3.85f) / 0.0075f);
    if (v >= 3.75f) return 40 + (int)((v - 3.75f) / 0.005f);
    if (v >= 3.65f) return 20 + (int)((v - 3.65f) / 0.005f);
    if (v >= 3.45f) return  5 + (int)((v - 3.45f) / 0.01f);
    if (v >= 3.20f) return  1;
    return 0;
}

bool battery_charging() {
    // Without a CHRG pin we can only guess: voltage at/above the float-charge
    // plateau (~4.18 V) almost always means the charger is active.
    return s_ok && s_lastV >= 4.18f;
}

void battery_enable_codec_rail() { /* no codec on this board — no-op */ }

#endif // BOARD_*
