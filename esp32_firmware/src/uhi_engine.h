#ifndef UHI_ENGINE_H
#define UHI_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Heat Severity Tiers                                                        */
/* -------------------------------------------------------------------------- */
typedef enum {
    UHI_SEVERITY_LOW = 0,
    UHI_SEVERITY_MODERATE,
    UHI_SEVERITY_HIGH,
    UHI_SEVERITY_CRITICAL
} uhi_severity_t;

/* -------------------------------------------------------------------------- */
/* 5 Core Cooling Interventions (Tailored for Urban Micro-Zones)             */
/* -------------------------------------------------------------------------- */
typedef enum {
    UHI_RECOMMENDATION_NONE = 0,
    UHI_RECOMMENDATION_COOL_ROOF,               /* High-Albedo Cool Roof Coating & Reflective Membrane */
    UHI_RECOMMENDATION_URBAN_FORESTRY,         /* Intensive Urban Tree Canopy & Street Afforestation */
    UHI_RECOMMENDATION_GREEN_ROOFS,            /* Extensive Green Roofs & Bio-Solar Living Walls */
    UHI_RECOMMENDATION_PERMEABLE_PAVEMENT,     /* Permeable Interlocking Cool Pavements & Porous Asphalt */
    UHI_RECOMMENDATION_MISTING_PLAZA,          /* Micro-Misting Canopies & Urban Water Retention Plazas */
    UHI_RECOMMENDATION_COUNT
} uhi_recommendation_t;

/* -------------------------------------------------------------------------- */
/* Environmental & Multi-Spectral Feature Vector (10 Parameters)              */
/* -------------------------------------------------------------------------- */
typedef struct {
    float lst_c;                  /* Land Surface Temperature (°C, Satellite TIR) */
    float ambient_temp_c;         /* Dry-bulb air temperature (°C, ESP32 DHT22)   */
    float humidity_pct;           /* Relative humidity (%, ESP32 DHT22)           */
    float ndvi;                   /* Normalized Difference Vegetation Index       */
    float ndbi;                   /* Normalized Difference Built-up Index         */
    float albedo;                 /* Surface shortwave solar reflectance          */
    float building_density_pct;   /* Built footprint ratio per hectare (%)        */
    float tree_canopy_cover_pct;  /* Tree canopy crown coverage (%)               */
    float traffic_density;        /* Anthropogenic traffic flux (veh/hr)          */
    float population_density;     /* Population density (persons/sq.km)           */
} uhi_features_t;

/* -------------------------------------------------------------------------- */
/* Strategy Metadata & Cost Parameters                                        */
/* -------------------------------------------------------------------------- */
typedef struct {
    uhi_recommendation_t id;
    const char *code;
    const char *title;
    const char *category;
    const char *description;
    float base_temp_reduction_min; /* Minimum estimated ΔT (°C) */
    float base_temp_reduction_max; /* Maximum estimated ΔT (°C) */
    float cost_per_sqm_inr;        /* Capital expenditure (INR / m²) */
    uint32_t implementation_weeks; /* Typical deployment duration (weeks) */
} uhi_strategy_meta_t;

/* -------------------------------------------------------------------------- */
/* Comprehensive Assessment Result Structure                                  */
/* -------------------------------------------------------------------------- */
typedef struct {
    float thermal_stress_index;               /* Composite UTSI score (0 - 100) */
    uhi_severity_t severity;                  /* Low, Moderate, High, Critical */
    uhi_recommendation_t primary_recommendation;
    uhi_recommendation_t secondary_recommendation;
    float strategy_scores[UHI_RECOMMENDATION_COUNT]; /* Suitability scores */
    float raw_mitigation_potential;           /* Sum of raw reduction factors */
    float expected_drop_c;                    /* Quantified ΔT_effective (°C) */
    float hvac_savings_pct;                   /* Grid HVAC reduction (%) */
    float estimated_cost_inr_sqm;             /* Intervention cost in INR/m² */
    uint32_t estimated_time_weeks;            /* Execution duration (weeks) */
    char recommendation_reason[256];          /* Context-aware rationale string */
} uhi_assessment_t;

/* -------------------------------------------------------------------------- */
/* Function Declarations                                                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief Computes the composite Urban Thermal Stress Index (UTSI).
 */
float uhi_compute_thermal_stress(const uhi_features_t *features);

/**
 * @brief Performs context-aware multi-criteria evaluation of the micro-zone
 *        and recommends tailored cooling strategies with quantified ΔT impact.
 */
uhi_assessment_t uhi_assess_zone(const uhi_features_t *features);

/**
 * @brief Non-linear temperature reduction model with diminishing returns:
 *        ΔT_effective = ΔT_max * (1 - exp(-k * ΣΔ_raw))
 */
float uhi_calculate_effective_drop(float raw_delta, float max_drop, float k_factor);

/**
 * @brief Returns metadata and catalog details for a cooling strategy.
 */
uhi_strategy_meta_t uhi_get_strategy_meta(uhi_recommendation_t recommendation);

/**
 * @brief Converts recommendation enum to readable string name.
 */
const char *uhi_recommendation_name(uhi_recommendation_t recommendation);

/**
 * @brief Converts severity enum to readable string name.
 */
const char *uhi_severity_name(uhi_severity_t severity);

#ifdef __cplusplus
}
#endif

#endif /* UHI_ENGINE_H */