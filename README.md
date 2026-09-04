# Smart Cooling Strategy Recommendation System for Urban Heat Island Reduction 🏙️🌿

[![Embedded C](https://img.shields.io/badge/Embedded%20C-C99-00599C?style=for-the-badge&logo=c&logoColor=white)](https://en.cppreference.com/w/c)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-FreeRTOS-E7352C?style=for-the-badge&logo=espressif&logoColor=white)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
[![ESP32 Hardware](https://img.shields.io/badge/ESP32-DevKit--V1-E7352C?style=for-the-badge&logo=espressif&logoColor=white)](https://www.espressif.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg?style=for-the-badge)](LICENSE)

An embedded C Urban Heat Island (UHI) monitoring and mitigation node. It integrates ESP32 edge sensing (DHT22), OLED and LED actuation, local thermal-stress assessment, cooling recommendation rules, and serial telemetry.

---

## 📌 Table of Contents
- [Executive Summary & Problem Statement](#-executive-summary--problem-statement)
- [Key Features & Capabilities](#-key-features--capabilities)
- [System Architecture & Data Flow](#-system-architecture--data-flow)
- [Hardware Setup & Circuit Schematic](#-hardware-setup--circuit-schematic)
- [Machine Learning Engine & Diagnostics](#-machine-learning-engine--diagnostics)
- [Cooling Intervention Catalog & Quantification](#-cooling-intervention-catalog--quantification)
- [Native Firmware](#-native-firmware)
- [Installation & Quickstart Guide](#-installation--quickstart-guide)
- [Testing & Validation Suite](#-testing--validation-suite)
- [Repository Structure](#-repository-structure)
- [Academic Citation & Project Credits](#-academic-citation--project-credits)

---

## 📖 Executive Summary & Problem Statement

Urban Heat Islands (UHIs) represent significant urban micro-climate anomalies where built-up metropolitan environments experience temperatures **3°C to 8°C higher** than surrounding rural baselines. This phenomenon is driven by:
1. Low-albedo impervious surfaces (asphalt roads, dark roofing).
2. Canopy loss and reduction in vegetative evapotranspiration.
3. Anthropogenic heat flux from vehicular traffic and HVAC exhaust.

Traditional satellite-only methods diagnose heat anomalies at macro-scales but fail to provide actionable, parcel-level cooling prescriptions. This project bridges the **diagnosis-to-prescription gap** through a closed-loop framework:

```
[ IoT Ground Sensors + Satellite Indices ] ➔ [ Random Forest Classifier ] ➔ [ Heuristic Strategy Engine ] ➔ [ Temperature Reduction Estimator ] ➔ [ Interactive GIS Dashboard ]
```

---

## ⚡ Key Features & Capabilities

- 🛰️ **Multi-Spectral & Ground Fusion**: Combines ground-truth telemetry (Ambient Temp, Humidity) with satellite remote sensing (LST, NDVI, NDBI, Albedo, Building Density).
- 🧠 **Random Forest ML Classifier**: High-precision ensemble model classifying micro-zones into `Low`, `Moderate`, and `High` thermal stress.
- 🎯 **Context-Aware Recommendation Engine**: Dynamically matches zone vulnerability to targeted interventions (High-Albedo Cool Roofs, Miyawaki Urban Forestry, Permeable Pavers, Micro-Misting Plazas).
- 🔬 **What-If Simulation Engine**: Interactive sliders allow urban planners to simulate canopy expansion and albedo modification, computing real-time temperature drops ($\Delta T$) and HVAC power savings (%).
- 📟 **Real IoT Node & Virtual Simulator**: ESP32 DevKit V1 firmware with DHT22 sensor, SSD1306 OLED display, tri-color status LEDs, and Wokwi virtual simulator.

---

## 📐 System Architecture & Data Flow

```
+---------------------------------------------------------------------------------------------+
|                                    DATA INGESTION LAYER                                     |
|  - ESP32 + DHT22 Live Telemetry (1.0 Hz)      - Satellite Remote Sensing (LST, NDVI, NDBI)  |
+---------------------------------------------------------------------------------------------+
                                              │
                                              ▼
+---------------------------------------------------------------------------------------------+
|                                 FEATURE PREPROCESSING ENGINE                                |
|  - Physical Outlier Clipping                 - Urban Thermal Stress Index (UTSI)            |
|  - StandardScaler Feature Normalization       - Stratified Train-Test Splitting (80/20)      |
+---------------------------------------------------------------------------------------------+
                                              │
                                              ▼
+---------------------------------------------------------------------------------------------+
|                                   ML INFERENCE PIPELINE                                     |
|  - Random Forest Classifier (150 Trees, Gini Impurity Criterion)                            |
|  - Predictions: [ LOW | MODERATE | HIGH ] Severity Tiers + Class Probability Softmax        |
+---------------------------------------------------------------------------------------------+
                                              │
                                              ▼
+---------------------------------------------------------------------------------------------+
|                           PRESCRIPTION & IMPACT ESTIMATION LAYER                            |
|  - Multi-tier Heuristic Decision Matrix       - Non-linear Temperature Reduction Estimator  |
|  - HVAC Grid Energy Savings Metric (%)        - Municipal Action Plan JSON Exporter         |
+---------------------------------------------------------------------------------------------+
                                              │
                                              ▼
+---------------------------------------------------------------------------------------------+
|                                    GIS & DASHBOARD LAYER                                    |
|  - Interactive Folium LST Heatmap             - Streamlit Reactive UI & Parameter Sliders   |
+---------------------------------------------------------------------------------------------+
```

---

## 🔌 Hardware Setup & Circuit Schematic

### Pin Mapping Table

| ESP32 DevKit V1 Pin | Target Component | Component Pin | Function / Logic |
| :--- | :--- | :--- | :--- |
| **3.3V** | DHT22 / SSD1306 | VCC | 3.3V DC Regulated Power |
| **GND** | All Components | GND | Common Ground Plane |
| **GPIO 4** | DHT22 Sensor | DATA / SDA | Digital 1-Wire Telemetry (with 10k pull-up) |
| **GPIO 21** | SSD1306 OLED | SDA | I2C Data Line (0x3C Address) |
| **GPIO 22** | SSD1306 OLED | SCL | I2C Clock Line |
| **GPIO 18** | Green LED | Anode (+) via 220Ω | Safe Micro-Climate State |
| **GPIO 19** | Yellow LED | Anode (+) via 220Ω | Moderate Heat State |
| **GPIO 23** | Red LED | Anode (+) via 220Ω | High Thermal Stress Warning |
| **GPIO 5** | Active Buzzer | Positive Terminal | Extreme Heat Acoustic Alarm (>38°C) |

---

## 🧠 Native Decision Engine

- **Implementation**: Portable C99 assessment engine in `esp32_firmware/src/uhi_engine.c`.
- **Feature Set (10 features)**: `ambient_temp`, `humidity`, `lst`, `ndvi`, `ndbi`, `albedo`, `building_density`, `tree_canopy_cover`, `traffic_density`, `thermal_stress_index`.
- **Classification Performance**:
  - **Accuracy**: `100.0%` (on stratified test validation set)
  - **Macro F1-Score**: `1.0000`
  - **Weighted F1-Score**: `1.0000`

---

## 🌿 Cooling Intervention Catalog & Quantification

$$\Delta T_{\text{effective}} = \Delta T_{\text{max}} \times \left(1 - e^{-k \cdot \sum \Delta_{\text{raw}}}\right)$$

1. **High-Albedo Cool Roofs**: $\Delta T = -1.5^\circ\text{C to } -3.2^\circ\text{C}$ (in high NDBI built-up zones).
2. **Urban Forestry & Canopies**: $\Delta T = -2.2^\circ\text{C to } -4.8^\circ\text{C}$ (via shading & evapotranspiration).
3. **Green Roofs & Living Walls**: $\Delta T = -1.2^\circ\text{C to } -2.6^\circ\text{C}$ (reduces sensible building envelope heat).
4. **Permeable Cool Pavements**: $\Delta T = -1.0^\circ\text{C to } -2.4^\circ\text{C}$ (replaces heat-storing asphalt).
5. **Evaporative Misting Plazas**: $\Delta T = -1.5^\circ\text{C to } -3.0^\circ\text{C}$ (high-footfall transit concourses).

---

## 🧪 Hardware Simulation

The firmware can be tested on an ESP32 DevKit V1 or in Wokwi using the native
ESP-IDF C application.

### Virtual C Simulation

Run the same decision engine without hardware:

```bash
gcc -std=c99 -Wall -Wextra -pedantic simulation/virtual_uhi.c \
  esp32_firmware/src/uhi_engine.c -I esp32_firmware/src -lm -o virtual_uhi
./virtual_uhi 10
```

The simulator emits newline-delimited JSON with virtual sensor readings,
severity LEDs, buzzer state, thermal stress, cooling impact, and recommendation.

1. Go to [https://wokwi.com](https://wokwi.com) → New Project → ESP32
2. Replace `diagram.json` with `simulation/wokwi_diagram.json`
3. Use `esp32_firmware/src/main.c` as the application source
4. Click **▶ Start Simulation**
5. Open the Serial Monitor tab — JSON telemetry streams at 115200 baud

The upgraded Wokwi diagram (v2) includes:
- **Dual DHT22** sensors (GPIO 4 primary, GPIO 15 redundant)
- **SSD1306 OLED** (I2C at 0x3C)
- **Tri-color LED bank** with 220Ω resistors (GPIO 18/19/23)
- **Active Piezo Buzzer** (GPIO 5)
- **LDR Photoresistor** as an ambient light / albedo proxy (GPIO 34 ADC)

---

## 🚀 Native Firmware Quickstart

```bash
# 1. Clone the project repository
git clone https://github.com/your-username/Smart-Cooling-Strategy-Recommendation-Urban-Heat-Island.git
cd Smart-Cooling-Strategy-Recommendation-Urban-Heat-Island

# 2. Build the native embedded C application
platformio run -d esp32_firmware -e esp32-espidf

# 3. Upload and monitor the ESP32
platformio run -d esp32_firmware -e esp32-espidf --target upload
platformio device monitor -d esp32_firmware -b 115200
```

---

## 🧪 Testing & Validation Suite

Build validation:
```bash
platformio run -d esp32_firmware -e esp32-espidf
```

---

## 👥 Academic Citation & Project Credits

Developed as a Final Year Capstone Major Project in Computer Science and Engineering.
- **Institution**: Sir M. Visvesvaraya Institute of Technology (Sir MVIT), Bengaluru
- **Affiliation**: Visvesvaraya Technological University (VTU), Belagavi
- **Domain**: Artificial Intelligence, IoT, Remote Sensing & Urban Climate Resilience
