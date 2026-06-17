// No on-board RTC on the Waveshare ESP32-S3-Touch-LCD-1.28. The clock relies on
// NTP after WiFi associates; before then `time(nullptr)` is 0 and the HUD must
// tolerate that. These are no-op stubs so main.cpp keeps compiling unchanged.
#include "rtc_pcf85063.h"
#include <Arduino.h>

bool rtc_begin()                       { Serial.println("[rtc] no on-board RTC (LCD-1.28 kit)"); return false; }
bool rtc_present()                     { return false; }
bool rtc_read(struct tm * /*out*/)     { return false; }
bool rtc_write(const struct tm * /*t*/) { return false; }
