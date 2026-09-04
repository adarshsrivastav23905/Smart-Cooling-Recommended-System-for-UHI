#include "uhi_engine.h"

#include <math.h>

static float clamp_float(float value, float minimum, float maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

float uhi_compute_thermal_stress(const uhi_features_t *features) {
    float heat_component = clamp_float((features->lst_c - 20.0f) * 2.0f, 0.0f, 60.0f);
    float ambient_component = clamp_float((features->ambient_temp_c - 20.0f) * 2.5f, 0.0f, 50.0f);
    float humidity_component = clamp_float((features->humidity_pct - 40.0f) * 0.15f, 0.0f, 9.0f);
    float vegetation_relief = clamp_float(features->ndvi, -1.0f, 1.0f) * 10.0f;
    float built_relief = clamp_float(features->ndbi, -1.0f, 1.0f) * 8.0f;

    return clamp_float(heat_component + ambient_component + humidity_component
                       + built_relief - vegetation_relief, 0.0f, 100.0f);
}

uhi_assessment_t uhi_assess_zone(const uhi_features_t *features) {
    uhi_assessment_t assessment = {0};
    float raw_mitigation = 0.0f;

    assessment.thermal_stress_index = uhi_compute_thermal_stress(features);

    if (features->ndbi >= 0.25f && features->albedo <= 0.30f) {
        assessment.recommendation = UHI_RECOMMENDATION_COOL_ROOF;
        raw_mitigation += 2.4f;
    }
    if (features->ndvi <= 0.25f || features->tree_canopy_cover_pct <= 20.0f) {
        assessment.recommendation = UHI_RECOMMENDATION_URBAN_FORESTRY;
        raw_mitigation += 3.4f;
    }
    if (features->building_density_pct >= 65.0f && features->albedo <= 0.40f) {
        assessment.recommendation = UHI_RECOMMENDATION_PERMEABLE_PAVEMENT;
        raw_mitigation += 1.7f;
    }
    if (features->traffic_density >= 2500.0f) {
        assessment.recommendation = UHI_RECOMMENDATION_MISTING_PLAZA;
        raw_mitigation += 2.0f;
    }

    assessment.expected_drop_c = clamp_float(6.0f * (1.0f - expf(-0.22f * raw_mitigation)), 0.0f, 6.0f);
    assessment.hvac_savings_pct = clamp_float(assessment.expected_drop_c * 2.2f, 0.0f, 100.0f);
    return assessment;
}

const char *uhi_recommendation_name(uhi_recommendation_t recommendation) {
    switch (recommendation) {
        case UHI_RECOMMENDATION_COOL_ROOF: return "COOL_ROOF";
        case UHI_RECOMMENDATION_URBAN_FORESTRY: return "URBAN_FORESTRY";
        case UHI_RECOMMENDATION_PERMEABLE_PAVEMENT: return "PERMEABLE_PAVEMENT";
        case UHI_RECOMMENDATION_MISTING_PLAZA: return "MISTING_PLAZA";
        default: return "NONE";
    }
}