#ifndef UHI_SCENARIO_SIM_H
#define UHI_SCENARIO_SIM_H

#include "uhi_engine.h"
#include "uhi_random_forest.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SCENARIO_NAME_LEN 64
#define MAX_COMPARISON_SCENARIOS 4

/* -------------------------------------------------------------------------- */
/* Scenario Intervention Parameter Adjustments                                */
/* -------------------------------------------------------------------------- */
typedef struct {
    char scenario_name[MAX_SCENARIO_NAME_LEN];
    float delta_canopy_pct;          /* Tree canopy expansion (+0 to +50%)      */
    float delta_albedo;              /* Cool roof albedo increase (+0.0 to +0.5)*/
    float green_roof_coverage_pct;   /* Green roof deployment area (+0 to +60%) */
    float permeable_pavement_pct;    /* Permeable paver conversion (+0 to +80%) */
    float misting_coverage_pct;      /* Active micro-misting plazas (0 to 100%) */
    float target_zone_area_sqm;      /* Zone geographical area in m²            */
} uhi_scenario_params_t;

/* -------------------------------------------------------------------------- */
/* Simulated Scenario Outcome                                                 */
/* -------------------------------------------------------------------------- */
typedef struct {
    char scenario_name[MAX_SCENARIO_NAME_LEN];
    uhi_features_t baseline_features;
    uhi_features_t mitigated_features;
    float baseline_stress_index;
    float mitigated_stress_index;
    rf_prediction_t baseline_rf_prediction;
    rf_prediction_t mitigated_rf_prediction;
    float raw_cooling_potential_c;
    float effective_temp_drop_c;     /* Non-linear ΔT reduction (°C)            */
    float hvac_energy_savings_pct;   /* Municipal/Building HVAC savings (%)     */
    float total_investment_inr;      /* Total project cost in INR               */
    float annual_energy_saved_kwh;   /* Estimated annual grid energy reduction  */
    float co2_offset_metric_tons;    /* Annual carbon offset in tons            */
    uhi_recommendation_t primary_intervention;
} uhi_scenario_result_t;

/* -------------------------------------------------------------------------- */
/* Multi-Scenario Comparative Analysis                                        */
/* -------------------------------------------------------------------------- */
typedef struct {
    uint32_t scenario_count;
    uhi_scenario_result_t results[MAX_COMPARISON_SCENARIOS];
    uint32_t best_cooling_scenario_idx;
    uint32_t best_roi_scenario_idx;
} uhi_scenario_comparison_t;

/**
 * @brief Simulates a single urban cooling intervention scenario against a baseline micro-zone.
 */
uhi_scenario_result_t uhi_simulate_scenario(const uhi_features_t *baseline,
                                            const uhi_scenario_params_t *params);

/**
 * @brief Evaluates and compares multiple planning scenarios for municipal budget allocation.
 */
uhi_scenario_comparison_t uhi_compare_scenarios(const uhi_features_t *baseline,
                                                const uhi_scenario_params_t scenarios[],
                                                uint32_t count);

#ifdef __cplusplus
}
#endif

#endif /* UHI_SCENARIO_SIM_H */
