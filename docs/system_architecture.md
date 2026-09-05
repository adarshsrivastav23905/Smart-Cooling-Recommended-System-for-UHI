# System Architecture & Technical Specifications

## 1. System Overview
The **Smart Cooling Strategy Recommendation System for Urban Heat Island Reduction** is a multi-tier IoT, Remote Sensing, Machine Learning, and GIS decision-support platform designed to monitor micro-climatic thermal anomalies across urban micro-zones and recommend localized, actionable cooling interventions with quantified thermal mitigation estimates.

```
+----------------------------------------------------------------------------------------------------+
|                                 STAGE 1: DATA ACQUISITION LAYER                                    |
|                                                                                                    |
|   +---------------------------------------+         +------------------------------------------+   |
|   |         IoT Ground Telemetry          |         |    Satellite Earth Observation (EO)      |   |
|   |  - ESP32 DevKit V1 (Xtensa Dual-Core) |         |  - Landsat-8 TIR / Sentinel-2 Multi-Band |   |
|   |  - DHT22 Temp & Relative Humidity     |         |  - LST (Land Surface Temperature in °C)  |   |
|   |  - SSD1306 OLED & Acoustic Actuators  |         |  - NDVI (Normalized Veg Index: NIR-Red)  |   |
|   |  - JSON Telemetry Stream (Serial/1Hz) |         |  - NDBI (Built-Up Index: SWIR-NIR)       |   |
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
|   |               Embedded Random Forest Classifier Ensemble (25 Diverse Trees)                |   |
|   |   - Inputs: 10 Environmental & Spectral Parameters                                         |   |
|   |   - Output: Heat Severity Classification [ LOW | MODERATE | HIGH ]                         |   |
|   |   - Softmax Probability Distribution & Confidence Scoring                                  |   |
|   +--------------------------------------------------------------------------------------------+   |
+----------------------------------------------------------------------------------------------------+
                                                  │
                                                  ▼
+----------------------------------------------------------------------------------------------------+
|                 STAGE 5 & 6: 5-COOLING STRATEGY & NON-LINEAR IMPACT ESTIMATION                     |
|                                                                                                    |
|   +-----------------------------------------+     +--------------------------------------------+   |
|   |       5 Core Intervention Engine        |     |       Predictive Impact Estimator          |   |
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

| Parameter | Type | Domain / Range | Source | Physical Meaning |
| :--- | :--- | :--- | :--- | :--- |
| `ambient_temp` | Float | 15.0°C to 50.0°C | ESP32 DHT22 / Sensor | Ambient dry-bulb air temperature |
| `humidity` | Float | 5.0% to 100.0% | ESP32 DHT22 / Sensor | Relative humidity percentage |
| `lst` | Float | 15.0°C to 60.0°C | Satellite TIR (Thermal Band) | Radiometric surface temperature |
| `ndvi` | Float | -1.00 to +1.00 | Sentinel-2 / Landsat-8 (NIR, Red)| Vegetation density & canopy health |
| `ndbi` | Float | -1.00 to +1.00 | Sentinel-2 / Landsat-8 (SWIR, NIR)| Built-up & impermeable surface index |
| `albedo` | Float | 0.05 to 0.85 | Remote Sensing Optical Bands | Surface shortwave solar reflectance |
| `building_density`| Float | 0.0% to 100.0% | GIS Cadastral Data | Built footprint ratio per hectare |
| `tree_canopy_cover`| Float | 0.0% to 100.0% | LiDAR / GIS Forestry Layer | High-foliage tree canopy percentage |
| `traffic_density` | Float | 0 to 5000 veh/hr | Smart City Traffic Cameras | Anthropogenic heat & exhaust source |
| `population_density`| Float | 0 to 50000 /km² | Urban Demographics | Human exposure vulnerability factor |
