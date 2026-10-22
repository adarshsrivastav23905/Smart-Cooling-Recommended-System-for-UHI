# System Architecture & Technical Specifications

## 1. System Overview
The **Smart Cooling Strategy Recommendation System for Urban Heat Island Reduction** is a decision-support prototype for assessing configured urban micro-zone profiles, demonstrating ESP32 temperature and humidity telemetry, and comparing modeled cooling interventions. The diagram below describes the intended data flow; satellite products and GIS layers are external inputs, not acquired by the current firmware.

```
+----------------------------------------------------------------------------------------------------+
|                                 STAGE 1: DATA ACQUISITION LAYER                                    |
|                                                                                                    |
|   +---------------------------------------+         +------------------------------------------+   |
|   |         IoT Ground Telemetry          |         |    Satellite Earth Observation (EO)      |   |
|   |  - ESP32 DevKit V1 (Xtensa Dual-Core) |         |  - Landsat-8 TIR / Sentinel-2 Multi-Band |   |
|   |  - DHT22 Temp & Relative Humidity     |         |  - LST (Land Surface Temperature in °C)  |   |
|   |  - SSD1306 OLED & Acoustic Actuators  |         |  - NDVI (Normalized Veg Index: NIR-Red)  |   |
|   |  - JSON Telemetry Stream (Serial/2s)  |         |  - NDBI (Built-Up Index: SWIR-NIR)       |   |
|   +---------------------------------------+         +------------------------------------------+   |
+----------------------------------------------------------------------------------------------------+
                                                  │
                                                  ▼
+----------------------------------------------------------------------------------------------------+
|                          STAGE 2: DATA PREPROCESSING & FEATURE PIPELINE                            |
|                                                                                                    |
|   - Physical Boundary Clipping (Temp: 15-50°C, Humidity: 5-100%, NDVI: -1 to +1)                   |
|   - Missing Value Median Imputation in Pure C99                                                    |
|   - Urban Thermal Stress Index (UTSI) = f(LST, Ambient Temp, Humidity, NDVI, NDBI, Albedo)        |
+----------------------------------------------------------------------------------------------------+
                                                  │
                                                  ▼
+----------------------------------------------------------------------------------------------------+
|                            STAGE 3 & 4: RANDOM FOREST CLASSIFIER IN C                              |
|                                                                                                    |
|   +--------------------------------------------------------------------------------------------+   |
|   |                  C Classifier (25 Hand-Coded Decision Trees)                               |   |
|   |   - Inputs: 10 Environmental & Spectral Parameters                                         |   |
|   |   - Output: Heat Severity Classification [ LOW | MODERATE | HIGH ]                         |   |
|   |   - Vote-based class confidence                                                             |   |
|   +--------------------------------------------------------------------------------------------+   |
+----------------------------------------------------------------------------------------------------+
                                                  │
                                                  ▼
+----------------------------------------------------------------------------------------------------+
|                 STAGE 5 & 6: 5-COOLING STRATEGY & NON-LINEAR IMPACT ESTIMATION                     |
|                                                                                                    |
|   +-----------------------------------------+     +--------------------------------------------+   |
|   |       5 Core Intervention Engine        |     |       Assumption-based Impact Estimator    |   |
|   |  1. High-Albedo Cool Roofs (SRI > 80)   |     |  - ΔT_effective = ΔT_max * (1 - exp(-k*Δ)) |   |
|   |  2. Intensive Urban Forestry & Canopy   |     |  - Non-linear Synergy & Diminishing Return |   |
|   |  3. Extensive Green Roofs & Bio-Walls   |     |  - HVAC Grid Energy Savings (%): ~2.2%/°C  |   |
|   |  4. Permeable Interlocking Cool Pavers  |     |  - Capital Cost & Carbon Offset Modeling   |   |
|   |  5. Micro-Misting Water Plazas          |     |                                            |   |
|   +-----------------------------------------+     +--------------------------------------------+   |
+----------------------------------------------------------------------------------------------------+
                                                  │
                                                  ▼
+----------------------------------------------------------------------------------------------------+
|                          STAGE 7: GIS DASHBOARD & WHAT-IF PLANNER                                  |
|                                                                                                    |
|   - Interactive Leaflet GIS Map with Thermal Hotspot Color-Coding & Micro-Zone Selection           |
|   - What-If Intervention Scenario Simulator (Canopy +%, Albedo +, Green Roofs +%, Permeable +%)   |
|   - Multi-Scenario Budget & ROI Comparison Matrix                                                  |
|   - Automated Municipal Action Plan Export (JSON)                                                  |
+----------------------------------------------------------------------------------------------------+
```

## 2. Input/Output Parameter Specification

The ranges below are software input bounds, not guaranteed sensor ranges or proof that a value is currently measured. Only air temperature and relative humidity come from the current DHT22 firmware; other feature values require separately documented sources or must be labeled as demonstration inputs.

| Parameter | Type | Domain / Range | Current source/status | Physical Meaning |
| :--- | :--- | :--- | :--- | :--- |
| `ambient_temp` | Float | 15.0°C to 50.0°C | DHT22 in firmware; profile/API in dashboard | Ambient dry-bulb air temperature |
| `humidity` | Float | 5.0% to 100.0% | DHT22 in firmware; profile/API in dashboard | Relative humidity percentage |
| `lst` | Float | 15.0°C to 60.0°C | Not measured by device; fixed demo input in firmware | Land-surface temperature |
| `ndvi` | Float | -1.00 to +1.00 | Fixed demo input in firmware; external source required for real data | Vegetation index |
| `ndbi` | Float | -1.00 to +1.00 | Fixed demo input in firmware; external source required for real data | Built-up index |
| `albedo` | Float | 0.05 to 0.85 | Fixed demo input in firmware; external source required for real data | Surface shortwave solar reflectance |
| `building_density`| Float | 0.0% to 100.0% | Fixed demo input in firmware; GIS source required for real data | Built footprint ratio |
| `tree_canopy_cover`| Float | 0.0% to 100.0% | Fixed demo input in firmware; GIS source required for real data | Tree canopy coverage |
| `traffic_density` | Float | 0 to 5000 veh/hr | Fixed demo input in firmware; traffic data required for real data | Traffic activity |
| `population_density`| Float | 0 to 50000 /km² | Fixed demo input in firmware; demographic source required for real data | Population density |

The C tree rules and feature-importance values are encoded in source. They are not retrained at runtime from a documented training dataset. Cooling, cost, energy, and emissions outputs are model estimates and require local validation before operational use.
