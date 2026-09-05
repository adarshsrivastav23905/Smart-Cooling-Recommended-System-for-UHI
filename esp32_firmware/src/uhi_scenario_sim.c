#include "uhi_scenario_sim.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static float clamp_val(float val, float min, float max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

uhi_scenario_result_t uhi_simulate_scenario(const uhi_features_t *baseline,
                                            const uhi_scenario_params_t *params) {
    uhi_scenario_result_t res = {0};
    if (!baseline || !params) return res;

    strncpy(res.scenario_name, params->scenario_name, sizeof(res.scenario_name) - 1);
    res.baseline_features = *baseline;
    res.mitigated_features = *baseline;

    /* Apply physical micro-climate parameter transformations */
    /* 1. Canopy Expansion: increases NDVI, decreases LST, increases shade */
    float canopy_inc = clamp_val(params->delta_canopy_pct, 0.0f, 60.0f);
    res.mitigated_features.tree_canopy_cover_pct = clamp_val(baseline->tree_canopy_cover_pct + canopy_inc, 0.0f, 100.0f);
    res.mitigated_features.ndvi = clamp_val(baseline->ndvi + (canopy_inc * 0.006f), -1.0f, 0.95f);

    /* 2. Albedo Modification: increases solar reflectance, reduces surface sensible heat */
    float albedo_inc = clamp_val(params->delta_albedo, 0.0f, 0.6f);
    res.mitigated_features.albedo = clamp_val(baseline->albedo + albedo_inc, 0.05f, 0.85f);

    /* 3. Green Roofs: adds vegetation in high-density areas, reduces NDBI */
    float green_roof = clamp_val(params->green_roof_coverage_pct, 0.0f, 80.0f);
    res.mitigated_features.ndvi = clamp_val(res.mitigated_features.ndvi + (green_roof * 0.003f), -1.0f, 0.95f);
    res.mitigated_features.ndbi = clamp_val(res.mitigated_features.ndbi - (green_roof * 0.004f), -1.0f, 1.0f);

    /* 4. Permeable Cool Pavement: improves moisture retention, reduces heat storage */
    float perm_pave = clamp_val(params->permeable_pavement_pct, 0.0f, 90.0f);
    res.mitigated_features.ndbi = clamp_val(res.mitigated_features.ndbi - (perm_pave * 0.003f), -1.0f, 1.0f);

    /* 5. Micro-Misting / Water Retention Plazas */
    float misting = clamp_val(params->misting_coverage_pct, 0.0f, 100.0f);
    res.mitigated_features.humidity_pct = clamp_val(baseline->humidity_pct + (misting * 0.08f), 10.0f, 90.0f);

    /* Calculate component raw temperature reductions */
    float canopy_drop = canopy_inc * 0.075f;           /* Up to ~3.5°C */
    float albedo_drop = albedo_inc * 6.5f;             /* Up to ~2.8°C */
    float green_roof_drop = green_roof * 0.035f;       /* Up to ~2.0°C */
    float perm_pave_drop = perm_pave * 0.025f;         /* Up to ~1.8°C */
    float misting_drop = misting * 0.025f;             /* Up to ~2.5°C */

    res.raw_cooling_potential_c = canopy_drop + albedo_drop + green_roof_drop + perm_pave_drop + misting_drop;

    /* Compute non-linear diminishing returns: ΔT_effective = 6.0 * (1 - exp(-0.22 * raw)) */
    res.effective_temp_drop_c = uhi_calculate_effective_drop(res.raw_cooling_potential_c, 6.0f, 0.22f);

    /* Apply predicted cooling to mitigated LST and Ambient Temperature */
    res.mitigated_features.lst_c = clamp_val(baseline->lst_c - (res.effective_temp_drop_c * 1.4f), 18.0f, 60.0f);
    res.mitigated_features.ambient_temp_c = clamp_val(baseline->ambient_temp_c - res.effective_temp_drop_c, 18.0f, 50.0f);

    /* Compute Thermal Stress for Baseline vs Mitigated */
    res.baseline_stress_index = uhi_compute_thermal_stress(&res.baseline_features);
    res.mitigated_stress_index = uhi_compute_thermal_stress(&res.mitigated_features);

    /* Evaluate Random Forest Predictions */
    res.baseline_rf_prediction = uhi_rf_predict(&res.baseline_features);
    res.mitigated_rf_prediction = uhi_rf_predict(&res.mitigated_features);

    /* HVAC Energy Savings (approx 2.2% reduction in cooling load per 1°C drop) */
    res.hvac_energy_savings_pct = clamp_val(res.effective_temp_drop_c * 2.2f, 0.0f, 25.0f);

    /* Financial and Ecological Modeling based on target zone area */
    float area = params->target_zone_area_sqm > 0 ? params->target_zone_area_sqm : 50000.0f; /* default 5 hectares */

    float canopy_cost = (canopy_inc / 100.0f) * area * 600.0f;          /* 600 INR/m² */
    float albedo_cost = (albedo_inc > 0.05f ? 1.0f : 0.0f) * (area * (baseline->building_density_pct / 100.0f) * 0.6f) * 350.0f; /* 350 INR/m² */
    float green_roof_cost = (green_roof / 100.0f) * (area * (baseline->building_density_pct / 100.0f)) * 1200.0f; /* 1200 INR/m² */
    float perm_pave_cost = (perm_pave / 100.0f) * (area * 0.35f) * 850.0f; /* 850 INR/m² */
    float misting_cost = (misting / 100.0f) * (area * 0.10f) * 950.0f;     /* 950 INR/m² */

    res.total_investment_inr = canopy_cost + albedo_cost + green_roof_cost + perm_pave_cost + misting_cost;

    /* Estimated Annual Energy and Carbon Metrics */
    /* ~15 kWh/m²/year commercial cooling demand baseline in tropical urban zones */
    float annual_cooling_demand_kwh = area * (baseline->building_density_pct / 100.0f) * 45.0f;
    res.annual_energy_saved_kwh = annual_cooling_demand_kwh * (res.hvac_energy_savings_pct / 100.0f);
    /* 0.82 kg CO2 per kWh grid emission factor */
    res.co2_offset_metric_tons = (res.annual_energy_saved_kwh * 0.82f) / 1000.0f;

    /* Determine Primary Intervention by maximum contribution */
    if (canopy_drop >= albedo_drop && canopy_drop >= green_roof_drop && canopy_drop >= perm_pave_drop && canopy_drop >= misting_drop) {
        res.primary_intervention = UHI_RECOMMENDATION_URBAN_FORESTRY;
    } else if (albedo_drop >= green_roof_drop && albedo_drop >= perm_pave_drop && albedo_drop >= misting_drop) {
        res.primary_intervention = UHI_RECOMMENDATION_COOL_ROOF;
    } else if (green_roof_drop >= perm_pave_drop && green_roof_drop >= misting_drop) {
        res.primary_intervention = UHI_RECOMMENDATION_GREEN_ROOFS;
    } else if (perm_pave_drop >= misting_drop) {
        res.primary_intervention = UHI_RECOMMENDATION_PERMEABLE_PAVEMENT;
    } else if (misting_drop > 0.0f) {
        res.primary_intervention = UHI_RECOMMENDATION_MISTING_PLAZA;
    } else {
        res.primary_intervention = UHI_RECOMMENDATION_NONE;
    }

    return res;
}

uhi_scenario_comparison_t uhi_compare_scenarios(const uhi_features_t *baseline,
                                                const uhi_scenario_params_t scenarios[],
                                                uint32_t count) {
    uhi_scenario_comparison_t comp = {0};
    if (!baseline || !scenarios || count == 0) return comp;

    comp.scenario_count = (count > MAX_COMPARISON_SCENARIOS) ? MAX_COMPARISON_SCENARIOS : count;

    float max_drop = -1.0f;
    float best_roi = -1.0f;

    for (uint32_t i = 0; i < comp.scenario_count; i++) {
        comp.results[i] = uhi_simulate_scenario(baseline, &scenarios[i]);

        if (comp.results[i].effective_temp_drop_c > max_drop) {
            max_drop = comp.results[i].effective_temp_drop_c;
            comp.best_cooling_scenario_idx = i;
        }

        /* ROI Metric: °C reduction per Lakh INR invested */
        float cost_lakhs = comp.results[i].total_investment_inr / 100000.0f;
        float roi = (cost_lakhs > 0.1f) ? (comp.results[i].effective_temp_drop_c / cost_lakhs) : 0.0f;
        if (roi > best_roi) {
            best_roi = roi;
            comp.best_roi_scenario_idx = i;
        }
    }

    return comp;
}
