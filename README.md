# Capsule Radar 🛩️

A live **ADS-B aircraft radar** for the **Waveshare ESP32-S3-Touch-LCD-1.28** — a round 240×240 IPS LCD with capacitive touch. It pulls nearby aircraft from a free online feed over WiFi and plots them on a touch radar scope centered on your location, with live flight details and a selectable **Dragon Ball "Dragon Radar"** skin.

> This fork targets the smaller, cheaper **ESP32-S3-Touch-LCD-1.28** kit (GC9A01 SPI, CST816S touch, QMI8658 IMU, ETA6096 charger, no audio codec, no on-board RTC). The original codebase was written for the bigger ESP32-S3-Touch-AMOLED-1.75 (466×466). See [CLAUDE.md](CLAUDE.md) for migration notes.

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
- **Touch** (CST816S): tap an aircraft → detail card (callsign, type, altitude, vertical speed, ground speed, distance, heading, squawk, and **origin → destination** looked up from adsbdb, cached in NVS). **Double-tap** or tap the on-screen zoom button to cycle range. Swipe between **Radar / List / Stats** (circular layouts).
- **Boot splash** + **boot-button reset**: hold the BOOT button on the back of the board for 3 s to clear WiFi credentials, 10 s for a full factory reset.
- **Smooth motion**: aircraft glyphs glide between polls (interpolated) instead of jumping, using cheap partial redraws.
- **Top HUD**: WiFi status (amber if the data feed is failing), in-range aircraft count, NTP/RTC clock, **battery %** (charging bolt, red when low), and the date. The Stats view footer shows how to reach the config page (`capsuleradar.local` + IP).
- **Battery aware** (ETA6096 + ADC voltage divider on GPIO1): shows charge level, warns when low, and slows the feed poll rate on battery to save power.
- **Clock** synced from NTP after WiFi joins. (No on-board RTC on this kit, so the time is unknown until the first NTP sync.)
- **Smart brightness**: configurable idle auto-dim (no touch), and **face-down sleep** (QMI8658 IMU — flip it over to turn the screen off).
- **Configuration web page** at `http://capsuleradar.local/` — center point, display range, theme, live brightness slider, WiFi reset. Settings persist in NVS.
- **First-boot WiFi setup** via a captive portal (`CapsuleRadar-Setup`).

## Hardware

Waveshare **ESP32-S3-Touch-LCD-1.28**: ESP32-S3R2 (2 MB QSPI PSRAM, 16 MB NOR flash), **GC9A01** 240×240 IPS over 4-wire SPI, **CST816S** touch, **QMI8658** IMU, **ETA6096** Li-ion charger. Pins are verified against the [Waveshare demo](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.28) and live in [`src/config.h`](src/config.h).

## Build & flash (PlatformIO)

```bash
pio run -e esp32-s3-touch-lcd-128 -t upload   # build + flash over USB-C
pio device monitor -b 115200                  # serial log
```
On first flash you may need to hold **BOOT** then tap **RESET**. After flashing, on first boot connect your phone to the **`CapsuleRadar-Setup`** WiFi and enter your home network — real aircraft appear within seconds.

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

Browse to `http://capsuleradar.local/` (or the device IP) on the same WiFi to set the **center lat/lon**, **display range**, **theme** and **brightness**, or to **reset WiFi**. Saving restarts the device to apply.

## Repo layout

```
src/
  config.h           pins + tunables (Dénia, Spain by default)
  main.cpp           tasks, WiFi/NTP, web config, brightness/IMU glue
  display.*          GC9A01 (Arduino_GFX, 4-wire SPI) + LVGL bring-up
  radar_view.*       the radar scope, aircraft, themes
  ui.*               views (radar/list/stats) + detail card + HUD
  touch_cst816s.*    capacitive touch driver
  imu_qmi8658.*      accelerometer (face-down sleep)
  battery.*          ETA6096: ADC voltage on GPIO1 + Li-ion percent curve
  rtc_pcf85063.*     stub (no on-board RTC on this kit)
  adsb_client.*      airplanes.live fetch + parse
  route*.* route.*   origin→destination lookup (adsbdb)
  sim_main.cpp       native SDL simulator (not flashed)
include/lv_conf.h    LVGL config (v8)
web/flash/           browser web-flasher (ESP Web Tools) for makers
scripts/             build_webflasher.sh (merge firmware -> single .bin)
docs/                hardware / data-source / architecture notes
```

## Data & license

Aircraft data: **airplanes.live** (free, **non-commercial / educational** — exactly this project; be polite with request cadence). Routes: **adsbdb.com** (free). Personal/hobby project intended for a future MakerWorld release (3D-printed enclosure + this firmware).
