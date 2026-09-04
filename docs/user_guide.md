# User & Developer Guide

## Quickstart

### 1. Build the Native Firmware
Install PlatformIO, connect an ESP32 DevKit V1, then run:

```bash
platformio run -d esp32_firmware -e esp32-espidf
platformio run -d esp32_firmware -e esp32-espidf --target upload
platformio device monitor -d esp32_firmware -b 115200
```

### 2. Device Runtime
The ESP-IDF application reads the DHT22, drives the SSD1306 display and
severity LEDs, activates the buzzer for extreme heat, and emits newline-
delimited telemetry JSON over serial. The native C engine also calculates
thermal stress, expected cooling impact, HVAC savings, and a recommendation.

### 3. Hardware Configuration
The pin assignments and sampling interval are defined in
`esp32_firmware/src/main.c`. The native C build is configured in
`esp32_firmware/platformio.ini`.
