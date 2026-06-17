// Battery readout for the Waveshare ESP32-S3-Touch-LCD-1.28 (ETA6096 charger).
// No I2C fuel gauge: voltage is measured on PIN_BAT_ADC through a 200k/100k
// divider, then mapped to a percentage via a coarse Li-ion discharge curve.
// "charging" can't be detected without an extra GPIO from the charger, so we
// approximate: voltage above ~4.18 V (≈ float charge) => charging-ish.
#include "battery.h"
#include "config.h"
#include <Arduino.h>

static bool s_ok = false;
static float s_lastV = 0.0f;

bool battery_begin() {
    // On Arduino-ESP32 v3 the first analogRead() call auto-configures the pin as ADC.
    // Calling pinMode(INPUT) or analogSetPinAttenuation() before that breaks the
    // peripheral-manager bookkeeping and produces "Pin is not configured as analog channel".
    analogReadResolution(12);                 // 0..4095
    (void)analogRead(PIN_BAT_ADC);            // configure + warm-up sample
    analogSetPinAttenuation(PIN_BAT_ADC, ADC_11db);  // ~0..3.1 V at the pin -> ~0..9.3 V battery
    delay(2);
    const int raw = analogRead(PIN_BAT_ADC);
    s_lastV = (3.3f / 4095.0f) * BAT_DIVIDER_RATIO * (float)raw;
    s_ok = (s_lastV > 1.5f);                  // <1.5 V => no battery / running off USB
    Serial.printf("[batt] ETA6096: v=%.2fV (raw=%d) %s\n", s_lastV, raw, s_ok ? "present" : "absent");
    return s_ok;
}

static float read_voltage_filtered() {
    // Light EMA across 8 samples; the divider + ADC noise add ~30 mV jitter.
    int acc = 0;
    for (int i = 0; i < 8; ++i) { acc += analogRead(PIN_BAT_ADC); delayMicroseconds(200); }
    const float v = (3.3f / 4095.0f) * BAT_DIVIDER_RATIO * (float)(acc / 8);
    s_lastV = (s_lastV == 0.0f) ? v : (s_lastV * 0.8f + v * 0.2f);
    return s_lastV;
}

bool battery_present() {
    if (!s_ok) return false;
    return read_voltage_filtered() > 2.8f;    // disconnected pack reads ~0
}

int battery_percent() {
    if (!s_ok) return -1;
    const float v = read_voltage_filtered();
    // Coarse 1S Li-ion curve. Good enough for a HUD %.
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
