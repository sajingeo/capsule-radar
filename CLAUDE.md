# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is
Capsule Radar — firmware for the **Waveshare ESP32-S3-Touch-LCD-1.28** (round 240×240 IPS LCD, capacitive touch, IMU, Li-ion charger). Pulls live ADS-B traffic from airplanes.live (fallback adsb.lol) and renders a touch radar scope. Centered on Dénia, Spain by default; runtime-configurable via NVS + a built-in web page.

Visual reference: `assets/plane_radar_2.0_mockup.html`. Themes: Phosphor / Dragon / Amber CRT / Military.

> Hardware history: this codebase grew up on the larger ESP32-S3-Touch-AMOLED-1.75 (466×466 / CO5300 QSPI / CST9217 touch / PCF85063 RTC / AXP2101 PMIC / ES8311 audio). The current target is the smaller, cheaper 1.28" kit. Anywhere the code still references those parts, that's a migration leftover — see "Migration deltas from the AMOLED variant" below.

## Hardware (Waveshare ESP32-S3-Touch-LCD-1.28)
- **MCU**: ESP32-S3R2 — 2 MB in-package QSPI PSRAM, **no OPI PSRAM**. 512 KB SRAM.
- **Flash**: 16 MB NOR (W25Q128). USB-UART via CH343P.
- **Display**: GC9A01A, 240×240 round IPS, **4-wire SPI** (up to 80 MHz). Backlight on GPIO2 (PWM).
- **Touch**: CST816S, I2C addr `0x15`, shared bus.
- **IMU**: QMI8658, I2C addr `0x6B` (autoprobed `0x6A`/`0x6B` — board uses 0x6B), shared bus.
- **Battery**: ETA6096 charger (no I2C fuel-gauge). Battery voltage read via ADC on GPIO1 through a 200 kΩ / 100 kΩ divider — `vbat = (3.3 / 4096) * 3 * raw`.
- **Aux GPIO**: MOSFET switches on GPIO4 and GPIO5 (vibration motor / passive buzzer). UART TX/RX = GPIO43/44.
- **No on-board RTC, no PMIC over I2C, no audio codec.** Don't expect PCF85063 / AXP2101 / ES8311 here.

### Pin map (confirmed from `ESP32-S3-Touch-LCD-1.28-Demo/Arduino/examples/ESP32-S3-Touch-LCD-1.28-Test/DEV_Config.h` + `Setup207_GC9A01.h` + `LVGL_Arduino.ino`)
| Signal       | GPIO | Notes                                         |
|--------------|------|-----------------------------------------------|
| LCD MOSI     | 11   | 4-wire SPI                                    |
| LCD MISO     | 12   | unused by GC9A01 in practice                  |
| LCD SCLK     | 10   |                                               |
| LCD CS       | 9    |                                               |
| LCD DC       | 8    |                                               |
| LCD RST      | 14   |                                               |
| LCD BL       | 2    | PWM via `ledcWrite`                           |
| I2C SDA      | 6    | shared: CST816S + QMI8658                     |
| I2C SCL      | 7    |                                               |
| Touch RST    | 13   |                                               |
| Touch INT    | 5    | active-low, attach interrupt for tap latency  |
| Battery ADC  | 1    | divider 200 k / 100 k                         |

The wiki also mentions GPIO4/GPIO5 as "MOSFET switches" (vibration motor / buzzer). The official demo claims GPIO5 for Touch_INT, so that label likely refers to the SH1.0 GPIO breakout, not a populated on-board MOSFET. **Treat GPIO5 as Touch_INT** unless the schematic says otherwise.

## Build / flash / simulate
PlatformIO + Arduino framework. Three environments in `platformio.ini`:
```bash
pio run -e esp32-s3-touch-lcd-128 -t upload   # USB flash (CH343P, no BOOT/RESET dance usually)
pio run -e esp32-s3-touch-lcd-128-ota -t upload   # OTA over WiFi (espota -> capsuleradar.local)
pio run -e native -t exec                      # desktop SDL2 simulator (brew install sdl2)
pio device monitor -b 115200                   # serial log
```
The board ID, `board_build.psram_type`, and `board_upload.flash_size` need updating for this kit: ESP32-S3R2 uses **`qio_qspi`** PSRAM (NOT `qio_opi`) and the partition table should still target 16 MB. Single-bin web flasher: `./scripts/build_webflasher.sh` writes `web/flash/CapsuleRadar-esp32s3.bin` (merged bootloader+partitions+app at 0x0). CI does the same in `.github/workflows/webflasher.yml` (Pages) and `release.yml` (tagged `v*` → GitHub Release with both merged and OTA-only `.bin`).

There is no test suite. The simulator IS the iteration loop for UI work; flash to the board for anything that touches WiFi, IMU, battery, or the panel driver.

## Architecture
**Two-core split, one mutex.** Render path must never block on I/O.
- **Core 0 — `adsb_task`** (`main.cpp`): WiFi keepalive, polls airplanes.live every `POLL_INTERVAL_MS` (slowed to `POLL_INTERVAL_BATTERY_MS` on battery), writes a fresh `std::vector<Aircraft>` into `g_aircraft` under `g_ac_mutex` via `swap()` (O(1) handoff, no String copies under the lock). Same task also services on-demand route + photo lookups for the selected aircraft, but ALWAYS polls the feed first each cycle so a slow lookup can't starve it. Self-heals: if WiFi is up but the feed has been dead >180 s, `ESP.restart()` (NVS settings persist).
- **Core 1 — Arduino `loop()`**: `lv_timer_handler()`, IMU read, brightness logic, web server, OTA. Reads `g_aircraft` under the mutex and renders.

**Render layers.** `display.cpp` brings up GC9A01 via Arduino_GFX (4-wire SPI) and wires LVGL v8 draw buffers. **Memory budget is much tighter than on the AMOLED kit**: 2 MB PSRAM (QSPI, slower than the AMOLED's OPI) + 512 KB internal SRAM. A full RGB565 framebuffer at 240×240 is only ~115 KB, but prefer LVGL **partial buffers** in internal SRAM over a full-frame buffer in PSRAM — QSPI PSRAM is markedly slower for random access and the panel SPI clock is the bottleneck anyway. `radar_view.cpp` draws the scope (rings, sweep, glyphs rotated by `track`, fading trails, themes). `ui.cpp` owns the LVGL views (Radar / List / Stats), detail card, and HUD. Touch goes through `touch_cst816s.cpp` (replacement for the old CST9217 driver) → LVGL indev → hit test in `radar_view`.

**State that lives in `main.cpp`** (read by web handlers + UI): `g_settings`, `g_brightnessDay`, `g_idleDimMs`, `g_showSweep`, `g_units`, `g_showAirports`, `g_rotation`, `g_onBattery`. All persisted in NVS namespace `"capsuleradar"` via `Preferences`.

**Cross-core requeries.** Range zoom (double-tap) calls `onRangeChange()` which sets `g_requery` + `g_requeryKm`; `adsb_task` notices, calls `g_adsb.begin()` with the new radius, and forces an immediate poll. Don't call `AdsbClient::begin()` from core 1.

## Migration deltas from the AMOLED variant
This is the *current* hardware list; if the old code still references the AMOLED parts, the migration isn't complete:
- **CO5300 QSPI → GC9A01 4-wire SPI.** `display.cpp` switches to the Arduino_GFX `Arduino_GC9A01` constructor, no `LCD_COL_OFFSET` panel-gap quirk. Brightness is **PWM on GPIO2** (`ledcWrite`), not the AMOLED's `0x51` panel command — `display::setBrightness()` needs a rewrite.
- **CST9217 → CST816S.** Replace `touch_cst9217.*` with a CST816S driver (different I2C protocol; the SensorLib `TouchDrvCST816` works). `TP_MIRROR_X/Y` may flip — verify after the swap.
- **AXP2101 PMIC → ETA6096 + ADC.** Rip out XPowersLib. `battery.cpp` becomes a pure ADC reader on GPIO1 with the 200 kΩ / 100 kΩ divider. Charging-state detection is no longer free; either approximate from voltage trend or accept "battery %" only.
- **PCF85063 RTC → none.** Remove `rtc_pcf85063.*`. The clock relies on NTP after WiFi connect; before that, time is unknown. The HUD must tolerate `time(nullptr) == 0`.
- **ES8311 codec → none.** Remove `audio.*` and the alert-ping feature, OR retarget alerts to a passive buzzer / vibration motor on GPIO4/5 (MOSFET-switched). Drop `g_volume`, `g_muted`, `g_alertMode`, `g_proximityKm` from settings if alerts go away — and remove their NVS keys / web-page controls accordingly.
- **PSRAM type.** `platformio.ini` must change `board_build.psram_type = opi` → `qspi` and `board_build.arduino.memory_type = qio_opi` → `qio_qspi`. Forgetting this is the #1 way to ship a board that boot-loops.
- **Resolution 466 → 240.** `SCREEN_W/H`, `SCREEN_CX/CY`, `RADAR_R_OUTER_PX`, font sizes, glyph sizes, hit-test radii, and the boot splash all need re-tuning. Treat any `466` / `233` / `218` literal as a regression.
- **No `LCD_COL_OFFSET` / `LCD_ROW_OFFSET`** for GC9A01 (panel has no column gap).

## Native simulator
`sim_main.cpp` is desktop-only (its own `main()` + SDL). The firmware env excludes it via `build_src_filter = +<*> -<sim_main.cpp>`. The native env includes ONLY portable UI files — `radar_view.cpp + ui.cpp + route.cpp + photo.cpp + coastline.cpp + airports.cpp + sim_main.cpp`. **Implication:** any new file the radar/UI depends on must be added to BOTH the native `build_src_filter` and kept free of Arduino/WiFi includes, or the simulator build breaks. The simulator window should be 240×240 to match the new panel.

## Versioning
Bump `FW_VERSION` in `src/config.h` on every release. The CI flasher workflow greps that exact line (`grep -oE 'FW_VERSION "[0-9.]+"'`) to stamp the Pages site, so don't change the macro shape.

## Conventions
- C++17. No `delay()` / network / file I/O on core 1. All blocking I/O lives in `adsb_task`.
- Touch `g_aircraft` only under `xSemaphoreTake(g_ac_mutex, ...)`.
- Tunables in `config.h` — no magic numbers in render code.
- HTTPS uses `WiFiClientSecure::setInsecure()` (`ADSB_HTTPS_INSECURE=1`); acceptable for this hobby device, swap to a pinned root CA if that ever changes.
- airplanes.live is non-commercial / educational — keep `POLL_INTERVAL_MS >= 1000` and `ADSB_USER_AGENT` honest.
- New persistent settings: add a `Preferences` key in `loadSettings()` AND a save in the relevant write path (web handler / range change / theme switch).

## Generated data
`src/airports_data.h` and `src/coastline_data.h` are generated by `tools/gen_airports.py` and `tools/gen_coastline.py`. Don't hand-edit; re-run the generator if the source data changes.

## Detailed docs
`docs/HARDWARE.md`, `docs/DATA_SOURCE.md`, `docs/ARCHITECTURE.md`, `docs/SETUP.md`, `docs/FEATURES.md`, `docs/MAKERWORLD.md`, `docs/LISTING.md`. **These still describe the AMOLED variant** — update them as the migration lands.
