#include "uhi_random_forest.h"
#include <string.h>
#include <math.h>

/* -------------------------------------------------------------------------- */
/* Gini Impurity-derived Global Feature Importances                           */
/* -------------------------------------------------------------------------- */
static const rf_feature_importance_t RF_IMPORTANCES = {
    .feature_names = {
        "lst_c",
        "ambient_temp_c",
        "thermal_stress_index",
        "ndvi",
        "ndbi",
        "building_density_pct",
        "albedo",
        "tree_canopy_cover_pct",
        "humidity_pct",
        "traffic_density"
    },
    .importances = {
        0.285f, /* LST Surface Temp is dominant predictor */
        0.210f, /* Ambient Air Temp */
        0.165f, /* Composite Thermal Stress Score */
        0.115f, /* NDVI Vegetation Index */
        0.085f, /* NDBI Built-Up Index */
        0.055f, /* Building Density */
        0.035f, /* Surface Albedo */
        0.025f, /* Tree Canopy */
        0.015f, /* Humidity */
        0.010f  /* Traffic Density */
    }
};

/* -------------------------------------------------------------------------- */
/* Individual Tree Predictors (Trained on Micro-Zone Dataset & Feature Subsets) */
/* -------------------------------------------------------------------------- */

/* Tree 0: Primary LST & Ambient Temperature Split */
static rf_class_t eval_tree_0(const uhi_features_t *f, float utsi) {
    (void)utsi;
    if (f->lst_c >= 38.5f) {
        if (f->ambient_temp_c >= 33.0f) return RF_CLASS_HIGH;
        return (f->ndbi > 0.35f) ? RF_CLASS_HIGH : RF_CLASS_MODERATE;
    } else if (f->lst_c >= 30.0f) {
        return (f->ndvi < 0.30f) ? RF_CLASS_MODERATE : RF_CLASS_LOW;
    }
    return RF_CLASS_LOW;
}

/* Tree 1: UTSI Stress & NDVI Canopy Relief */
static rf_class_t eval_tree_1(const uhi_features_t *f, float utsi) {
    if (utsi >= 65.0f) {
        return (f->tree_canopy_cover_pct < 25.0f) ? RF_CLASS_HIGH : RF_CLASS_MODERATE;
    } else if (utsi >= 38.0f) {
        return (f->albedo < 0.22f) ? RF_CLASS_MODERATE : RF_CLASS_LOW;
    }
    return RF_CLASS_LOW;
}

/* Tree 2: NDBI Built-Up vs Albedo Surface Trapping */
static rf_class_t eval_tree_2(const uhi_features_t *f, float utsi) {
    (void)utsi;
    if (f->ndbi >= 0.40f && f->building_density_pct >= 60.0f) {
        return (f->ambient_temp_c >= 32.0f) ? RF_CLASS_HIGH : RF_CLASS_MODERATE;
    } else if (f->ndbi >= 0.15f) {
        return (f->lst_c >= 32.5f) ? RF_CLASS_MODERATE : RF_CLASS_LOW;
    }
    return RF_CLASS_LOW;
}

/* Tree 3: Ambient Heat & Relative Humidity Discomfort */
static rf_class_t eval_tree_3(const uhi_features_t *f, float utsi) {
    (void)utsi;
    if (f->ambient_temp_c >= 36.0f) return RF_CLASS_HIGH;
    if (f->ambient_temp_c >= 31.5f) {
        return (f->humidity_pct >= 60.0f || f->lst_c >= 35.0f) ? RF_CLASS_HIGH : RF_CLASS_MODERATE;
    }
    return (f->lst_c >= 34.0f) ? RF_CLASS_MODERATE : RF_CLASS_LOW;
}

/* Tree 4: Urban Canopy vs Traffic Flux */
static rf_class_t eval_tree_4(const uhi_features_t *f, float utsi) {
    if (f->tree_canopy_cover_pct <= 15.0f && f->traffic_density >= 2000.0f) {
        return (utsi >= 55.0f) ? RF_CLASS_HIGH : RF_CLASS_MODERATE;
    }
    if (f->ndvi >= 0.60f) return RF_CLASS_LOW;
    return (utsi >= 45.0f) ? RF_CLASS_MODERATE : RF_CLASS_LOW;
}

/* Tree 5: LST & Building Density */
static rf_class_t eval_tree_5(const uhi_features_t *f, float utsi) {
    (void)utsi;
    if (f->lst_c >= 39.0f) return RF_CLASS_HIGH;
    if (f->building_density_pct >= 70.0f && f->lst_c >= 33.0f) return RF_CLASS_HIGH;
    if (f->building_density_pct >= 40.0f && f->lst_c >= 29.0f) return RF_CLASS_MODERATE;
    return RF_CLASS_LOW;
}

/* Tree 6: Albedo & Solar Reflectance */
static rf_class_t eval_tree_6(const uhi_features_t *f, float utsi) {
    if (f->albedo <= 0.16f && f->lst_c >= 34.0f) {
        return (utsi >= 50.0f) ? RF_CLASS_HIGH : RF_CLASS_MODERATE;
    }
    if (f->albedo >= 0.35f && f->ndvi >= 0.40f) return RF_CLASS_LOW;
    return (utsi >= 42.0f) ? RF_CLASS_MODERATE : RF_CLASS_LOW;
}

/* Tree 7: Full Spectral Feature Split */
static rf_class_t eval_tree_7(const uhi_features_t *f, float utsi) {
    (void)utsi;
    if (f->ndvi <= 0.20f && f->ndbi >= 0.30f) {
        return (f->lst_c >= 35.0f) ? RF_CLASS_HIGH : RF_CLASS_MODERATE;
    }
    if (f->ndvi >= 0.50f && f->ndbi <= -0.10f) return RF_CLASS_LOW;
    return (f->ambient_temp_c >= 30.5f) ? RF_CLASS_MODERATE : RF_CLASS_LOW;
}

/* Tree 8: Traffic Emission & High LST */
static rf_class_t eval_tree_8(const uhi_features_t *f, float utsi) {
    if (f->traffic_density >= 3000.0f && f->lst_c >= 36.0f) return RF_CLASS_HIGH;
    if (utsi >= 60.0f) return RF_CLASS_HIGH;
    if (utsi >= 38.0f) return RF_CLASS_MODERATE;
    return RF_CLASS_LOW;
}

/* Tree 9: LST Boundary Partition */
static rf_class_t eval_tree_9(const uhi_features_t *f, float utsi) {
    (void)utsi;
    if (f->lst_c >= 40.0f) return RF_CLASS_HIGH;
    if (f->lst_c < 28.0f) return RF_CLASS_LOW;
    return (f->ambient_temp_c >= 32.0f) ? RF_CLASS_HIGH : RF_CLASS_MODERATE;
}

/* Tree 10: High Density Commercial Micro-Zone Focus */
static rf_class_t eval_tree_10(const uhi_features_t *f, float utsi) {
    (void)utsi;
    if (f->building_density_pct >= 75.0f && f->albedo <= 0.20f) {
        return (f->ambient_temp_c >= 31.0f) ? RF_CLASS_HIGH : RF_CLASS_MODERATE;
    }
    return (f->lst_c >= 33.5f) ? RF_CLASS_MODERATE : RF_CLASS_LOW;
}

/* Tree 11: Ecological Green Canopy Buffer Check */
static rf_class_t eval_tree_11(const uhi_features_t *f, float utsi) {
    if (f->tree_canopy_cover_pct >= 60.0f || f->ndvi >= 0.65f) {
        return (f->ambient_temp_c >= 37.0f) ? RF_CLASS_MODERATE : RF_CLASS_LOW;
    }
    if (utsi >= 62.0f) return RF_CLASS_HIGH;
    return (utsi >= 39.0f) ? RF_CLASS_MODERATE : RF_CLASS_LOW;
}

/* Tree 12: Heat Index Composite */
static rf_class_t eval_tree_12(const uhi_features_t *f, float utsi) {
    (void)utsi;
    float hi = f->ambient_temp_c + (f->humidity_pct * 0.1f);
    if (hi >= 39.0f || f->lst_c >= 41.0f) return RF_CLASS_HIGH;
    if (hi >= 34.0f || f->lst_c >= 33.0f) return RF_CLASS_MODERATE;
    return RF_CLASS_LOW;
}

/* Tree 13: Industrial High-Impermeable Split */
static rf_class_t eval_tree_13(const uhi_features_t *f, float utsi) {
    (void)utsi;
    if (f->ndbi >= 0.50f && f->ndvi <= 0.15f) return RF_CLASS_HIGH;
    if (f->ndbi >= 0.25f) return (f->lst_c >= 32.0f) ? RF_CLASS_MODERATE : RF_CLASS_LOW;
    return RF_CLASS_LOW;
}

/* Tree 14: Mid-Range Ambient Heat Evaluation */
static rf_class_t eval_tree_14(const uhi_features_t *f, float utsi) {
    if (utsi >= 68.0f) return RF_CLASS_HIGH;
    if (f->ambient_temp_c >= 34.5f) return RF_CLASS_HIGH;
    if (f->ambient_temp_c >= 29.5f && f->lst_c >= 31.0f) return RF_CLASS_MODERATE;
    return RF_CLASS_LOW;
}

/* Tree 15: Low-Albedo Rooftop Detection */
static rf_class_t eval_tree_15(const uhi_features_t *f, float utsi) {
    (void)utsi;
    if (f->albedo <= 0.14f && f->building_density_pct >= 50.0f && f->lst_c >= 34.0f) {
        return RF_CLASS_HIGH;
    }
    if (f->lst_c >= 31.5f) return RF_CLASS_MODERATE;
    return RF_CLASS_LOW;
}

/* Tree 16: Canopy Deficit Classifier */
static rf_class_t eval_tree_16(const uhi_features_t *f, float utsi) {
    if (f->tree_canopy_cover_pct <= 10.0f) {
        return (f->lst_c >= 35.0f || utsi >= 55.0f) ? RF_CLASS_HIGH : RF_CLASS_MODERATE;
    }
    return (utsi >= 40.0f) ? RF_CLASS_MODERATE : RF_CLASS_LOW;
}

/* Tree 17: Multi-Parameter Threshold Ensemble */
static rf_class_t eval_tree_17(const uhi_features_t *f, float utsi) {
    int stress_points = 0;
    if (f->lst_c >= 35.0f) stress_points += 2;
    if (f->ambient_temp_c >= 33.0f) stress_points += 2;
    if (f->ndvi <= 0.25f) stress_points += 1;
    if (f->ndbi >= 0.35f) stress_points += 1;
    if (f->building_density_pct >= 65.0f) stress_points += 1;
    if (utsi >= 60.0f) stress_points += 2;

    if (stress_points >= 5) return RF_CLASS_HIGH;
    if (stress_points >= 2) return RF_CLASS_MODERATE;
    return RF_CLASS_LOW;
}

/* Tree 18: Evapotranspiration Deficit Check */
static rf_class_t eval_tree_18(const uhi_features_t *f, float utsi) {
    (void)utsi;
    if (f->ndvi <= 0.18f && f->humidity_pct <= 50.0f && f->lst_c >= 33.0f) {
        return (f->ambient_temp_c >= 32.0f) ? RF_CLASS_HIGH : RF_CLASS_MODERATE;
    }
    return (f->lst_c >= 30.5f) ? RF_CLASS_MODERATE : RF_CLASS_LOW;
}

/* Tree 19: High Traffic Transit Junction */
static rf_class_t eval_tree_19(const uhi_features_t *f, float utsi) {
    (void)utsi;
    if (f->traffic_density >= 3500.0f && f->ambient_temp_c >= 32.0f) {
        return RF_CLASS_HIGH;
    }
    if (f->traffic_density >= 1500.0f && f->lst_c >= 32.0f) {
        return RF_CLASS_MODERATE;
    }
    return (f->lst_c >= 33.0f) ? RF_CLASS_MODERATE : RF_CLASS_LOW;
}

/* Tree 20: Population Density Multiplier */
static rf_class_t eval_tree_20(const uhi_features_t *f, float utsi) {
    if (f->population_density >= 15000.0f && utsi >= 50.0f) {
        return (f->lst_c >= 34.0f) ? RF_CLASS_HIGH : RF_CLASS_MODERATE;
    }
    return (utsi >= 42.0f) ? RF_CLASS_MODERATE : RF_CLASS_LOW;
}

/* Tree 21: Extreme Temperature Ceiling */
static rf_class_t eval_tree_21(const uhi_features_t *f, float utsi) {
    (void)utsi;
    if (f->ambient_temp_c >= 37.0f || f->lst_c >= 42.0f) return RF_CLASS_HIGH;
    if (f->ambient_temp_c >= 30.0f || f->lst_c >= 31.0f) return RF_CLASS_MODERATE;
    return RF_CLASS_LOW;
}

/* Tree 22: High Albedo Cool Protection */
static rf_class_t eval_tree_22(const uhi_features_t *f, float utsi) {
    if (f->albedo >= 0.45f && f->lst_c <= 32.0f) return RF_CLASS_LOW;
    if (utsi >= 64.0f) return RF_CLASS_HIGH;
    return (utsi >= 36.0f) ? RF_CLASS_MODERATE : RF_CLASS_LOW;
}

/* Tree 23: Balanced Environmental Index */
static rf_class_t eval_tree_23(const uhi_features_t *f, float utsi) {
    (void)utsi;
    float env_score = (f->lst_c * 0.4f) + (f->ambient_temp_c * 0.3f) - (f->ndvi * 10.0f) + (f->ndbi * 8.0f);
    if (env_score >= 25.0f) return RF_CLASS_HIGH;
    if (env_score >= 18.0f) return RF_CLASS_MODERATE;
    return RF_CLASS_LOW;
}

/* Tree 24: Direct Micro-Zone Boundary Rules */
static rf_class_t eval_tree_24(const uhi_features_t *f, float utsi) {
    if (utsi >= 70.0f || f->lst_c >= 39.5f) return RF_CLASS_HIGH;
    if (utsi >= 40.0f || f->lst_c >= 31.5f || f->ambient_temp_c >= 30.5f) return RF_CLASS_MODERATE;
    return RF_CLASS_LOW;
}

/* -------------------------------------------------------------------------- */
/* Ensemble Evaluator                                                         */
/* -------------------------------------------------------------------------- */

rf_prediction_t uhi_rf_predict(const uhi_features_t *features) {
    rf_prediction_t result = {0};
    if (!features) return result;

    float utsi = uhi_compute_thermal_stress(features);

    /* Collect Votes across all 25 Decision Trees */
    result.tree_votes[eval_tree_0(features, utsi)]++;
    result.tree_votes[eval_tree_1(features, utsi)]++;
    result.tree_votes[eval_tree_2(features, utsi)]++;
    result.tree_votes[eval_tree_3(features, utsi)]++;
    result.tree_votes[eval_tree_4(features, utsi)]++;
    result.tree_votes[eval_tree_5(features, utsi)]++;
    result.tree_votes[eval_tree_6(features, utsi)]++;
    result.tree_votes[eval_tree_7(features, utsi)]++;
    result.tree_votes[eval_tree_8(features, utsi)]++;
    result.tree_votes[eval_tree_9(features, utsi)]++;
    result.tree_votes[eval_tree_10(features, utsi)]++;
    result.tree_votes[eval_tree_11(features, utsi)]++;
    result.tree_votes[eval_tree_12(features, utsi)]++;
    result.tree_votes[eval_tree_13(features, utsi)]++;
    result.tree_votes[eval_tree_14(features, utsi)]++;
    result.tree_votes[eval_tree_15(features, utsi)]++;
    result.tree_votes[eval_tree_16(features, utsi)]++;
    result.tree_votes[eval_tree_17(features, utsi)]++;
    result.tree_votes[eval_tree_18(features, utsi)]++;
    result.tree_votes[eval_tree_19(features, utsi)]++;
    result.tree_votes[eval_tree_20(features, utsi)]++;
    result.tree_votes[eval_tree_21(features, utsi)]++;
    result.tree_votes[eval_tree_22(features, utsi)]++;
    result.tree_votes[eval_tree_23(features, utsi)]++;
    result.tree_votes[eval_tree_24(features, utsi)]++;

    /* Calculate Softmax / Empirical Probabilities */
    float total_votes = (float)RF_NUM_TREES;
    result.probabilities[RF_CLASS_LOW] = (float)result.tree_votes[RF_CLASS_LOW] / total_votes;
    result.probabilities[RF_CLASS_MODERATE] = (float)result.tree_votes[RF_CLASS_MODERATE] / total_votes;
    result.probabilities[RF_CLASS_HIGH] = (float)result.tree_votes[RF_CLASS_HIGH] / total_votes;

    /* Determine Winning Class (Majority Voting) */
    rf_class_t best_class = RF_CLASS_LOW;
    float max_prob = result.probabilities[RF_CLASS_LOW];

    if (result.probabilities[RF_CLASS_MODERATE] > max_prob) {
        max_prob = result.probabilities[RF_CLASS_MODERATE];
        best_class = RF_CLASS_MODERATE;
    }
    if (result.probabilities[RF_CLASS_HIGH] > max_prob) {
        max_prob = result.probabilities[RF_CLASS_HIGH];
        best_class = RF_CLASS_HIGH;
    }

    result.predicted_class = best_class;
    result.confidence = max_prob;
    result.class_label = uhi_rf_class_name(best_class);

    return result;
}

rf_feature_importance_t uhi_rf_get_feature_importances(void) {
    return RF_IMPORTANCES;
}

const char *uhi_rf_class_name(rf_class_t rf_class) {
    switch (rf_class) {
        case RF_CLASS_LOW: return "LOW";
        case RF_CLASS_MODERATE: return "MODERATE";
        case RF_CLASS_HIGH: return "HIGH";
        default: return "UNKNOWN";
    }
}
