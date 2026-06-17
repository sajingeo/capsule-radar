#pragma once
// Battery readout. Implementation switches at compile time:
//   BOARD_AMOLED_175  -> AXP2101 PMIC over I2C (XPowersLib).
//   BOARD_LCD_128     -> ETA6096 + voltage divider on PIN_BAT_ADC; charging is approximated.
// All calls run on the LVGL loop core so they don't fight the network task.
bool battery_begin();      // init; false if the PMIC isn't found
bool battery_present();    // a LiPo is connected
int  battery_percent();    // 0..100, or -1 if unknown
bool battery_charging();   // currently charging
void battery_enable_codec_rail();  // enable ALDO1 3.3V (ES8311 analog AVDD) for audio
