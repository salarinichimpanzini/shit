# SALARINI TOOL v9.0 — LOW RESOURCE UI

ESP32-C5 firmware project with a compact black/white security-lab style UI. The camera is removed. Hardware support retained: ST7796 LCD, FT6336 touch, SHTC3, QMI8658, PCF85063 RTC, AXP2101 PMU, SD and board I/O.

## UI

The main menu follows this tree:

```text
SALARINI TOOL
|-- Exploit
|   |-- Deauth
|   |   |-- APs
|   |   |   |-- All APs
|   |   |   |-- Channel
|   |   |   `-- Select Wifi
|   |   `-- STAs
|   |       `-- Select Station
|   |-- Evil Twin
|   |   `-- Select Wifi
|   |-- cool stuff
|   |   |-- deauth air play
|   |   |-- game1
|   |   `-- game2
|   |-- Beacon
|   |   |-- All SSIDs Dupe
|   |   |-- Selected Wifi Dupe
|   |   |-- Random
|   |   |-- Channel
|   |   `-- Prefix
|   |-- AP Spoofing
|   |   `-- Selected
|   `-- B. T Adv
|       |-- Samsung
|       `-- iOS
|-- Scan
|-- Packet Monitor
|-- Settings
`-- About
```

Exploit-named entries are navigation/placeholder slots. Implemented wireless functions are passive survey, channel/RSSI analysis and normal Wi-Fi connection. No deauthentication, jamming, packet injection, credential capture or other disruptive routines are included.

## Performance

- One vertical scrolling content container.
- Fixed-height 38–42 px rows.
- No shadows, gradients, transform/zoom effects or transparent screen layers.
- LVGL handler target: 16 ms (~60 FPS).
- LVGL logs and unused widgets disabled.
- Sensor refresh ~700 ms; UI refresh ~350 ms; Wi-Fi service ~350 ms.
- SD telemetry interval 15 s.
- Automatic NTP retry interval 15 s with a short 700 ms wait so Wi-Fi time sync does not freeze the UI for seconds.

## Build

Use Arduino IDE with ESP32-C5 Dev Module. Run `tools\install_libraries.bat` when you need the project-local libraries. Run `tools\preflight_low_resource.bat` before compiling.


LVGL FIX NOTE: `makeField()` uses `lv_obj_set_height(ta, 38)`; all setter calls must pass the LVGL object as the first argument. Run `tools\check_lvgl_setters.bat` before building.


## v11 fixes
- Rotation uses the ST7796 hardware MADCTL path and a 480 x 4-line DMA-capable draw buffer.
- Touch coordinates are recalculated from the stored rotation.
- No startup overlay is shown.
- Back navigation lives in the fixed header.
- Wi-Fi scan results live in the single page scroll container.
- Password entry is only present in `WIFI CONNECT`, not in scan/target selection.
- Packet Monitor uses a lightweight 13-channel occupancy bar graph instead of a heavy chart widget.
