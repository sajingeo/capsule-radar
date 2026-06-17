#pragma once
// Battery readout. ESP32-S3-Touch-LCD-1.28: ETA6096 charger + ADC on PIN_BAT_ADC.
// All calls run on the LVGL loop core so they don't fight the network task.
bool battery_begin();      // init; false if the PMIC isn't found
bool battery_present();    // a LiPo is connected
int  battery_percent();    // 0..100, or -1 if unknown
bool battery_charging();   // currently charging
void battery_enable_codec_rail();  // enable ALDO1 3.3V (ES8311 analog AVDD) for audio
