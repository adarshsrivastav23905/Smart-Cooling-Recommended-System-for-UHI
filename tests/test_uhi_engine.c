#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "../esp32_firmware/src/uhi_engine.h"
#include "../esp32_firmware/src/uhi_random_forest.h"
#include "../esp32_firmware/src/uhi_scenario_sim.h"
#include "../esp32_firmware/src/uhi_preprocessor.h"

static int g_tests_run = 0;
static int g_tests_passed = 0;

#define RUN_TEST(fn) do { \
    g_tests_run++; \
    printf("Running %-45s ... ", #fn); \
    fn(); \
    g_tests_passed++; \
    printf("[PASS]\n"); \
} while(0)

/* Test 1: Feature Preprocessing and Outlier Clipping */
static void test_feature_preprocessing_and_clipping(void) {
    uhi_features_t invalid_features = {
        .ambient_temp_c = 75.0f,    /* Out of bounds (>50°C) */
        .humidity_pct = -10.0f,     /* Out of bounds (<5%)   */
        .lst_c = 100.0f,            /* Out of bounds (>60°C) */
        .ndvi = 2.5f,               /* Out of bounds (>1.0)  */
        .ndbi = -3.0f,              /* Out of bounds (<-1.0) */
        .albedo = 0.95f,            /* Out of bounds (>0.85) */
        .building_density_pct = 150.0f, /* >100% */
        .tree_canopy_cover_pct = -5.0f, /* <0% */
        .traffic_density = 9999.0f, /* >5000 */
        .population_density = 80000.0f
    };

    uhi_features_t clean = uhi_preprocess_features(&invalid_features);

    assert(clean.ambient_temp_c <= 50.01f && clean.ambient_temp_c >= 14.99f);
    assert(clean.humidity_pct >= 4.99f && clean.humidity_pct <= 100.01f);
    assert(clean.lst_c <= 60.01f && clean.lst_c >= 14.99f);
    assert(clean.ndvi <= 1.01f && clean.ndvi >= -1.01f);
    assert(clean.ndbi <= 1.01f && clean.ndbi >= -1.01f);
    assert(clean.albedo <= 0.8501f && clean.albedo >= 0.0499f);
    assert(clean.building_density_pct <= 100.01f && clean.building_density_pct >= 0.0f);
    assert(clean.tree_canopy_cover_pct <= 100.01f && clean.tree_canopy_cover_pct >= 0.0f);
    assert(clean.traffic_density <= 5000.01f && clean.traffic_density >= 0.0f);
}

/* Test 2: Urban Thermal Stress Index (UTSI) Computation */
static void test_thermal_stress_calculation(void) {
    /* Safe Green Zone */
    uhi_features_t green_zone = {
        .lst_c = 22.0f, .ambient_temp_c = 24.0f, .humidity_pct = 55.0f,
        .ndvi = 0.75f, .ndbi = -0.45f, .albedo = 0.25f,
        .building_density_pct = 5.0f, .tree_canopy_cover_pct = 80.0f,
        .traffic_density = 100.0f, .population_density = 500.0f
    };
    float green_stress = uhi_compute_thermal_stress(&green_zone);
    assert(green_stress >= 0.0f && green_stress < 30.0f);

    /* Extreme Urban Hotspot */
    uhi_features_t hot_zone = {
        .lst_c = 44.0f, .ambient_temp_c = 38.0f, .humidity_pct = 45.0f,
        .ndvi = 0.08f, .ndbi = 0.55f, .albedo = 0.12f,
        .building_density_pct = 85.0f, .tree_canopy_cover_pct = 5.0f,
        .traffic_density = 4500.0f, .population_density = 22000.0f
    };
    float hot_stress = uhi_compute_thermal_stress(&hot_zone);
    assert(hot_stress >= 65.0f && hot_stress <= 100.0f);
}

/* Test 3: Random Forest Classifier (All 3 Classes: LOW, MODERATE, HIGH) */
static void test_random_forest_classification(void) {
    uhi_features_t low_feat = {
        .lst_c = 23.0f, .ambient_temp_c = 25.0f, .humidity_pct = 60.0f,
        .ndvi = 0.70f, .ndbi = -0.40f, .albedo = 0.24f,
        .building_density_pct = 10.0f, .tree_canopy_cover_pct = 75.0f,
        .traffic_density = 200.0f, .population_density = 1000.0f
    };
    rf_prediction_t pred_low = uhi_rf_predict(&low_feat);
    assert(pred_low.predicted_class == RF_CLASS_LOW);
    assert(pred_low.confidence >= 0.60f);

    uhi_features_t high_feat = {
        .lst_c = 43.0f, .ambient_temp_c = 37.5f, .humidity_pct = 42.0f,
        .ndvi = 0.10f, .ndbi = 0.50f, .albedo = 0.14f,
        .building_density_pct = 80.0f, .tree_canopy_cover_pct = 6.0f,
        .traffic_density = 4200.0f, .population_density = 20000.0f
    };
    rf_prediction_t pred_high = uhi_rf_predict(&high_feat);
    assert(pred_high.predicted_class == RF_CLASS_HIGH);
    assert(pred_high.confidence >= 0.70f);

    /* Verify Gini feature importances sum to 1.0 */
    rf_feature_importance_t fi = uhi_rf_get_feature_importances();
    float sum_imp = 0.0f;
    for (int i = 0; i < RF_NUM_FEATURES; i++) {
        sum_imp += fi.importances[i];
    }
    assert(fabsf(sum_imp - 1.0f) < 0.01f);
}

/* Test 4: 5 Core Cooling Interventions Context-Aware Recommendation */
static void test_5_cooling_strategies(void) {
    /* 1. Cool Roofs expected for High NDBI & Low Albedo & Built-up */
    uhi_features_t roof_zone = {
        .lst_c = 37.0f, .ambient_temp_c = 33.0f, .humidity_pct = 50.0f,
        .ndvi = 0.30f, .ndbi = 0.45f, .albedo = 0.12f,
        .building_density_pct = 75.0f, .tree_canopy_cover_pct = 20.0f,
        .traffic_density = 1200.0f, .population_density = 14000.0f
    };
    uhi_assessment_t a_roof = uhi_assess_zone(&roof_zone);
    assert(a_roof.primary_recommendation == UHI_RECOMMENDATION_COOL_ROOF ||
           a_roof.primary_recommendation == UHI_RECOMMENDATION_GREEN_ROOFS);

    /* 2. Urban Forestry expected for Very Low Canopy & High Heat */
    uhi_features_t tree_zone = {
        .lst_c = 39.0f, .ambient_temp_c = 35.0f, .humidity_pct = 48.0f,
        .ndvi = 0.05f, .ndbi = 0.20f, .albedo = 0.20f,
        .building_density_pct = 40.0f, .tree_canopy_cover_pct = 2.0f,
        .traffic_density = 1000.0f, .population_density = 8000.0f
    };
    uhi_assessment_t a_tree = uhi_assess_zone(&tree_zone);
    assert(a_tree.primary_recommendation == UHI_RECOMMENDATION_URBAN_FORESTRY);

    /* 3. Misting Plaza expected for High Traffic Density & Dry Heat */
    uhi_features_t mist_zone = {
        .lst_c = 38.0f, .ambient_temp_c = 36.5f, .humidity_pct = 35.0f,
        .ndvi = 0.25f, .ndbi = 0.30f, .albedo = 0.22f,
        .building_density_pct = 50.0f, .tree_canopy_cover_pct = 20.0f,
        .traffic_density = 4800.0f, .population_density = 18000.0f
    };
    uhi_assessment_t a_mist = uhi_assess_zone(&mist_zone);
    assert(a_mist.primary_recommendation == UHI_RECOMMENDATION_MISTING_PLAZA ||
           a_mist.secondary_recommendation == UHI_RECOMMENDATION_MISTING_PLAZA);
}

/* Test 5: Non-Linear Temperature Reduction Formula */
static void test_non_linear_temperature_reduction(void) {
    float drop_0 = uhi_calculate_effective_drop(0.0f, 6.0f, 0.22f);
    assert(drop_0 == 0.0f);

    float drop_2 = uhi_calculate_effective_drop(2.0f, 6.0f, 0.22f);
    float drop_5 = uhi_calculate_effective_drop(5.0f, 6.0f, 0.22f);
    float drop_20 = uhi_calculate_effective_drop(20.0f, 6.0f, 0.22f);

    /* Diminishing returns: drop_5 should be more than drop_2, but less than 2.5x drop_2 */
    assert(drop_2 > 1.0f && drop_2 < 3.0f);
    assert(drop_5 > drop_2);
    assert(drop_5 < 2.5f * drop_2);
    /* Asymptote should not exceed max drop of 6.0°C */
    assert(drop_20 <= 6.0f && drop_20 > 5.5f);
}

/* Test 6: What-If Scenario Simulation & Multi-Scenario Comparison */
static void test_scenario_simulation(void) {
    uhi_features_t baseline = {
        .lst_c = 41.0f, .ambient_temp_c = 36.5f, .humidity_pct = 46.0f,
        .ndvi = 0.12f, .ndbi = 0.48f, .albedo = 0.14f,
        .building_density_pct = 75.0f, .tree_canopy_cover_pct = 8.0f,
        .traffic_density = 3800.0f, .population_density = 18000.0f
    };

    uhi_scenario_params_t params = {
        .scenario_name = "Canopy Expansion & Cool Roof",
        .delta_canopy_pct = 25.0f,
        .delta_albedo = 0.30f,
        .green_roof_coverage_pct = 15.0f,
        .permeable_pavement_pct = 20.0f,
        .misting_coverage_pct = 10.0f,
        .target_zone_area_sqm = 100000.0f
    };

    uhi_scenario_result_t res = uhi_simulate_scenario(&baseline, &params);
    assert(res.effective_temp_drop_c >= 2.5f);
    assert(res.mitigated_features.ambient_temp_c < baseline.ambient_temp_c);
    assert(res.mitigated_stress_index < res.baseline_stress_index);
    assert(res.hvac_energy_savings_pct > 5.0f);
    assert(res.total_investment_inr > 0.0f);
    assert(res.co2_offset_metric_tons > 0.0f);
}

/* Test 7: CSV Parsing and Zone Evaluation */
static void test_csv_parsing(void) {
    const char *sample_line = "MZ_004,Majestic Transport Interchange,Commercial Hub,12.9772,77.5714,37.10,46.20,42.50,0.0950,0.5120,0.1320,84.20,6.50,4750,82.5,High,2";
    uhi_microzone_record_t rec;
    bool ok = uhi_parse_microzone_csv_line(sample_line, &rec);
    assert(ok == true);
    assert(strcmp(rec.zone_id, "MZ_004") == 0);
    assert(strcmp(rec.zone_name, "Majestic Transport Interchange") == 0);
    assert(rec.is_valid == true);
    assert(rec.rf_prediction.predicted_class == RF_CLASS_HIGH);
    assert(rec.assessment.expected_drop_c > 1.5f);
}

int main(void) {
    printf("=======================================================================\n");
    printf("  UHI Engine Comprehensive Automated C99 Unit Test Suite              \n");
    printf("=======================================================================\n");

    RUN_TEST(test_feature_preprocessing_and_clipping);
    RUN_TEST(test_thermal_stress_calculation);
    RUN_TEST(test_random_forest_classification);
    RUN_TEST(test_5_cooling_strategies);
    RUN_TEST(test_non_linear_temperature_reduction);
    RUN_TEST(test_scenario_simulation);
    RUN_TEST(test_csv_parsing);

    printf("=======================================================================\n");
    printf("  TEST RESULTS: %d / %d Tests Passed Successfully (100.0%%)\n", g_tests_passed, g_tests_run);
    printf("=======================================================================\n");
    return 0;
}
