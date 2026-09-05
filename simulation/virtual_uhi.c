#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "uhi_engine.h"
#include "uhi_random_forest.h"
#include "uhi_scenario_sim.h"
#include "uhi_preprocessor.h"

static void print_banner(void) {
    fprintf(stderr, "=======================================================================\n");
    fprintf(stderr, " Smart Cooling Strategy Recommendation System for Urban Heat Island   \n");
    fprintf(stderr, " Native C99 Decision-Support & Micro-Climate Simulation Engine         \n");
    fprintf(stderr, "=======================================================================\n");
}

static void print_usage(const char *prog) {
    print_banner();
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s [count]                        - Stream [count] virtual telemetry packets\n", prog);
    fprintf(stderr, "  %s --eval-csv <file.csv> [limit]  - Evaluate micro-zones from dataset CSV\n", prog);
    fprintf(stderr, "  %s --scenario                     - Run scenario comparison simulation\n", prog);
    fprintf(stderr, "  %s --export-plan <output.json>    - Export Municipal Action Plan to JSON\n", prog);
}

static void run_virtual_stream(int packet_limit) {
    if (packet_limit <= 0) packet_limit = 10;

    for (int i = 1; i <= packet_limit; ++i) {
        float phase = (float)i * 0.45f;
        float ambient = 32.0f + 4.5f * sinf(phase);
        float humidity = 58.0f - 12.0f * sinf(phase);
        float lst = ambient + 7.5f + 1.5f * fmaxf(0.0f, sinf(phase));
        float heat_index = ambient + (humidity * 0.1f);

        uhi_features_t f = {
            .lst_c = lst,
            .ambient_temp_c = ambient,
            .humidity_pct = humidity,
            .ndvi = 0.22f + 0.05f * cosf(phase),
            .ndbi = 0.38f - 0.03f * cosf(phase),
            .albedo = 0.18f,
            .building_density_pct = 72.0f,
            .tree_canopy_cover_pct = 14.0f,
            .traffic_density = 2400.0f + 500.0f * sinf(phase),
            .population_density = 14500.0f
        };

        uhi_assessment_t assessment = uhi_assess_zone(&f);
        rf_prediction_t rf = uhi_rf_predict(&f);
        uhi_strategy_meta_t meta = uhi_get_strategy_meta(assessment.primary_recommendation);

        printf("{"
               "\"packet_id\":%d,"
               "\"node_id\":\"VIRTUAL_UHI_NODE_C\","
               "\"ambient_temp_c\":%.2f,"
               "\"relative_humidity_pct\":%.2f,"
               "\"lst_c\":%.2f,"
               "\"heat_index_c\":%.2f,"
               "\"severity\":\"%s\","
               "\"rf_class\":\"%s\","
               "\"rf_confidence\":%.2f,"
               "\"rf_probs\":{\"low\":%.2f,\"moderate\":%.2f,\"high\":%.2f},"
               "\"thermal_stress_index\":%.2f,"
               "\"recommendation\":\"%s\","
               "\"recommendation_title\":\"%s\","
               "\"expected_drop_c\":%.2f,"
               "\"hvac_savings_pct\":%.2f,"
               "\"cost_inr_sqm\":%.0f,"
               "\"implementation_weeks\":%u,"
               "\"recommendation_reason\":\"%s\","
               "\"green_led\":%s,"
               "\"yellow_led\":%s,"
               "\"red_led\":%s,"
               "\"buzzer\":%s,"
               "\"uptime_ms\":%d"
               "}\n",
               i,
               ambient,
               humidity,
               lst,
               heat_index,
               uhi_severity_name(assessment.severity),
               rf.class_label,
               rf.confidence,
               rf.probabilities[0], rf.probabilities[1], rf.probabilities[2],
               assessment.thermal_stress_index,
               meta.code,
               meta.title,
               assessment.expected_drop_c,
               assessment.hvac_savings_pct,
               meta.cost_per_sqm_inr,
               meta.implementation_weeks,
               assessment.recommendation_reason,
               (assessment.severity == UHI_SEVERITY_LOW) ? "true" : "false",
               (assessment.severity == UHI_SEVERITY_MODERATE) ? "true" : "false",
               (assessment.severity == UHI_SEVERITY_HIGH) ? "true" : "false",
               (ambient >= 38.0f) ? "true" : "false",
               i * 1000);
        fflush(stdout);
    }
}

static void run_eval_csv(const char *csv_path, int limit) {
    FILE *fp = fopen(csv_path, "r");
    if (!fp) {
        fprintf(stderr, "Error: Could not open CSV file '%s'\n", csv_path);
        return;
    }

    print_banner();
    printf("Evaluating micro-zones from '%s'...\n\n", csv_path);
    printf("%-8s | %-28s | %-6s | %-6s | %-6s | %-8s | %-22s | %-8s\n",
           "Zone ID", "Micro-Zone Name", "Temp", "LST", "UTSI", "RF Class", "Primary Strategy", "Exp Drop");
    printf("-------------------------------------------------------------------------------------------------------------\n");

    char line[512];
    int count = 0;
    int class_counts[3] = {0};
    float total_drop = 0.0f;

    while (fgets(line, sizeof(line), fp)) {
        uhi_microzone_record_t rec;
        if (uhi_parse_microzone_csv_line(line, &rec)) {
            count++;
            class_counts[rec.rf_prediction.predicted_class]++;
            total_drop += rec.assessment.expected_drop_c;

            uhi_strategy_meta_t meta = uhi_get_strategy_meta(rec.assessment.primary_recommendation);
            printf("%-8s | %-28.28s | %5.1fC | %5.1fC | %6.1f | %-8s | %-22.22s | -%4.1fC\n",
                   rec.zone_id, rec.zone_name,
                   rec.cleaned_features.ambient_temp_c,
                   rec.cleaned_features.lst_c,
                   rec.assessment.thermal_stress_index,
                   rec.rf_prediction.class_label,
                   meta.title,
                   rec.assessment.expected_drop_c);

            if (limit > 0 && count >= limit) break;
        }
    }
    fclose(fp);

    printf("-------------------------------------------------------------------------------------------------------------\n");
    printf("Total Micro-Zones Processed: %d\n", count);
    printf("Distribution: LOW = %d (%.1f%%) | MODERATE = %d (%.1f%%) | HIGH = %d (%.1f%%)\n",
           class_counts[0], count ? (class_counts[0]*100.0f/count) : 0,
           class_counts[1], count ? (class_counts[1]*100.0f/count) : 0,
           class_counts[2], count ? (class_counts[2]*100.0f/count) : 0);
    printf("Average Temperature Reduction: -%.2f C\n", count ? (total_drop / count) : 0.0f);
}

static void run_scenario_demo(void) {
    print_banner();
    printf("Running Scenario Comparison on High-Heat Commercial Zone (Majestic Hub Baseline):\n\n");

    uhi_features_t baseline = {
        .lst_c = 41.5f,
        .ambient_temp_c = 36.8f,
        .humidity_pct = 48.0f,
        .ndvi = 0.12f,
        .ndbi = 0.48f,
        .albedo = 0.14f,
        .building_density_pct = 78.0f,
        .tree_canopy_cover_pct = 8.0f,
        .traffic_density = 4200.0f,
        .population_density = 18500.0f
    };

    uhi_scenario_params_t scenarios[4] = {
        {
            .scenario_name = "Scenario A: High-Albedo Cool Roofs Only",
            .delta_canopy_pct = 0.0f,
            .delta_albedo = 0.35f,
            .green_roof_coverage_pct = 0.0f,
            .permeable_pavement_pct = 0.0f,
            .misting_coverage_pct = 0.0f,
            .target_zone_area_sqm = 100000.0f
        },
        {
            .scenario_name = "Scenario B: Miyawaki Urban Forestry + Tree Canopy",
            .delta_canopy_pct = 25.0f,
            .delta_albedo = 0.0f,
            .green_roof_coverage_pct = 0.0f,
            .permeable_pavement_pct = 0.0f,
            .misting_coverage_pct = 0.0f,
            .target_zone_area_sqm = 100000.0f
        },
        {
            .scenario_name = "Scenario C: Permeable Pavements + Water Misting Plazas",
            .delta_canopy_pct = 5.0f,
            .delta_albedo = 0.10f,
            .green_roof_coverage_pct = 10.0f,
            .permeable_pavement_pct = 40.0f,
            .misting_coverage_pct = 30.0f,
            .target_zone_area_sqm = 100000.0f
        },
        {
            .scenario_name = "Scenario D: Holistic Multi-Tier Integrated Mitigation",
            .delta_canopy_pct = 20.0f,
            .delta_albedo = 0.25f,
            .green_roof_coverage_pct = 20.0f,
            .permeable_pavement_pct = 30.0f,
            .misting_coverage_pct = 20.0f,
            .target_zone_area_sqm = 100000.0f
        }
    };

    uhi_scenario_comparison_t comp = uhi_compare_scenarios(&baseline, scenarios, 4);

    float base_utsi = uhi_compute_thermal_stress(&baseline);
    rf_prediction_t base_rf = uhi_rf_predict(&baseline);
    printf("BASELINE CONDITIONS: Ambient=%.1fC | LST=%.1fC | UTSI=%.1f | RF Class=%s (%.1f%%)\n",
           baseline.ambient_temp_c, baseline.lst_c, base_utsi, base_rf.class_label, base_rf.confidence * 100.0f);
    printf("=======================================================================================================\n");
    printf("%-40s | %-8s | %-8s | %-8s | %-12s | %-12s\n",
           "Scenario Name", "New Temp", "Temp Drop", "RF Tier", "Cost (INR)", "HVAC Savings");
    printf("-------------------------------------------------------------------------------------------------------\n");

    for (uint32_t i = 0; i < comp.scenario_count; i++) {
        uhi_scenario_result_t *r = &comp.results[i];
        printf("%-40.40s | %5.1fC   | -%4.1fC   | %-8s | %6.2f Lakhs | %5.1f%%\n",
               r->scenario_name,
               r->mitigated_features.ambient_temp_c,
               r->effective_temp_drop_c,
               r->mitigated_rf_prediction.class_label,
               r->total_investment_inr / 100000.0f,
               r->hvac_energy_savings_pct);
    }
    printf("=======================================================================================================\n");
    printf("★ Best Absolute Cooling: %s (-%.2f C)\n",
           comp.results[comp.best_cooling_scenario_idx].scenario_name,
           comp.results[comp.best_cooling_scenario_idx].effective_temp_drop_c);
    printf("★ Best ROI per Lakh INR: %s\n\n",
           comp.results[comp.best_roi_scenario_idx].scenario_name);
}

static void run_export_plan(const char *output_file) {
    FILE *fp = fopen(output_file, "w");
    if (!fp) {
        fprintf(stderr, "Error opening output file '%s'\n", output_file);
        return;
    }

    uhi_features_t zones[] = {
        {.lst_c = 42.1f, .ambient_temp_c = 37.2f, .humidity_pct = 45.0f, .ndvi = 0.10f, .ndbi = 0.52f, .albedo = 0.12f, .building_density_pct = 82.0f, .tree_canopy_cover_pct = 6.0f, .traffic_density = 4600.0f, .population_density = 21000.0f},
        {.lst_c = 36.5f, .ambient_temp_c = 33.4f, .humidity_pct = 52.0f, .ndvi = 0.28f, .ndbi = 0.32f, .albedo = 0.20f, .building_density_pct = 64.0f, .tree_canopy_cover_pct = 18.0f, .traffic_density = 2200.0f, .population_density = 13500.0f},
        {.lst_c = 24.5f, .ambient_temp_c = 26.8f, .humidity_pct = 68.0f, .ndvi = 0.76f, .ndbi = -0.42f, .albedo = 0.22f, .building_density_pct = 5.0f, .tree_canopy_cover_pct = 82.0f, .traffic_density = 180.0f, .population_density = 1200.0f}
    };
    const char *zone_names[] = {"Majestic Intermodal Transit Concourse", "Indiranagar 100ft Road Corridor", "Cubbon Park Ecological Reserve"};
    const char *zone_ids[] = {"MZ_004", "MZ_003", "MZ_001"};

    fprintf(fp, "{\n");
    fprintf(fp, "  \"system_name\": \"Smart Cooling Strategy Recommendation System for Urban Heat Island\",\n");
    fprintf(fp, "  \"export_type\": \"Municipal Heat Mitigation & Intervention Action Plan\",\n");
    fprintf(fp, "  \"generated_timestamp\": \"%ld\",\n", (long)time(NULL));
    fprintf(fp, "  \"micro_zones\": [\n");

    for (int i = 0; i < 3; i++) {
        uhi_assessment_t a = uhi_assess_zone(&zones[i]);
        rf_prediction_t rf = uhi_rf_predict(&zones[i]);
        uhi_strategy_meta_t m = uhi_get_strategy_meta(a.primary_recommendation);

        fprintf(fp, "    {\n");
        fprintf(fp, "      \"zone_id\": \"%s\",\n", zone_ids[i]);
        fprintf(fp, "      \"zone_name\": \"%s\",\n", zone_names[i]);
        fprintf(fp, "      \"lst_c\": %.2f,\n", zones[i].lst_c);
        fprintf(fp, "      \"ambient_temp_c\": %.2f,\n", zones[i].ambient_temp_c);
        fprintf(fp, "      \"thermal_stress_index\": %.2f,\n", a.thermal_stress_index);
        fprintf(fp, "      \"rf_class\": \"%s\",\n", rf.class_label);
        fprintf(fp, "      \"rf_confidence\": %.2f,\n", rf.confidence);
        fprintf(fp, "      \"primary_recommendation\": \"%s\",\n", m.title);
        fprintf(fp, "      \"category\": \"%s\",\n", m.category);
        fprintf(fp, "      \"expected_drop_c\": %.2f,\n", a.expected_drop_c);
        fprintf(fp, "      \"hvac_savings_pct\": %.2f,\n", a.hvac_savings_pct);
        fprintf(fp, "      \"cost_per_sqm_inr\": %.0f,\n", m.cost_per_sqm_inr);
        fprintf(fp, "      \"implementation_weeks\": %u,\n", m.implementation_weeks);
        fprintf(fp, "      \"reason\": \"%s\"\n", a.recommendation_reason);
        fprintf(fp, "    }%s\n", (i == 2) ? "" : ",");
    }

    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");
    fclose(fp);

    printf("Successfully generated Municipal Action Plan JSON: %s\n", output_file);
}

int main(int argc, char **argv) {
    if (argc >= 2 && strcmp(argv[1], "--scenario") == 0) {
        run_scenario_demo();
        return 0;
    }
    if (argc >= 3 && strcmp(argv[1], "--eval-csv") == 0) {
        int limit = (argc >= 4) ? atoi(argv[3]) : 0;
        run_eval_csv(argv[2], limit);
        return 0;
    }
    if (argc >= 3 && strcmp(argv[1], "--export-plan") == 0) {
        run_export_plan(argv[2]);
        return 0;
    }
    if (argc >= 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        print_usage(argv[0]);
        return 0;
    }

    int count = (argc >= 2) ? atoi(argv[1]) : 10;
    run_virtual_stream(count);
    return 0;
}