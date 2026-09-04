#ifndef UHI_ENGINE_H
#define UHI_ENGINE_H

#include <stdbool.h>

typedef enum {
    UHI_RECOMMENDATION_NONE = 0,
    UHI_RECOMMENDATION_COOL_ROOF,
    UHI_RECOMMENDATION_URBAN_FORESTRY,
    UHI_RECOMMENDATION_PERMEABLE_PAVEMENT,
    UHI_RECOMMENDATION_MISTING_PLAZA
} uhi_recommendation_t;

typedef struct {
    float lst_c;
    float ambient_temp_c;
    float humidity_pct;
    float ndvi;
    float ndbi;
    float albedo;
    float building_density_pct;
    float tree_canopy_cover_pct;
    float traffic_density;
} uhi_features_t;

typedef struct {
    float thermal_stress_index;
    float expected_drop_c;
    float hvac_savings_pct;
    uhi_recommendation_t recommendation;
} uhi_assessment_t;

float uhi_compute_thermal_stress(const uhi_features_t *features);
uhi_assessment_t uhi_assess_zone(const uhi_features_t *features);
const char *uhi_recommendation_name(uhi_recommendation_t recommendation);

#endif