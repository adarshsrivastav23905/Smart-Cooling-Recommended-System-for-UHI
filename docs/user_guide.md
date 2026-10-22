# User & Developer Guide

## Quickstart

### 1. Build the ESP32 Firmware
Install PlatformIO and the Espressif32 platform, then run:

```bash
pio run -d esp32_firmware -e esp32-espidf
```

For a virtual hardware demonstration, open the `esp32_firmware` project in VS Code and start Wokwi after the build completes. See [Wokwi Demonstration Guide](wokwi_demo.md) for the steps and expected readings.

### 2. Device Runtime
The ESP-IDF application samples the DHT22 every two seconds, computes heat index, drives the SSD1306 display and severity LEDs, activates the buzzer for extreme heat, and emits newline-delimited JSON over serial. The JSON distinguishes the firmware's threshold tier, the C-engine assessment, and the classifier prediction.

Only ambient air temperature and relative humidity are read from the DHT22. LST and urban-form features used by the C engine are not sensed by this circuit; they are unavailable or configured demonstration inputs.

### 3. Hardware Configuration
Pin assignments and sampling thresholds are defined in `esp32_firmware/src/main.c`. PlatformIO's ESP-IDF build environment is configured in `esp32_firmware/platformio.ini`.

### 4. Native C Tests and Simulation
The C engine tests and CLI simulation are built with GCC. Commands are documented in the repository [README](../README.md#-testing-and-validation).
