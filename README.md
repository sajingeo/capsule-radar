# Capsule Radar 🛩️

A live **ADS-B aircraft radar** for two Waveshare round-screen ESP32-S3 boards. Pulls nearby aircraft from a free online feed over WiFi and plots them on a touch radar scope centered on your location, with live flight details and a selectable **Dragon Ball "Dragon Radar"** skin.

| Board | Screen | Peripherals | PIO env |
|---|---|---|---|
| [**ESP32-S3-Touch-LCD-1.28**](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.28) | GC9A01 240×240 IPS, 4-wire SPI | CST816S touch, QMI8658 IMU, ETA6096 charger | `esp32-s3-touch-lcd-128` |
| [**ESP32-S3-Touch-AMOLED-1.75**](https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75) | CO5300 466×466 AMOLED, QSPI | CST9217 touch, AXP2101 PMIC, ES8311 audio, PCF85063 RTC | `esp32-s3-amoled-175` |

One source tree, board picked at compile time via `-DBOARD_*` flags. UI layout, peripheral drivers, and feed protocol all key off the selected board — see [CLAUDE.md](CLAUDE.md) for the split.

> Visual reference: open [`assets/plane_radar_2.0_mockup.html`](assets/plane_radar_2.0_mockup.html) in a browser.

<p align="center"><img src="docs/img/radar.gif" width="360" alt="Capsule Radar live scope"></p>

| Phosphor | Dragon (Capsule Corp) | Amber CRT | Military |
|:--:|:--:|:--:|:--:|
| ![Phosphor](docs/img/radar.png) | ![Dragon](docs/img/dragon.png) | ![Amber](docs/img/amber.png) | ![Military](docs/img/military.png) |

<sub>Captured from the bundled desktop simulator (the device screen is round; the square corners are off-panel).</sub>

## Features

- **Live traffic** from [airplanes.live](https://airplanes.live) (free, non-commercial; fallback adsb.lol), updated every couple of seconds. Memory-safe streaming parser with a hard aircraft cap.
- **Four themes** (long-press the screen to cycle, or pick on the web; remembered across reboots):
  - **Phosphor** — green-on-black radar scope: rings, animated sweep, aircraft glyphs rotated by heading and color-coded by altitude, fading trails, emergency halo.
  - **Dragon** — DBZ Dragon Radar skin: green gradient + grid, the 7 nearest aircraft as yellow "dragon balls" emitting waves, off-range traffic as edge arrows pointing its way, orange target rings.
  - **Amber CRT** and **Military** — the same scope retinted (warm amber / night-vision green).
- **Touch**: tap an aircraft → detail card (callsign, type, altitude, vertical speed, ground speed, distance, heading, squawk, and **origin → destination** looked up from adsbdb, cached in NVS). **Double-tap** or tap the on-screen zoom button to cycle range. Swipe between **Radar / List / Stats** (circular layouts).
- **Boot splash** + **boot-button reset**: hold the BOOT button on the back of the board for 3 s to clear WiFi credentials, 10 s for a full factory reset.
- **Audio alerts** (AMOLED kit only, ES8311 speaker): a soft ping when a new aircraft enters range, an urgent double-beep for emergency/military — volume & mute on the web page.
- **Smooth motion**: aircraft glyphs glide between polls (interpolated) instead of jumping, using cheap partial redraws.
- **Top HUD**: WiFi status (amber if the data feed is failing), in-range aircraft count, NTP/RTC clock, **battery %** (charging bolt, red when low), and the date. The Stats view footer shows how to reach the config page (`capsuleradar.local` + IP).
- **Battery aware**: shows charge level, warns when low, and slows the feed poll rate on battery to save power. (LCD-1.28 reads ADC voltage on GPIO1 through a 200k/100k divider; AMOLED uses the AXP2101 PMIC fuel gauge.)
- **Clock** synced from NTP after WiFi joins. **Time zone** is set on the web config page (12 presets + custom POSIX TZ); default is US Eastern. The AMOLED kit also has a PCF85063 RTC that keeps time across power loss.
- **Smart brightness**: configurable idle auto-dim (no touch), and **face-down sleep** (QMI8658 IMU — flip it over to turn the screen off).
- **Configuration web page** at `http://capsuleradar.local/` — center point, display range, theme, time zone, live brightness slider, WiFi reset. Settings persist in NVS.
- **First-boot WiFi setup** via a captive portal (`CapsuleRadar-Setup`).

## Hardware

Both supported boards are compared in the table at the top of this README. All pins are sourced from the official Waveshare demos and live in [`src/config.h`](src/config.h) — no guessing.

## Build & flash (PlatformIO)

Pick the env that matches your board:
```bash
pio run -e esp32-s3-touch-lcd-128 -t upload   # 240x240 IPS kit
pio run -e esp32-s3-amoled-175    -t upload   # 466x466 AMOLED kit
pio device monitor -b 115200                   # serial log
```
The `*-ota` variants of each env upload over WiFi (`upload_port = capsuleradar.local`) once the device is online. On first USB flash you may need to hold **BOOT** then tap **RESET**. After flashing, on first boot connect your phone to the **`CapsuleRadar-Setup`** WiFi and enter your home network — real aircraft appear within seconds.

## Flash from your browser (no toolchain)

Makers can flash without installing anything using **ESP Web Tools** (Chrome or Edge on desktop):

1. Open the **[web flasher](https://sajingeo.github.io/capsule-radar/)** (the project's GitHub Pages site).
2. Plug the board in with a USB-C **data** cable and click **Install**.

The flasher is built and published automatically by GitHub Actions ([`.github/workflows/webflasher.yml`](.github/workflows/webflasher.yml)) on every push to `main` — enable it once in **Settings → Pages → Source = GitHub Actions**. Tagged releases (`git tag v1.0.0 && git push origin v1.0.0`) also attach a ready-to-flash `CapsuleRadar-esp32s3.bin` to a **GitHub Release** via [`release.yml`](.github/workflows/release.yml). To preview the flasher locally:

```bash
./scripts/build_webflasher.sh                      # build + merge into web/flash/
python3 -m http.server -d web/flash 8000           # serve (Web Serial works on localhost)
# open http://localhost:8000
```

## Desktop simulator

The whole UI is portable LVGL and runs on your computer (SDL2) — great for iterating without hardware:
```bash
pio run -e native -t exec     # opens a 240×240 window (needs SDL2: `brew install sdl2`)
```
Mouse = touch · `T` = switch theme · close the window to quit.

## Configuration

Browse to `http://capsuleradar.local/` (or the device IP) on the same WiFi to set the **center lat/lon**, **display range**, **theme**, **time zone** and **brightness**, or to **reset WiFi**. Saving restarts the device to apply.

## Repo layout

```
src/
  config.h           per-board pins + tunables; UI scale macros; default home (Dénia, Spain)
  main.cpp           tasks, WiFi/NTP, web config, brightness/IMU glue, BOOT-button reset
  display.*          one file, two panel paths (GC9A01 SPI / CO5300 QSPI) + LVGL
  radar_view.*       the radar scope, aircraft, themes
  ui.*               views (radar/list/stats) + detail card + HUD
  touch.h            shared touch API
  touch_cst816s.*    LCD-1.28 driver (BOARD_LCD_128 only)
  touch_cst9217.*    AMOLED driver (BOARD_AMOLED_175 only)
  imu_qmi8658.*      accelerometer (face-down sleep)
  battery.*          ETA6096+ADC on LCD-1.28; AXP2101 PMIC on AMOLED
  rtc_pcf85063.*     PCF85063 on AMOLED; stub on LCD-1.28 (NTP-only)
  audio.*            ES8311 alert pings on AMOLED; stub on LCD-1.28
  adsb_client.*      airplanes.live fetch + parse (HTTP on LCD-1.28, HTTPS on AMOLED)
  route*.* route.*   origin→destination lookup (adsbdb)
  sim_main.cpp       native SDL simulator (not flashed)
include/lv_conf.h    LVGL config (v8)
web/flash/           browser web-flasher (ESP Web Tools) for makers
scripts/             build_webflasher.sh (merge firmware -> single .bin)
docs/                hardware / data-source / architecture notes
```

## Data & license

Aircraft data: **airplanes.live** (free, **non-commercial / educational** — exactly this project; be polite with request cadence). Routes: **adsbdb.com** (free). Personal/hobby project intended for a future MakerWorld release (3D-printed enclosure + this firmware).
