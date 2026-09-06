# Smart Cooling Strategy Recommendation System for Urban Heat Island Reduction 🏙️🌿
    
[![Language: Pure C99](https://img.shields.io/badge/Language-Pure%20C99-00599C?style=for-the-badge&logo=c&logoColor=white)](https://en.cppreference.com/w/c)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-FreeRTOS-E7352C?style=for-the-badge&logo=espressif&logoColor=white)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
[![ESP32 Hardware](https://img.shields.io/badge/ESP32-DevKit--V1-E7352C?style=for-the-badge&logo=espressif&logoColor=white)](https://www.espressif.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg?style=for-the-badge)](LICENSE)

An embedded C decision-support system and IoT monitoring framework for Urban Heat Island (UHI) mitigation. It integrates ESP32 edge sensing (DHT22), Random Forest heat severity classification in C, a 5-intervention context-aware recommendation engine, non-linear temperature reduction impact estimation ($\Delta T_{\text{effective}}$), What-If scenario simulation, and an interactive GIS heatmap dashboard.
     
---

## 📌 Table of Contents           
- [Executive Summary & Problem Statement](#-executive-summary--problem-statement)
- [Key Features & Capabilities](#-key-features--capabilities)
- [System Architecture (6-Stage Pipeline)](#-system-architecture-6-stage-pipeline)
- [Hardware Setup & Circuit Schematic](#-hardware-setup--circuit-schematic)
- [Machine Learning & Embedded Random Forest Classifier in C](#-machine-learning--embedded-random-forest-classifier-in-c)
- [5-Cooling Intervention Catalog & Quantification](#-5-cooling-intervention-catalog--quantification)
- [What-If Scenario Simulation & Comparison Engine](#-what-if-scenario-simulation--comparison-engine)
- [Virtual C Simulation & CLI Guide](#-virtual-c-simulation--cli-guide)
- [Interactive GIS Dashboard](#-interactive-gis-dashboard)
- [Native Firmware Quickstart](#-native-firmware-quickstart)
- [Testing & Validation Suite](#-testing--validation-suite)
- [Academic Citation & Project Credits](#-academic-citation--project-credits)

---

## 📖 Executive Summary & Problem Statement

Urban Heat Islands (UHIs) represent micro-climatic thermal anomalies where dense metropolitan zones experience temperatures **3°C to 8°C higher** than surrounding rural baselines due to:
1. Low-albedo impervious surfaces (asphalt roads, dark roofing).
2. Canopy loss and reduction in vegetative evapotranspiration.
3. Anthropogenic heat flux from vehicular traffic and HVAC exhaust.

Traditional methods are primarily diagnostic and macro-scale. This project bridges the **diagnosis-to-prescription gap** through a closed-loop six-stage pipeline implemented in pure C:

```
[ Ground Sensors + Satellite Observation ] ➔ [ Data Preprocessing in C ] ➔ [ Micro-Zone Segmentation ] ➔ [ Random Forest Classifier in C ] ➔ [ 5-Cooling Strategy Engine ] ➔ [ Non-Linear Impact Estimation ] ➔ [ GIS Dashboard & What-If Planner ]
```

---

## ⚡ Key Features & Capabilities

- 🛰️ **Multi-Source Data Fusion**: Combines ground-truth telemetry (Ambient Air Temp, Humidity) with satellite remote sensing (LST, NDVI, NDBI, Albedo, Building Density, Population Density).
- 🧠 **Embedded C Random Forest Classifier**: High-precision ensemble model in pure C99 classifying micro-zones into `LOW`, `MODERATE`, and `HIGH` heat tiers with confidence scores and Gini feature importances.
- 🎯 **Prescriptive 5-Strategy Engine**: Recommends targeted cooling interventions (Cool Roofs, Urban Forestry, Green Roofs & Living Walls, Permeable Cool Pavements, Evaporative Misting Plazas).
- 🔬 **Quantified Impact & What-If Simulator**: Computes non-linear temperature drops ($\Delta T_{\text{effective}}$), HVAC grid power savings (%), budget estimates (INR), and carbon offset (Tons CO₂/year).
- 🗺️ **Interactive GIS Micro-Zone Heatmap**: Interactive Leaflet map displaying parcel-level thermal variations and diagnostic cards.
- 📟 **Real IoT Node & Virtual Simulator**: ESP32 DevKit V1 firmware with DHT22, SSD1306 OLED, tri-color status LEDs, active buzzer, and virtual C CLI simulator.

---

## 📐 System Architecture (6-Stage Pipeline)

```
+---------------------------------------------------------------------------------------------+
|                                  1. DATA ACQUISITION LAYER                                  |
|  - Ground Sensors: ESP32 DevKit V1 + DHT11/DHT22 Temp & Humidity                            |
|  - Satellite Earth Observation: Landsat-8 TIR (LST) & Sentinel-2 (NDVI, NDBI, Albedo)       |
+---------------------------------------------------------------------------------------------+
                                              │
                                              ▼
+---------------------------------------------------------------------------------------------+
|                             2. DATA PREPROCESSING ENGINE (C99)                              |
|  - Physical Outlier Boundary Clipping (Temp: 15-50°C, Humidity: 5-100%, NDVI: -1 to +1)     |
|  - Missing Value Climatological Median Imputation                                           |
|  - Urban Thermal Stress Index (UTSI) Computation: UTSI = f(LST, Temp, Hum, NDBI, NDVI, ...)  |
+---------------------------------------------------------------------------------------------+
                                              │
                                              ▼
+---------------------------------------------------------------------------------------------+
|                                3. MICRO-ZONE SEGMENTATION                                   |
|  - Partitioning urban study area into fine-grained micro-zones (Commercial, Transit, Parks) |
|  - Parsing & ingestion via C microzone evaluator                                            |
+---------------------------------------------------------------------------------------------+
                                              │
                                              ▼
+---------------------------------------------------------------------------------------------+
|                           4. RANDOM FOREST HEAT CLASSIFIER (C99)                            |
|  - 25-Tree Diverse Ensemble Classifier evaluating multi-spectral & ground parameters        |
|  - Predictions: [ LOW | MODERATE | HIGH ] + Softmax Probability Distribution & Confidence  |
+---------------------------------------------------------------------------------------------+
                                              │
                                              ▼
+---------------------------------------------------------------------------------------------+
|                         5. COOLING-STRATEGY RECOMMENDATION ENGINE                           |
|  - Context-Aware Multi-Criteria Decision Matrix evaluating 5 Core Interventions             |
|  - Primary & Secondary Prescription Ranking + Rationale Generation                          |
+---------------------------------------------------------------------------------------------+
                                              │
                                              ▼
+---------------------------------------------------------------------------------------------+
|                             6. IMPACT ESTIMATION & WHAT-IF SIM                              |
|  - Non-Linear Diminishing Returns: ΔT_effective = ΔT_max * (1 - exp(-k * ΣΔ_raw))           |
|  - HVAC Grid Energy Savings Metric: ~2.2% reduction per 1°C ambient drop                    |
|  - Multi-Scenario Budget & ROI Matrix Comparison                                            |
+---------------------------------------------------------------------------------------------+
                                              │
                                              ▼
+---------------------------------------------------------------------------------------------+
|                            7. INTERACTIVE GIS PLANNER DASHBOARD                             |
|  - Spatial Heatmap, What-If Slider Simulator, Live Serial Monitor & Municipal Plan Export   |
+---------------------------------------------------------------------------------------------+
```

---

## 🔌 Hardware Setup & Circuit Schematic

| ESP32 DevKit V1 Pin | Target Component | Component Pin | Function / Logic |
| :--- | :--- | :--- | :--- |
| **3.3V** | DHT22 / SSD1306 | VCC | 3.3V DC Regulated Power |
| **GND** | All Components | GND | Common Ground Plane |
| **GPIO 4** | DHT22 Sensor | DATA / SDA | Digital 1-Wire Telemetry (with 10k pull-up) |
| **GPIO 21** | SSD1306 OLED | SDA | I2C Data Line (0x3C Address) |
| **GPIO 22** | SSD1306 OLED | SCL | I2C Clock Line |
| **GPIO 18** | Green LED | Anode (+) via 220Ω | Safe Micro-Climate State (`LOW`) |
| **GPIO 19** | Yellow LED | Anode (+) via 220Ω | Moderate Heat State (`MODERATE`) |
| **GPIO 23** | Red LED | Anode (+) via 220Ω | High Thermal Stress Warning (`HIGH`) |
| **GPIO 5** | Active Buzzer | Positive Terminal | Extreme Heat Acoustic Alarm (>38°C) |

---

## 🌿 5-Cooling Intervention Catalog & Quantification

$$\Delta T_{\text{effective}} = \Delta T_{\text{max}} \times \left(1 - e^{-k \cdot \sum \Delta_{\text{raw}}}\right)$$

1. **High-Albedo Cool Roofs**: $\Delta T = -1.5^\circ\text{C to } -3.2^\circ\text{C}$ (in high NDBI & low-albedo built-up zones; Cost: ₹350/m²; SRI > 80).
2. **Intensive Urban Forestry & Tree Canopy**: $\Delta T = -2.2^\circ\text{C to } -4.8^\circ\text{C}$ (via shading & latent heat evapotranspiration; Cost: ₹600/m²).
3. **Extensive Green Roofs & Bio-Solar Living Walls**: $\Delta T = -1.2^\circ\text{C to } -2.6^\circ\text{C}$ (insulates building envelope and reduces sensible heat flux; Cost: ₹1,200/m²).
4. **Permeable Cool Pavements & Porous Asphalt**: $\Delta T = -1.0^\circ\text{C to } -2.4^\circ\text{C}$ (replaces heat-storing asphalt with moisture-retentive pavers; Cost: ₹850/m²).
5. **Micro-Misting Canopies & Water Retention Plazas**: $\Delta T = -1.5^\circ\text{C to } -3.0^\circ\text{C}$ (rapid sensible heat absorption in high-footfall concourses; Cost: ₹950/m²).

---

## 🧪 Virtual C Simulation & CLI Guide

Compile and run the native C simulation tool:

```bash
# Compile Virtual UHI CLI in Pure C99
gcc -std=c99 -Wall -Wextra simulation/virtual_uhi.c esp32_firmware/src/uhi_engine.c esp32_firmware/src/uhi_random_forest.c esp32_firmware/src/uhi_scenario_sim.c esp32_firmware/src/uhi_preprocessor.c -I esp32_firmware/src -lm -o virtual_uhi

# 1. Stream 10 live virtual IoT telemetry packets
./virtual_uhi 10

# 2. Evaluate all micro-zones from dataset CSV
./virtual_uhi --eval-csv dataset/processed_microzone_data.csv 20

# 3. Run Scenario Comparison Simulation in terminal
./virtual_uhi --scenario

# 4. Export Municipal Action Plan to JSON
./virtual_uhi --export-plan municipal_action_plan.json
```

---

## 📊 Interactive GIS Dashboard

Open `dashboard/index.html` directly in any modern browser:
- **Tab 1: GIS Heatmap & Micro-Zones**: Click any zone to view parcel diagnostics and prescription.
- **Tab 2: What-If Scenario Simulator**: Adjust sliders to simulate canopy expansion, albedo coating, green roofs, and permeable pavements, observing real-time $\Delta T$, budget, and carbon offset.
- **Tab 3: Live IoT Edge Stream**: Choose Live Open-Meteo Weather API stream, Virtual C stream, or ESP32 Web Serial.
- **Tab 4: 5-Cooling Intervention Catalog**: Browse detailed technical specifications.
- **Export Button**: Download complete Municipal Action Plan JSON with a single click.

---

## 🧪 Testing & Validation Suite

Automated C unit test suite verifying feature preprocessing, Random Forest classification, 5 cooling strategy rules, non-linear impact formulas, and scenario simulations:

```bash
# Compile and run automated C test suite
gcc -std=c99 -Wall -Wextra tests/test_uhi_engine.c esp32_firmware/src/uhi_engine.c esp32_firmware/src/uhi_random_forest.c esp32_firmware/src/uhi_scenario_sim.c esp32_firmware/src/uhi_preprocessor.c -I esp32_firmware/src -lm -o test_runner
./test_runner
```
