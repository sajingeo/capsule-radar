// No audio codec on the Waveshare ESP32-S3-Touch-LCD-1.28 (the AMOLED variant
// had ES8311; this kit doesn't). All entry points are no-ops; audio_present()
// returns false so main.cpp's alert path early-returns.
#include "audio.h"
#include <Arduino.h>

bool audio_begin()                  { Serial.println("[audio] no codec on LCD-1.28 kit"); return false; }
bool audio_present()                { return false; }
void audio_set_volume(int /*pct*/)  {}
void audio_set_muted(bool /*m*/)    {}
void audio_play(AudioCue /*cue*/)   {}
void audio_selftest()               {}
