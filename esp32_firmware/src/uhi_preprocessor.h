#ifndef UHI_PREPROCESSOR_H
#define UHI_PREPROCESSOR_H

#include <stdbool.h>
#include <stdint.h>
#include "uhi_engine.h"
#include "uhi_random_forest.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ZONE_NAME_LEN 64
#define MAX_ZONE_ID_LEN 16
#define MAX_ZONE_TYPE_LEN 32

/* Physical Environmental Boundaries */
#define UHI_TEMP_MIN_C         15.0f
#define UHI_TEMP_MAX_C         50.0f
#define UHI_HUMIDITY_MIN_PCT    5.0f
#define UHI_HUMIDITY_MAX_PCT  100.0f
#define UHI_LST_MIN_C          15.0f
#define UHI_LST_MAX_C          60.0f
#define UHI_NDVI_MIN          -1.00f
#define UHI_NDVI_MAX          +1.00f
#define UHI_NDBI_MIN          -1.00f
#define UHI_NDBI_MAX          +1.00f
#define UHI_ALBEDO_MIN         0.05f
#define UHI_ALBEDO_MAX         0.85f

/* Climatological Median Fallbacks for Imputation */
#define MEDIAN_TEMP_C         29.5f
#define MEDIAN_HUMIDITY_PCT   62.0f
#define MEDIAN_LST_C          34.0f
#define MEDIAN_NDVI            0.28f
#define MEDIAN_NDBI            0.32f
#define MEDIAN_ALBEDO          0.20f
#define MEDIAN_BUILDING_PCT   65.0f
#define MEDIAN_CANOPY_PCT     22.0f
#define MEDIAN_TRAFFIC        1800.0f
#define MEDIAN_POPULATION    12500.0f

typedef struct {
    char zone_id[MAX_ZONE_ID_LEN];
    char zone_name[MAX_ZONE_NAME_LEN];
    char zone_type[MAX_ZONE_TYPE_LEN];
    double latitude;
    double longitude;
    uhi_features_t raw_features;
    uhi_features_t cleaned_features;
    uhi_assessment_t assessment;
    rf_prediction_t rf_prediction;
    bool is_valid;
} uhi_microzone_record_t;

/**
 * @brief Cleans, validates, and clips raw telemetry/satellite features to physical domain limits.
 */
uhi_features_t uhi_preprocess_features(const uhi_features_t *raw);

/**
 * @brief Checks if feature values are within physically valid ranges.
 */
bool uhi_validate_features(const uhi_features_t *f);

/**
 * @brief Parses a standard CSV line into an evaluated micro-zone record.
 */
bool uhi_parse_microzone_csv_line(const char *csv_line, uhi_microzone_record_t *out_record);

/**
 * @brief Evaluates an entire micro-zone record (preprocessing + Random Forest + Recommendation).
 */
void uhi_evaluate_microzone(uhi_microzone_record_t *record);

#ifdef __cplusplus
}
#endif

#endif /* UHI_PREPROCESSOR_H */
