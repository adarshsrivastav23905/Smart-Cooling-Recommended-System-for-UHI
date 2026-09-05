#include "uhi_engine.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

static float clamp_f(float value, float minimum, float maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static const uhi_strategy_meta_t STRATEGY_CATALOG[UHI_RECOMMENDATION_COUNT] = {
    {
        .id = UHI_RECOMMENDATION_NONE,
        .code = "NONE",
        .title = "No Specific Intervention Required",
        .category = "Baseline",
        .description = "Micro-climate thermal conditions are within safe comfort thresholds.",
        .base_temp_reduction_min = 0.0f,
        .base_temp_reduction_max = 0.0f,
        .cost_per_sqm_inr = 0.0f,
        .implementation_weeks = 0
    },
    {
        .id = UHI_RECOMMENDATION_COOL_ROOF,
        .code = "COOL_ROOF_ALBEDO",
        .title = "High-Albedo Cool Roof Coating & Reflective Membrane",
        .category = "Surface Modification",
        .description = "High-reflectance titanium dioxide elastomeric or thermoplastic white roof coatings (SRI > 80) reflecting shortwave solar radiation before absorption.",
        .base_temp_reduction_min = 1.5f,
        .base_temp_reduction_max = 3.2f,
        .cost_per_sqm_inr = 350.0f,
        .implementation_weeks = 3
    },
    {
        .id = UHI_RECOMMENDATION_URBAN_FORESTRY,
        .code = "URBAN_FORESTRY_CANOPY",
        .title = "Intensive Urban Tree Canopy & Street Afforestation",
        .category = "Green Infrastructure",
        .description = "Dense multi-tier native shade tree planting (Neem, Honge, Peepal) to shade impermeable surfaces and maximize cooling via latent heat evapotranspiration.",
        .base_temp_reduction_min = 2.2f,
        .base_temp_reduction_max = 4.8f,
        .cost_per_sqm_inr = 600.0f,
        .implementation_weeks = 8
    },
    {
        .id = UHI_RECOMMENDATION_GREEN_ROOFS,
        .code = "GREEN_ROOFS_VERTICAL_GARDEN",
        .title = "Extensive Green Roofs & Bio-Solar Living Walls",
        .category = "Green Infrastructure",
        .description = "Lightweight modular sedum and succulent vegetative layers on roofs and vertical living facades to insulate building envelope and reduce sensible heat flux.",
        .base_temp_reduction_min = 1.2f,
        .base_temp_reduction_max = 2.6f,
        .cost_per_sqm_inr = 1200.0f,
        .implementation_weeks = 6
    },
    {
        .id = UHI_RECOMMENDATION_PERMEABLE_PAVEMENT,
        .code = "PERMEABLE_COOL_PAVEMENTS",
        .title = "Permeable Interlocking Cool Pavements & Porous Asphalt",
        .category = "Pavement Engineering",
        .description = "High-albedo porous concrete and interlocking pavers replacing standard impervious blacktop asphalt, retaining subsurface moisture for evaporative cooling.",
        .base_temp_reduction_min = 1.0f,
        .base_temp_reduction_max = 2.4f,
        .cost_per_sqm_inr = 850.0f,
        .implementation_weeks = 5
    },
    {
        .id = UHI_RECOMMENDATION_MISTING_PLAZA,
        .code = "EVAPORATIVE_MISTING_WATER_PLAZA",
        .title = "Micro-Misting Canopies & Urban Water Retention Plazas",
        .category = "Active Evaporative Cooling",
        .description = "High-pressure micro-nozzle misting lines and shallow daylighted water retention fountains to rapidly lower localized ambient air temperatures by sensible heat absorption.",
        .base_temp_reduction_min = 1.5f,
        .base_temp_reduction_max = 3.0f,
        .cost_per_sqm_inr = 950.0f,
        .implementation_weeks = 4
    }
};

float uhi_compute_thermal_stress(const uhi_features_t *features) {
    if (!features) return 0.0f;

    /* LST Surface Heat Load (15°C to 55°C mapped to 0-35 pts) */
    float surface_load = clamp_f((features->lst_c - 20.0f) * 1.0f, 0.0f, 35.0f);

    /* Ambient Air Heat Load (20°C to 45°C mapped to 0-30 pts) */
    float air_load = clamp_f((features->ambient_temp_c - 22.0f) * 1.3f, 0.0f, 30.0f);

    /* Relative Humidity Discomfort Factor */
    float humidity_load = 0.0f;
    if (features->humidity_pct > 50.0f) {
        humidity_load = clamp_f((features->humidity_pct - 50.0f) * 0.25f, 0.0f, 12.0f);
    }

    /* Built-up Impervious Surface Factor (NDBI and Building Density) */
    float built_load = clamp_f(features->ndbi * 12.0f + (features->building_density_pct / 100.0f) * 10.0f, 0.0f, 22.0f);

    /* Low Albedo Heat Trapping Penalty */
    float albedo_penalty = 0.0f;
    if (features->albedo < 0.25f) {
        albedo_penalty = (0.25f - features->albedo) * 20.0f;
    }

    /* Anthropogenic Traffic Load */
    float traffic_load = clamp_f((features->traffic_density / 5000.0f) * 8.0f, 0.0f, 8.0f);

    /* Vegetative Relief Offset (NDVI and Tree Canopy) */
    float veg_relief = clamp_f(features->ndvi * 12.0f + (features->tree_canopy_cover_pct / 100.0f) * 10.0f, 0.0f, 22.0f);

    float total_stress = surface_load + air_load + humidity_load + built_load + albedo_penalty + traffic_load - veg_relief;
    return clamp_f(total_stress, 0.0f, 100.0f);
}

float uhi_calculate_effective_drop(float raw_delta, float max_drop, float k_factor) {
    if (raw_delta <= 0.0f) return 0.0f;
    return clamp_f(max_drop * (1.0f - expf(-k_factor * raw_delta)), 0.0f, max_drop);
}

uhi_strategy_meta_t uhi_get_strategy_meta(uhi_recommendation_t recommendation) {
    if (recommendation >= 0 && recommendation < UHI_RECOMMENDATION_COUNT) {
        return STRATEGY_CATALOG[recommendation];
    }
    return STRATEGY_CATALOG[UHI_RECOMMENDATION_NONE];
}

uhi_assessment_t uhi_assess_zone(const uhi_features_t *features) {
    uhi_assessment_t assessment = {0};
    if (!features) return assessment;

    assessment.thermal_stress_index = uhi_compute_thermal_stress(features);

    /* Determine baseline severity */
    if (assessment.thermal_stress_index >= 70.0f || features->ambient_temp_c >= 36.5f || features->lst_c >= 42.0f) {
        assessment.severity = UHI_SEVERITY_HIGH;
    } else if (assessment.thermal_stress_index >= 40.0f || features->ambient_temp_c >= 31.0f || features->lst_c >= 33.0f) {
        assessment.severity = UHI_SEVERITY_MODERATE;
    } else {
        assessment.severity = UHI_SEVERITY_LOW;
    }

    /* Score each of the 5 interventions based on multi-criteria suitability */
    float scores[UHI_RECOMMENDATION_COUNT] = {0};

    /* 1. Cool Roofs: Favored when NDBI is high, building density is high, albedo is low, and LST is elevated */
    scores[UHI_RECOMMENDATION_COOL_ROOF] =
        (features->building_density_pct / 100.0f) * 30.0f +
        clamp_f(features->ndbi * 25.0f, 0.0f, 25.0f) +
        clamp_f((0.30f - features->albedo) * 60.0f, 0.0f, 25.0f) +
        clamp_f((features->lst_c - 30.0f) * 1.0f, 0.0f, 20.0f);

    /* 2. Urban Forestry: Favored when NDVI/Canopy is low and thermal stress is high */
    scores[UHI_RECOMMENDATION_URBAN_FORESTRY] =
        clamp_f((0.50f - features->ndvi) * 50.0f, 0.0f, 35.0f) +
        clamp_f((40.0f - features->tree_canopy_cover_pct) * 0.8f, 0.0f, 30.0f) +
        clamp_f((assessment.thermal_stress_index - 30.0f) * 0.7f, 0.0f, 35.0f);

    /* 3. Green Roofs & Living Walls: Favored in dense built environments with limited street space for large trees */
    scores[UHI_RECOMMENDATION_GREEN_ROOFS] =
        (features->building_density_pct / 100.0f) * 35.0f +
        clamp_f((0.35f - features->ndvi) * 40.0f, 0.0f, 25.0f) +
        clamp_f((features->lst_c - 28.0f) * 1.2f, 0.0f, 20.0f) +
        clamp_f(features->ndbi * 20.0f, 0.0f, 20.0f);

    /* 4. Permeable Cool Pavements: Favored in transit/paved zones with high NDBI and low albedo */
    scores[UHI_RECOMMENDATION_PERMEABLE_PAVEMENT] =
        clamp_f(features->ndbi * 30.0f, 0.0f, 30.0f) +
        clamp_f((0.25f - features->albedo) * 50.0f, 0.0f, 25.0f) +
        clamp_f((features->lst_c - 32.0f) * 1.2f, 0.0f, 25.0f) +
        clamp_f((features->traffic_density / 5000.0f) * 20.0f, 0.0f, 20.0f);

    /* 5. Misting Plazas & Water Features: Favored in high footfall/traffic areas with moderate/low humidity and high temp */
    scores[UHI_RECOMMENDATION_MISTING_PLAZA] =
        clamp_f((features->traffic_density / 4000.0f) * 35.0f, 0.0f, 35.0f) +
        clamp_f((features->ambient_temp_c - 30.0f) * 2.0f, 0.0f, 30.0f) +
        clamp_f((65.0f - features->humidity_pct) * 0.5f, 0.0f, 20.0f) +
        clamp_f((assessment.thermal_stress_index - 40.0f) * 0.3f, 0.0f, 15.0f);

    for (int i = 0; i < UHI_RECOMMENDATION_COUNT; i++) {
        assessment.strategy_scores[i] = scores[i];
    }

    /* Find Top 2 Primary and Secondary Recommendations */
    uhi_recommendation_t best_rec = UHI_RECOMMENDATION_NONE;
    uhi_recommendation_t second_rec = UHI_RECOMMENDATION_NONE;
    float best_score = -1.0f;
    float second_score = -1.0f;

    for (int i = 1; i < UHI_RECOMMENDATION_COUNT; i++) {
        if (scores[i] > best_score) {
            second_score = best_score;
            second_rec = best_rec;
            best_score = scores[i];
            best_rec = (uhi_recommendation_t)i;
        } else if (scores[i] > second_score) {
            second_score = scores[i];
            second_rec = (uhi_recommendation_t)i;
        }
    }

    assessment.primary_recommendation = best_rec;
    assessment.secondary_recommendation = second_rec;

    uhi_strategy_meta_t best_meta = uhi_get_strategy_meta(best_rec);
    uhi_strategy_meta_t sec_meta = uhi_get_strategy_meta(second_rec);

    /* Calculate raw mitigation potential */
    float avg_best_drop = (best_meta.base_temp_reduction_min + best_meta.base_temp_reduction_max) * 0.5f;
    float avg_sec_drop = (sec_meta.base_temp_reduction_min + sec_meta.base_temp_reduction_max) * 0.5f;
    assessment.raw_mitigation_potential = avg_best_drop + (avg_sec_drop * 0.45f);

    /* Compute non-linear effective temperature drop: ΔT_effective = ΔT_max * (1 - exp(-k * ΣΔ_raw)) */
    assessment.expected_drop_c = uhi_calculate_effective_drop(assessment.raw_mitigation_potential, 6.0f, 0.22f);

    /* HVAC energy savings metric: ~2.2% grid HVAC energy reduction per 1°C ambient drop */
    assessment.hvac_savings_pct = clamp_f(assessment.expected_drop_c * 2.2f, 0.0f, 25.0f);

    assessment.estimated_cost_inr_sqm = best_meta.cost_per_sqm_inr;
    assessment.estimated_time_weeks = best_meta.implementation_weeks;

    /* Build Context-Aware Rationale String */
    snprintf(assessment.recommendation_reason, sizeof(assessment.recommendation_reason),
             "High-priority prescription '%s' selected for UTSI %.1f (LST %.1f°C, NDVI %.2f, Built %.0f%%). Projected cooling drop: -%.1f°C with %.1f%% HVAC energy savings.",
             best_meta.title, assessment.thermal_stress_index, features->lst_c,
             features->ndvi, features->building_density_pct, assessment.expected_drop_c, assessment.hvac_savings_pct);

    return assessment;
}

const char *uhi_recommendation_name(uhi_recommendation_t recommendation) {
    return uhi_get_strategy_meta(recommendation).code;
}

const char *uhi_severity_name(uhi_severity_t severity) {
    switch (severity) {
        case UHI_SEVERITY_LOW: return "LOW";
        case UHI_SEVERITY_MODERATE: return "MODERATE";
        case UHI_SEVERITY_HIGH: return "HIGH";
        case UHI_SEVERITY_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}