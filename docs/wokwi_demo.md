# Wokwi Demonstration Guide

This guide is for a repeatable ESP32 demonstration when physical hardware is unavailable. The Wokwi simulation demonstrates the firmware and configured peripherals; it does not validate real sensor accuracy or urban cooling impacts.

## 1. Build the firmware

Install PlatformIO Core and the Espressif32 platform, then run from the repository root:

```powershell
pio run -d esp32_firmware -e esp32-espidf
```

The Wokwi configuration in `esp32_firmware/wokwi.toml` expects the generated firmware files under `esp32_firmware/.pio/build/esp32-espidf/`.

## 2. Open and start the simulation

Open `esp32_firmware` as the PlatformIO project in VS Code, open `diagram.json`, and start the Wokwi simulation with the Wokwi extension. Open the serial monitor at **115200 baud**.

The circuit contains one DHT22 on GPIO 4, an SSD1306 OLED on I²C, three severity LEDs, and a buzzer on GPIO 5. These match the current firmware pin assignments.

## 3. Demonstrate the severity transitions

While the simulation is running, select the DHT22 and change its temperature. Allow at least one **2-second sample interval** after each change.

| DHT22 temperature | Expected firmware severity | Expected indicator |
|---:|---|---|
| 28°C | LOW, unless the calculated heat index crosses a configured threshold | Green LED |
| 33°C | MODERATE | Yellow LED |
| 37°C | HIGH | Red LED |
| 39°C | HIGH | Red LED and brief buzzer pulse |

Humidity affects the computed heat index and can move a reading into a higher tier. The firmware thresholds are demonstration settings: 31.0°C for moderate, 36.5°C for high, and 38.0°C for the buzzer. They are not universal public-health thresholds.

For each state, demonstrate:

1. The DHT22 values changing in the serial JSON output.
2. Temperature, humidity, heat index, and firmware severity on the OLED.
3. The matching status LED; at 39°C, the buzzer should pulse.
4. The separate C-engine severity and `rf_class` fields in the JSON packet.

The firmware severity is threshold-based. The C-engine assessment and classifier are separate outputs and may disagree with the LED tier. Explain that distinction rather than treating them as one classifier.

## 4. Interpret the telemetry packet correctly

The firmware reports DHT22 air temperature and relative humidity as sensor readings. `lst_c` is `null`, because the current circuit has no land-surface-temperature instrument. The C assessment still uses configured demonstration values for LST and other non-DHT22 features; the packet labels this in `source_label`.

Expected packet fields include:

- `ambient_temp_c`, `relative_humidity_pct`, and `heat_index_c`
- `severity`: firmware threshold tier used for the status LED
- `assessment_severity`: C-engine thermal assessment
- `rf_class` and `rf_confidence`: C classifier output
- `lst_c: null`: LST is not measured by this device
- `green_led`, `yellow_led`, and `red_led`: LED states; `buzzer`: whether the alarm pulse was triggered for that sample
- `source_label`: explains measured values and fixed demonstration inputs

Use the dashboard's **ESP32 Web Serial** option only when the simulator exposes its serial port to the browser or when using a physical ESP32. Wokwi's own serial monitor is sufficient to demonstrate the virtual firmware even if a browser serial port is not available.

## 5. Suggested presentation narration

> “This Wokwi circuit simulates one ESP32 node with a DHT22, OLED, three severity LEDs, and buzzer. The sensor provides air temperature and humidity every two seconds. The firmware calculates heat index, applies configured thresholds, and sends a JSON packet. The packet separately identifies the firmware threshold tier, the C engine assessment, and the classifier result. LST and urban-form indicators are not measured by this circuit, so they are marked as unavailable or demonstration inputs.”

## Troubleshooting

- **No serial output:** confirm the simulation is running, the serial monitor is open at 115200 baud, and the PlatformIO build succeeded.
- **DHT22 read faults:** check the GPIO 4 connection and restart the simulation. The firmware reports sensor errors rather than treating them as valid readings.
- **No OLED output:** verify SDA is GPIO 21, SCL is GPIO 22, and the display address is `0x3C`.
- **Expected LED is not active:** allow the next sample interval and check the DHT22 temperature and humidity; heat-index thresholds also influence classification.
- **Wokwi cannot find firmware:** build with PlatformIO and verify that `firmware.bin` and `firmware.elf` exist at the paths configured in `wokwi.toml`.
