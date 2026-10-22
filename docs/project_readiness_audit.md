# Project Readiness Audit

**Audit date:** 2026-10-23
**Scope:** Repository claims, reproducibility, model evaluation, dashboard/API, C engine, ESP32 firmware, tests, and demo materials.

## Overall assessment

The repository is a substantial engineering prototype with a browser dashboard, native C assessment and simulation code, ESP32 firmware, a processed dataset in the local working directory, and automated tests. It is **not yet ready to be presented as a validated predictive system or as guaranteed first-prize work**. The main blockers are a reproducible ML training/evaluation pipeline, a real dashboard-to-C execution path, uncertainty evidence, persistent telemetry, and reproducible CI/demo setup.

Competition results cannot be guaranteed. A credible submission should make claims supported by repeatable experiments and clearly distinguish measured hardware values, reference dataset values, simulated inputs, and model estimates.

## Requested upgrade checklist

| # | Requested capability | Audit result | Evidence and remaining work |
|---|---|---|---|
| 1 | Reproducibly train and evaluate a real Random Forest | **Not present in tracked source** | The processed CSV in this working directory has 2,500 rows, 10 zone IDs, and labels Low 711 / Moderate 271 / High 1,518. There is no tracked training script or reproducible metric report. The `.pkl` model/scaler and confusion-matrix screenshots are ignored by Git. Add provenance, deterministic preprocessing, a leakage-aware split, per-class precision/recall/F1, confusion matrix, and saved run metadata before making performance claims. Because observations repeat across just 10 zones, a random row split could leak zone-specific patterns; hold out whole zones for a stronger generalization test and separately report a stratified random split only if useful. |
| 2 | Dashboard calls the native C engine | **Not present** | The dashboard currently computes recommendations/scenarios in JavaScript, and `/api/v1/recommendations` and `/api/v1/scenarios` compute separately in Python. The native C CLI is a separate process and is not called by these routes. Consolidate around one C engine interface (for example a small validated JSON CLI or shared library) and compare outputs with regression tests. Avoid maintaining three divergent formula copies. |
| 3 | Resilience and deployment-priority ranking | **Partially present; heuristic corrected in this audit** | The dashboard ranks its eight hard-coded map profiles, not the 10 dataset zones. The prior score saturated at 100 and ordered highest resilience first. It now uses normalized weighted inputs, ranks highest priority first, and labels scores as heuristics. Weights and thresholds remain design assumptions, not validated heat-risk or resilience indices; document and evaluate them before using them for real decisions. |
| 4 | Sensitivity and uncertainty analysis | **Not present** | Scenario output is a point estimate. No range propagation, parameter sensitivity calculation, tornado chart, or uncertainty interval is implemented. Catalog cooling ranges currently describe catalog assumptions and should not be presented as calibrated prediction intervals. |
| 5 | Persistent telemetry and alerts | **Not present** | Dashboard telemetry is held in browser memory for the current session. No SQLite/CSV time-series store, restart-safe history, retention policy, or persistent alert log was found. Add source and timestamp handling, quality checks, storage, and a history endpoint/chart before describing monitoring as durable. |
| 6 | Cost-benefit/payback analysis | **Partially present** | Scenario and intervention views display indicative cost and estimated energy/carbon values, but no payback period or cost per degree is calculated. The assumptions are not all exposed alongside the KPI values. Include lifecycle/maintenance costs and local tariff/emissions assumptions before presenting ROI as a financial forecast. |
| 7 | PDF municipal report | **Not present** | The dashboard has JSON action-plan export; there is no PDF generation path. A PDF should include input sources, assumptions, timestamp, limitations, and uncertainty labels. |
| 8 | Broader tests and CI | **Partially present** | Seven native C tests and four REST API integration tests pass in this audit. No root `.github/workflows` CI configuration is tracked. The firmware build did not reach a final successful PlatformIO status in this environment: the direct command rejected the workspace path containing spaces; a no-space junction retry completed an ESP-IDF build step but failed in Git metadata lookup. Hardware/Wokwi behavior was not validated by these tests. Add CI for C/API/dashboard checks and a separate firmware build job with a supported path/toolchain. |
| 9 | One-command demo, sample data, slides/screenshots | **Partially present** | README and presentation content exist, but there is no root start script, and the expected `dataset/sample_microzones.csv` exception in `.gitignore` has no matching file. The full CSVs under `dataset/` and pickle files under `ml_engine/` are ignored and absent from tracked files, so a fresh clone cannot reproduce the advertised dataset-driven API/model work. Existing screenshots are static artifacts; no updated capture workflow or current browser screenshots were produced in this audit. |

## Validation performed

- Dataset inspection: 2,500 processed rows, 10 zone IDs, three labels (counts shown above). Dataset files are present locally but ignored by Git.
- Native C suite: **7 / 7 passed**.
- Dashboard REST API suite: **4 / 4 passed**.
- JavaScript syntax (`node --check dashboard/app.js`): passed.
- Python syntax (`py_compile` for the API server and tests): passed.
- ESP32 PlatformIO build: **not verified as successful**; see item 8.
- Physical ESP32 and Wokwi runtime: not tested in this audit.
- Cooling impact, cost, energy savings, and resilience score: not validated against field measurements.

## Minimum high-value work before judging

1. Make a clean clone reproduce the data/model results: provide an appropriately licensed, documented sample dataset or retrieval instructions, dependency versions, training command, deterministic split, metrics, and artifact provenance.
2. Connect browser/API behavior to the same C engine and add cross-path tests demonstrating matching classifications and recommendations.
3. Validate the classifier with zone-level holdout and report class-wise results; do not use the current screenshots as proof without traceable generation code and test data.
4. Label telemetry by source (sensor/weather/virtual), persist readings if history is claimed, and demonstrate a physical or Wokwi path separately.
5. Add uncertainty/sensitivity analysis and show assumptions next to cost-benefit outputs; avoid treating illustrative ranges as confidence intervals.
6. Add CI and a repeatable one-command demo, then capture fresh screenshots and update the slide deck from tested outputs.
7. Perform a documented hardware rehearsal: wiring, sensor fault behavior, OLED/LED/buzzer transitions, serial JSON, and browser integration.

## Presentation guidance

Describe this as a **decision-support prototype** until the data sources, model generalization, hardware behavior, and intervention impacts have been independently validated. A clear account of limitations is stronger in a jury review than unsupported “AI accuracy,” “real-time city monitoring,” or guaranteed cooling claims.
