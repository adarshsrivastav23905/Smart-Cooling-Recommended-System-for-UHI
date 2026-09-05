#include "uhi_preprocessor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static float clamp_bound(float val, float min_val, float max_val, float fallback_median) {
    if (isnan(val) || isinf(val)) return fallback_median;
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return val;
}

uhi_features_t uhi_preprocess_features(const uhi_features_t *raw) {
    uhi_features_t clean = {0};
    if (!raw) return clean;

    clean.ambient_temp_c = clamp_bound(raw->ambient_temp_c, UHI_TEMP_MIN_C, UHI_TEMP_MAX_C, MEDIAN_TEMP_C);
    clean.humidity_pct = clamp_bound(raw->humidity_pct, UHI_HUMIDITY_MIN_PCT, UHI_HUMIDITY_MAX_PCT, MEDIAN_HUMIDITY_PCT);
    clean.lst_c = clamp_bound(raw->lst_c, UHI_LST_MIN_C, UHI_LST_MAX_C, MEDIAN_LST_C);
    clean.ndvi = clamp_bound(raw->ndvi, UHI_NDVI_MIN, UHI_NDVI_MAX, MEDIAN_NDVI);
    clean.ndbi = clamp_bound(raw->ndbi, UHI_NDBI_MIN, UHI_NDBI_MAX, MEDIAN_NDBI);
    clean.albedo = clamp_bound(raw->albedo, UHI_ALBEDO_MIN, UHI_ALBEDO_MAX, MEDIAN_ALBEDO);
    clean.building_density_pct = clamp_bound(raw->building_density_pct, 0.0f, 100.0f, MEDIAN_BUILDING_PCT);
    clean.tree_canopy_cover_pct = clamp_bound(raw->tree_canopy_cover_pct, 0.0f, 100.0f, MEDIAN_CANOPY_PCT);
    clean.traffic_density = clamp_bound(raw->traffic_density, 0.0f, 5000.0f, MEDIAN_TRAFFIC);
    clean.population_density = clamp_bound(raw->population_density, 0.0f, 50000.0f, MEDIAN_POPULATION);

    return clean;
}

bool uhi_validate_features(const uhi_features_t *f) {
    if (!f) return false;
    if (f->ambient_temp_c < UHI_TEMP_MIN_C || f->ambient_temp_c > UHI_TEMP_MAX_C) return false;
    if (f->humidity_pct < UHI_HUMIDITY_MIN_PCT || f->humidity_pct > UHI_HUMIDITY_MAX_PCT) return false;
    if (f->lst_c < UHI_LST_MIN_C || f->lst_c > UHI_LST_MAX_C) return false;
    return true;
}

void uhi_evaluate_microzone(uhi_microzone_record_t *record) {
    if (!record) return;

    /* 1. Preprocess and clean features */
    record->cleaned_features = uhi_preprocess_features(&record->raw_features);

    /* 2. Run Random Forest Classification in C */
    record->rf_prediction = uhi_rf_predict(&record->cleaned_features);

    /* 3. Run Context-Aware Recommendation & Impact Estimation in C */
    record->assessment = uhi_assess_zone(&record->cleaned_features);

    record->is_valid = true;
}

bool uhi_parse_microzone_csv_line(const char *csv_line, uhi_microzone_record_t *out_record) {
    if (!csv_line || !out_record || strlen(csv_line) < 10) return false;
    if (csv_line[0] == '#' || strncmp(csv_line, "zone_id", 7) == 0) return false;

    memset(out_record, 0, sizeof(uhi_microzone_record_t));

    char buf[512];
    strncpy(buf, csv_line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    /* Tokenize comma-separated values */
    char *tokens[20];
    int token_count = 0;
    char *token = strtok(buf, ",\r\n");

    while (token && token_count < 20) {
        tokens[token_count++] = token;
        token = strtok(NULL, ",\r\n");
    }

    if (token_count < 14) return false;

    strncpy(out_record->zone_id, tokens[0], sizeof(out_record->zone_id) - 1);
    strncpy(out_record->zone_name, tokens[1], sizeof(out_record->zone_name) - 1);
    strncpy(out_record->zone_type, tokens[2], sizeof(out_record->zone_type) - 1);

    out_record->latitude = atof(tokens[3]);
    out_record->longitude = atof(tokens[4]);

    out_record->raw_features.ambient_temp_c = (float)atof(tokens[5]);
    out_record->raw_features.humidity_pct = (float)atof(tokens[6]);
    out_record->raw_features.lst_c = (float)atof(tokens[7]);
    out_record->raw_features.ndvi = (float)atof(tokens[8]);
    out_record->raw_features.ndbi = (float)atof(tokens[9]);
    out_record->raw_features.albedo = (float)atof(tokens[10]);
    out_record->raw_features.building_density_pct = (float)atof(tokens[11]);
    out_record->raw_features.tree_canopy_cover_pct = (float)atof(tokens[12]);
    out_record->raw_features.traffic_density = (float)atof(tokens[13]);
    out_record->raw_features.population_density = (token_count > 14) ? (float)atof(tokens[14]) : 12000.0f;

    uhi_evaluate_microzone(out_record);
    return true;
}
