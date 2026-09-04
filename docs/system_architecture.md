# System Architecture & Technical Specifications

## 1. System Overview
The **Smart Cooling Strategy Recommendation System for Urban Heat Island Reduction** is a multi-tier IoT, Remote Sensing, Machine Learning, and GIS decision-support platform designed to monitor micro-climatic thermal anomalies across urban micro-zones and recommend localized, actionable cooling interventions with quantified thermal mitigation estimates.

```
+----------------------------------------------------------------------------------------------------+
|                                    DATA ACQUISITION LAYER                                          |
|                                                                                                    |
|   +---------------------------------------+         +------------------------------------------+   |
|   |         IoT Ground Telemetry          |         |    Satellite Earth Observation (EO)      |   |
|   |  - ESP32 DevKit V1 (Tensilica Dual-Core)|       |  - Landsat-8 TIR / Sentinel-2 Multi-Band|   |
|   |  - DHT22 Temp & Relative Humidity     |         |  - LST (Land Surface Temperature in °C) |   |
|   |  - SSD1306 OLED & Acoustic Actuators  |         |  - NDVI (Normalized Veg Index: NIR-Red) |   |
|   |  - JSON Telemetry Stream (Serial/WiFi)|         |  - NDBI (Built-Up Index: SWIR-NIR)      |   |
|   +---------------------------------------+         +------------------------------------------+   |
+----------------------------------------------------------------------------------------------------+
                                                  │
                                                  ▼
+----------------------------------------------------------------------------------------------------+
|                                DATA PREPROCESSING & FEATURE PIPELINE                               |
|                                                                                                    |
|   - Physical Boundary Clipping (Temp: 15-50°C, Humidity: 5-100%, NDVI: -1 to 1)                   |
|   - Missing Value Median Imputation                                                                |
|   - Urban Thermal Stress Score = f(LST, Ambient Temp, Humidity, NDVI, NDBI)                       |
|   - Multi-zone Stratified Train/Test Partitioning & StandardScaler Normalization                   |
+----------------------------------------------------------------------------------------------------+
                                                  │
                                                  ▼
+----------------------------------------------------------------------------------------------------+
|                                MACHINE LEARNING INFERENCE ENGINE                                   |
|                                                                                                    |
|   +--------------------------------------------------------------------------------------------+   |
|   |                        Random Forest Classifier (Ensemble = 150 Trees)                     |   |
|   |   - Inputs: 10 Environmental & Spectral Parameters                                         |   |
|   |   - Output: Heat Severity Classification [ LOW | MODERATE | HIGH ]                         |   |
|   |   - Confidence Score & Gini Impurity Feature Importance                                    |   |
|   +--------------------------------------------------------------------------------------------+   |
+----------------------------------------------------------------------------------------------------+
                                                  │
                                                  ▼
+----------------------------------------------------------------------------------------------------+
|                             DECISION MATRIX & IMPACT PREDICTION LAYER                              |
|                                                                                                    |
|   +-----------------------------------------+     +--------------------------------------------+   |
|   |     Context-Aware Cooling Engine        |     |       Predictive Impact Estimator          |   |
|   |  - High NDBI & Low Albedo -> Cool Roofs |     |  - ΔT_effective = ΔT_max * (1 - exp(-k*Δ)) |   |
|   |  - Low NDVI/Canopy -> Urban Forestry    |     |  - Non-linear Synergy & Diminishing Return |   |
|   |  - High Footfall -> Misting Canopies    |     |  - Energy Savings (%) = ΔT * 2.2% HVAC red.|   |
|   +-----------------------------------------+     +--------------------------------------------+   |
+----------------------------------------------------------------------------------------------------+
                                                  │
                                                  ▼
+----------------------------------------------------------------------------------------------------+
|                                    PRESENTATION & GIS LAYER                                        |
|                                                                                                    |
|   - Interactive Folium GIS Map with LST Heat Density Overlays & Colored Zone Markers               |
|   - Streamlit Reactive Control Dashboard with Micro-Zone Selector & Telemetry Mirror               |
|   - What-If Urban Planning Slider Simulator (Canopy +%, Albedo +, Permeable Pavement +%)           |
|   - Automated Municipal Action Plan Export (JSON / PDF)                                            |
+----------------------------------------------------------------------------------------------------+
```

## 2. Input/Output Parameter Specification

| Parameter | Type | Domain / Range | Source | Physical Meaning |
| :--- | :--- | :--- | :--- | :--- |
| `ambient_temp` | Float | 15.0°C to 50.0°C | ESP32 DHT22 / Weather API | Ambient dry-bulb air temperature |
| `humidity` | Float | 5.0% to 100.0% | ESP32 DHT22 / Weather API | Relative humidity percentage |
| `lst` | Float | 15.0°C to 60.0°C | Satellite TIR (Thermal Band) | Radiometric surface temperature |
| `ndvi` | Float | -1.00 to +1.00 | Sentinel-2 / Landsat-8 (NIR, Red)| Vegetation density & canopy health |
| `ndbi` | Float | -1.00 to +1.00 | Sentinel-2 / Landsat-8 (SWIR, NIR)| Built-up & impermeable surface index |
| `albedo` | Float | 0.05 to 0.85 | Remote Sensing Optical Bands | Surface shortwave solar reflectance |
| `building_density`| Float | 0.0% to 100.0% | GIS Cadastral Data | Built footprint ratio per hectare |
| `tree_canopy_cover`| Float | 0.0% to 100.0% | LiDAR / GIS Forestry Layer | High-foliage tree canopy percentage |
| `traffic_density` | Integer | 0 to 5000 veh/hr | Smart City Traffic Cameras | Anthropogenic heat & exhaust source |
| `thermal_stress_index` | Float | 0.0 to 100.0 | Engineered Feature | Composite thermal discomfort index |

---

## 3. Simulation Layer Architecture

The simulation layer provides two operational modes that allow the system to operate without physical hardware.

### 3.1 Data Flow Diagram

```
╔══════════════════════════════════════════════════════════════════════════╗
║                        SIMULATION LAYER                                  ║
╠═══════════════════════════════╦══════════════════════════════════════════╣
║   VIRTUAL SIMULATOR MODE      ║   HARDWARE (SERIAL) MODE                 ║
║                               ║                                          ║
║  VirtualSensorNode            ║  ESP32 DevKit V1 (physical)             ║
║  ├─ Diurnal temperature cycle ║  ├─ DHT22 Primary    (GPIO 4)           ║
║  ├─ Gaussian noise injection  ║  ├─ DHT22 Redundant  (GPIO 15)          ║
║  ├─ Environmental drift       ║  ├─ SSD1306 OLED     (I2C 0x3C)         ║
║  └─ Fault injection (NaN)     ║  ├─ Tri-Color LEDs   (GPIO 18/19/23)    ║
║         │                     ║  ├─ Piezo Buzzer     (GPIO 5)           ║
║         ▼                     ║  └─ LDR Albedo Proxy (GPIO 34 ADC)      ║
║  SimulationRunner             ║         │                                ║
║  ├─ live_telemetry.json       ║         │ USB Serial (115200 baud)       ║
║  └─ hardware_state.json       ║         │ JSON stream                    ║
║         │                     ║         │                                ║
║         ▼                     ║         ▼                                ║
║  VirtualActuatorPanel         ║  pyserial readline()                    ║
║  ├─ GPIO 18 → Green LED       ║  json.loads() → packet dict             ║
║  ├─ GPIO 19 → Yellow LED      ║         │                                ║
║  ├─ GPIO 23 → Red LED         ║         ▼                                ║
║  └─ GPIO 5  → Buzzer          ║  Dashboard Tab 4 (Hardware Mode)        ║
║         │                     ║                                          ║
║         ▼                     ║                                          ║
║  VirtualOLEDDisplay           ║                                          ║
║  └─ SSD1306 text emulator     ║                                          ║
║         │                     ║                                          ║
║         ▼                     ║                                          ║
║  Dashboard Tab 4              ║                                          ║
║  (Virtual Simulator Mode)     ║                                          ║
╚═══════════════════════════════╩══════════════════════════════════════════╝
                                │
                                ▼
              ┌─────────────────────────────────────────┐
              │       WOKWI BROWSER SIMULATION          │
              │  simulation/wokwi_diagram.json          │
              │  ├─ ESP32 DevKit C V4                   │
              │  ├─ DHT22 Primary (GPIO 4)              │
              │  ├─ DHT22 Redundant (GPIO 15)           │
              │  ├─ SSD1306 OLED (I2C)                  │
              │  ├─ Tri-Color LEDs + Resistors          │
              │  ├─ Active Piezo Buzzer                 │
              │  └─ LDR Photoresistor (GPIO 34 ADC)     │
              └─────────────────────────────────────────┘
```

### 3.2 Simulation Module Descriptions

| Module | File | Role |
| :--- | :--- | :--- |
| Native telemetry task | `esp32_firmware/src/main.c` | Reads DHT22 telemetry, controls GPIO/OLED, and emits serial JSON |
| Embedded decision engine | `esp32_firmware/src/uhi_engine.c` | Computes thermal stress, impact, savings, and recommendations |
| PlatformIO target | `esp32_firmware/platformio.ini` | Builds the ESP-IDF Embedded C application |
| Wokwi Diagram | `simulation/wokwi_diagram.json` | Full ESP32 circuit definition for browser-based hardware simulation at wokwi.com |

### 3.3 Shared State Files

| File | Written By | Read By | Contents |
| :--- | :--- | :--- | :--- |
| Serial telemetry | Native firmware | Serial monitor | Latest packet and edge assessment |

### 3.4 Wokwi Hardware Simulation

To simulate the full circuit in a browser without physical components:
1. Open [https://wokwi.com](https://wokwi.com) and create a new project.
2. Replace the default `diagram.json` with the contents of `simulation/wokwi_diagram.json`.
3. Use `esp32_firmware/src/main.c` as the application source.
4. Click **▶ Run** — the serial monitor will stream live JSON telemetry packets.

