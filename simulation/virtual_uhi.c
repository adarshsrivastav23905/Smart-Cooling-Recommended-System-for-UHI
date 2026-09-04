#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "../esp32_firmware/src/uhi_engine.h"

static const char *severity_name(float temperature_c, float heat_index_c) {
    if (temperature_c >= 36.5f || heat_index_c >= 38.0f) return "HIGH";
    if (temperature_c >= 31.0f || heat_index_c >= 33.0f) return "MODERATE";
    return "LOW";
}

static void emit_packet(unsigned long packet_id, float temperature_c, float humidity_pct) {
    float heat_index_c = temperature_c + (humidity_pct * 0.10f);
    const char *severity = severity_name(temperature_c, heat_index_c);
    uhi_features_t features = {
        .lst_c = temperature_c + 8.0f,
        .ambient_temp_c = temperature_c,
        .humidity_pct = humidity_pct,
        .ndvi = 0.20f,
        .ndbi = 0.35f,
        .albedo = 0.25f,
        .building_density_pct = 70.0f,
        .tree_canopy_cover_pct = 15.0f,
        .traffic_density = 0.0f
    };
    uhi_assessment_t assessment = uhi_assess_zone(&features);

    printf("{\"packet_id\":%lu,\"node_id\":\"VIRTUAL_UHI_NODE_C\","
           "\"ambient_temp_c\":%.2f,\"relative_humidity_pct\":%.2f,"
           "\"heat_index_c\":%.2f,\"severity\":\"%s\","
           "\"green_led\":%s,\"yellow_led\":%s,\"red_led\":%s,"
           "\"buzzer\":%s,\"thermal_stress_index\":%.2f,"
           "\"expected_drop_c\":%.2f,\"hvac_savings_pct\":%.2f,"
           "\"recommendation\":\"%s\"}\n",
           packet_id, temperature_c, humidity_pct, heat_index_c, severity,
           (severity[0] == 'L') ? "true" : "false",
           (severity[0] == 'M') ? "true" : "false",
           (severity[0] == 'H') ? "true" : "false",
           (temperature_c >= 38.0f) ? "true" : "false",
           assessment.thermal_stress_index, assessment.expected_drop_c,
           assessment.hvac_savings_pct,
           uhi_recommendation_name(assessment.recommendation));
}

int main(int argc, char **argv) {
    int packet_count = 10;
    if (argc > 1) {
        packet_count = atoi(argv[1]);
        if (packet_count < 1 || packet_count > 1000) return EXIT_FAILURE;
    }

    for (int packet = 0; packet < packet_count; packet++) {
        float phase = (float)packet * 0.55f;
        float temperature_c = 32.0f + (4.5f * sinf(phase));
        float humidity_pct = 58.0f - (12.0f * sinf(phase));
        emit_packet((unsigned long)(packet + 1), temperature_c, humidity_pct);
    }
    return EXIT_SUCCESS;
}