/*
 * ==============================================================================
 * Project: Smart Cooling Strategy Recommendation System for Urban Heat Island (UHI)
 * Component: ESP32 Micro-Climate Sensing Edge Node Firmware
 * Language: Pure Embedded C (ISO C99 / ESP-IDF FreeRTOS Native Driver)
 * Target Hardware: ESP32 DevKit V1 (Xtensa dual-core 32-bit LX6 MCU)
 * Sensors: DHT22 / AM2302 (Digital 1-Wire Temperature & Relative Humidity)
 * Display: SSD1306 0.96" I2C Monochrome OLED (128x64 Pixels, Address 0x3C)
 * Actuators: Tri-Color LED Severity Bank & Active Piezo Acoustic Buzzer
 * ==============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "rom/ets_sys.h"
#include "uhi_engine.h"

#define TAG "UHI_EDGE_NODE"

/* -------------------------------------------------------------------------- */
/* GPIO Pin Configurations                                                    */
/* -------------------------------------------------------------------------- */
#define DHT22_GPIO_PIN        GPIO_NUM_4    /* 1-Wire Data Line (10k pull-up) */
#define LED_GREEN_PIN         GPIO_NUM_18   /* Safe / Low Heat Status LED     */
#define LED_YELLOW_PIN        GPIO_NUM_19   /* Caution / Moderate Heat LED   */
#define LED_RED_PIN           GPIO_NUM_23   /* Critical / High Heat Alert LED */
#define BUZZER_PIN            GPIO_NUM_5    /* Piezo Buzzer Active Alarm      */

#define I2C_MASTER_SDA_IO     GPIO_NUM_21   /* OLED I2C SDA Line              */
#define I2C_MASTER_SCL_IO     GPIO_NUM_22   /* OLED I2C SCL Line              */
#define I2C_MASTER_NUM        I2C_NUM_0     /* I2C Port Number                */
#define I2C_MASTER_FREQ_HZ    400000        /* 400kHz Fast Mode               */
#define SSD1306_I2C_ADDR      0x3C          /* OLED 7-bit I2C Address         */

/* -------------------------------------------------------------------------- */
/* Threshold Definitions & Sample Rates                                       */
/* -------------------------------------------------------------------------- */
#define THRESHOLD_MODERATE_C  31.0f         /* Moderate Heat Island Tier (°C) */
#define THRESHOLD_HIGH_C      36.5f         /* High Thermal Stress Tier (°C)  */
#define THRESHOLD_BUZZER_C    38.0f         /* Acoustic Alarm Trigger (°C)    */
#define SAMPLE_INTERVAL_MS    2000          /* 2.0s Sampling Interval (0.5Hz) */

/* -------------------------------------------------------------------------- */
/* Data Structures                                                            */
/* -------------------------------------------------------------------------- */
typedef enum {
    SEVERITY_LOW = 0,
    SEVERITY_MODERATE,
    SEVERITY_HIGH,
    SEVERITY_SENSOR_FAULT
} heat_severity_t;

typedef struct {
    float temperature_c;
    float relative_humidity_pct;
    float heat_index_c;
    heat_severity_t severity;
    bool  is_valid;
    uint32_t packet_id;
    int64_t  uptime_ms;
} sensor_telemetry_t;

/* -------------------------------------------------------------------------- */
/* Low-Level 1-Wire DHT22 Driver in Embedded C                                */
/* -------------------------------------------------------------------------- */
static inline int wait_for_gpio_level(gpio_num_t pin, int level, uint32_t timeout_us) {
    uint32_t micros = 0;
    while (gpio_get_level(pin) != level) {
        if (++micros > timeout_us) return -1;
        ets_delay_us(1);
    }
    return micros;
}

static bool dht22_read_raw(gpio_num_t pin, float *out_temp, float *out_hum) {
    uint8_t data[5] = {0, 0, 0, 0, 0};

    /* Step 1: Host MCU sends Start Signal */
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);                 /* Pull low for >= 1.1ms */
    ets_delay_us(1200);
    gpio_set_level(pin, 1);                 /* Pull high for 30us    */
    ets_delay_us(30);

    /* Step 2: Switch to Input Mode to listen for DHT response */
    gpio_set_direction(pin, GPIO_MODE_INPUT);

    /* Step 3: Wait for DHT22 acknowledgment (80us Low -> 80us High) */
    if (wait_for_gpio_level(pin, 0, 85) < 0) return false;
    if (wait_for_gpio_level(pin, 1, 85) < 0) return false;
    if (wait_for_gpio_level(pin, 0, 85) < 0) return false;

    /* Step 4: Read 40 Data Bits (5 Bytes: Hum MSB/LSB, Temp MSB/LSB, CRC) */
    for (int i = 0; i < 40; i++) {
        if (wait_for_gpio_level(pin, 1, 65) < 0) return false;
        
        /* Measure duration of high pulse: ~26-28us = bit 0, ~70us = bit 1 */
        uint32_t high_duration = 0;
        while (gpio_get_level(pin) == 1) {
            if (++high_duration > 100) return false;
            ets_delay_us(1);
        }

        uint8_t byte_idx = i / 8;
        data[byte_idx] <<= 1;
        if (high_duration > 40) {
            data[byte_idx] |= 1;
        }
    }

    /* Step 5: Checksum Validation */
    uint8_t checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
    if (checksum != data[4]) {
        ESP_LOGW(TAG, "DHT22 Checksum Mismatch! Calc: 0x%02X vs Received: 0x%02X", checksum, data[4]);
        return false;
    }

    /* Step 6: Convert 16-bit registers into engineering units */
    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = (((uint16_t)data[2] & 0x7F) << 8) | data[3];

    *out_hum = (float)raw_hum / 10.0f;
    *out_temp = (float)raw_temp / 10.0f;
    if (data[2] & 0x80) {
        *out_temp = -(*out_temp); /* Sub-zero Celsius representation */
    }

    /* Sanity boundary check */
    if (*out_temp < -20.0f || *out_temp > 60.0f || *out_hum < 0.0f || *out_hum > 100.0f) {
        return false;
    }

    return true;
}

/* -------------------------------------------------------------------------- */
/* Rothfusz Heat Index Regression Calculation (NOAA / Rothfusz Equation)     */
/* -------------------------------------------------------------------------- */
static float compute_heat_index(float temp_c, float rh) {
    /* Convert Celsius to Fahrenheit for empirical equation */
    float T = (temp_c * 1.8f) + 32.0f;
    float R = rh;

    /* Steadman simple equation for mild conditions */
    float hi_f = 0.5f * (T + 61.0f + ((T - 68.0f) * 1.2f) + (R * 0.094f));

    if (hi_f >= 80.0f) {
        /* Full Rothfusz polynomial regression */
        hi_f = -42.379f + (2.04901523f * T) + (10.14333127f * R)
               - (0.22475541f * T * R) - (0.00683783f * T * T)
               - (0.05481717f * R * R) + (0.00122874f * T * T * R)
               + (0.00085282f * T * R * R) - (0.00000199f * T * T * R * R);

        /* Humidity adjustments */
        if (R < 13.0f && T >= 80.0f && T <= 112.0f) {
            hi_f -= ((13.0f - R) / 4.0f) * sqrtf((17.0f - fabsf(T - 95.0f)) / 17.0f);
        } else if (R > 85.0f && T >= 80.0f && T <= 87.0f) {
            hi_f += ((R - 85.0f) / 10.0f) * ((87.0f - T) / 5.0f);
        }
    }

    /* Convert back to Celsius */
    return (hi_f - 32.0f) / 1.8f;
}

/* -------------------------------------------------------------------------- */
/* Low-Level SSD1306 OLED Monochrome I2C Driver in Embedded C                 */
/* -------------------------------------------------------------------------- */
#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  64
static uint8_t s_oled_buffer[SSD1306_WIDTH * (SSD1306_HEIGHT / 8)];

/* Embedded Basic 5x7 ASCII Font Bitmap Table (Subset) */
static const uint8_t s_font5x7[][5] = {
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00},
    ['-'] = {0x08, 0x08, 0x08, 0x08, 0x08},
    ['.'] = {0x00, 0x60, 0x60, 0x00, 0x00},
    ['/'] = {0x20, 0x10, 0x08, 0x04, 0x02},
    ['0'] = {0x3E, 0x51, 0x49, 0x45, 0x3E},
    ['1'] = {0x00, 0x42, 0x7F, 0x40, 0x00},
    ['2'] = {0x42, 0x61, 0x51, 0x49, 0x46},
    ['3'] = {0x21, 0x41, 0x45, 0x4B, 0x31},
    ['4'] = {0x18, 0x14, 0x12, 0x7F, 0x10},
    ['5'] = {0x27, 0x45, 0x45, 0x45, 0x39},
    ['6'] = {0x3C, 0x4A, 0x49, 0x49, 0x30},
    ['7'] = {0x01, 0x71, 0x09, 0x05, 0x03},
    ['8'] = {0x36, 0x49, 0x49, 0x49, 0x36},
    ['9'] = {0x06, 0x49, 0x49, 0x29, 0x1E},
    [':'] = {0x00, 0x36, 0x36, 0x00, 0x00},
    ['='] = {0x14, 0x14, 0x14, 0x14, 0x14},
    ['['] = {0x00, 0x7F, 0x41, 0x41, 0x00},
    [']'] = {0x00, 0x41, 0x41, 0x7F, 0x00},
    ['%'] = {0x23, 0x13, 0x08, 0x64, 0x62},
    ['A'] = {0x7C, 0x12, 0x11, 0x12, 0x7C},
    ['B'] = {0x7F, 0x49, 0x49, 0x49, 0x36},
    ['C'] = {0x3E, 0x41, 0x41, 0x41, 0x22},
    ['D'] = {0x7F, 0x41, 0x41, 0x22, 0x1C},
    ['E'] = {0x7F, 0x49, 0x49, 0x49, 0x41},
    ['F'] = {0x7F, 0x09, 0x09, 0x09, 0x01},
    ['G'] = {0x3E, 0x41, 0x49, 0x49, 0x7A},
    ['H'] = {0x7F, 0x08, 0x08, 0x08, 0x7F},
    ['I'] = {0x00, 0x41, 0x7F, 0x41, 0x00},
    ['K'] = {0x7F, 0x08, 0x14, 0x22, 0x41},
    ['L'] = {0x7F, 0x40, 0x40, 0x40, 0x40},
    ['M'] = {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    ['N'] = {0x7F, 0x04, 0x08, 0x10, 0x7F},
    ['O'] = {0x3E, 0x41, 0x41, 0x41, 0x3E},
    ['P'] = {0x7F, 0x09, 0x09, 0x09, 0x06},
    ['R'] = {0x7F, 0x09, 0x19, 0x29, 0x46},
    ['S'] = {0x46, 0x49, 0x49, 0x49, 0x31},
    ['T'] = {0x01, 0x01, 0x7F, 0x01, 0x01},
    ['U'] = {0x3F, 0x40, 0x40, 0x40, 0x3F},
    ['V'] = {0x1F, 0x20, 0x40, 0x20, 0x1F},
    ['W'] = {0x7F, 0x20, 0x18, 0x20, 0x7F},
    ['X'] = {0x63, 0x14, 0x08, 0x14, 0x63},
    ['Y'] = {0x07, 0x08, 0x70, 0x08, 0x07},
    ['Z'] = {0x61, 0x51, 0x49, 0x45, 0x43},
    ['!'] = {0x00, 0x00, 0x5F, 0x00, 0x00}
};

static esp_err_t ssd1306_write_cmd(uint8_t cmd) {
    i2c_cmd_handle_t link = i2c_cmd_link_create();
    i2c_master_start(link);
    i2c_master_write_byte(link, (SSD1306_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(link, 0x00, true); /* Control Byte: 0x00 = Command */
    i2c_master_write_byte(link, cmd, true);
    i2c_master_stop(link);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, link, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(link);
    return ret;
}

static void ssd1306_init(void) {
    ssd1306_write_cmd(0xAE); /* Display OFF */
    ssd1306_write_cmd(0xD5); /* Set Display Clock Divide Ratio */
    ssd1306_write_cmd(0x80);
    ssd1306_write_cmd(0xA8); /* Set Multiplex Ratio */
    ssd1306_write_cmd(0x3F); /* 1/64 duty */
    ssd1306_write_cmd(0xD3); /* Set Display Offset */
    ssd1306_write_cmd(0x00);
    ssd1306_write_cmd(0x40); /* Set Start Line to 0 */
    ssd1306_write_cmd(0x8D); /* Charge Pump Setting */
    ssd1306_write_cmd(0x14); /* Enable Charge Pump */
    ssd1306_write_cmd(0x20); /* Memory Addressing Mode */
    ssd1306_write_cmd(0x00); /* Horizontal Addressing */
    ssd1306_write_cmd(0xA1); /* Segment Re-map */
    ssd1306_write_cmd(0xC8); /* COM Output Scan Direction */
    ssd1306_write_cmd(0xDA); /* Set COM Pins Hardware Config */
    ssd1306_write_cmd(0x12);
    ssd1306_write_cmd(0x81); /* Contrast Control */
    ssd1306_write_cmd(0xCF);
    ssd1306_write_cmd(0xD9); /* Set Pre-charge Period */
    ssd1306_write_cmd(0xF1);
    ssd1306_write_cmd(0xDB); /* Set VCOMH Deselect Level */
    ssd1306_write_cmd(0x40);
    ssd1306_write_cmd(0xA4); /* Entire Display ON resume */
    ssd1306_write_cmd(0xA6); /* Normal Display */
    ssd1306_write_cmd(0xAF); /* Display ON */
}

static void oled_clear_buffer(void) {
    memset(s_oled_buffer, 0x00, sizeof(s_oled_buffer));
}

static void oled_draw_char(uint8_t x, uint8_t y_page, char c) {
    if (c < 32 || c > 126 || x + 5 >= SSD1306_WIDTH || y_page >= 8) return;
    const uint8_t *bitmap = s_font5x7[(uint8_t)c];
    for (int col = 0; col < 5; col++) {
        s_oled_buffer[y_page * SSD1306_WIDTH + (x + col)] = bitmap[col];
    }
}

static void oled_draw_string(uint8_t x, uint8_t y_page, const char *str) {
    while (*str && x < (SSD1306_WIDTH - 6)) {
        oled_draw_char(x, y_page, *str++);
        x += 6;
    }
}

static void oled_update_screen(void) {
    i2c_cmd_handle_t link = i2c_cmd_link_create();
    i2c_master_start(link);
    i2c_master_write_byte(link, (SSD1306_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(link, 0x40, true); /* Control Byte: 0x40 = Data stream */
    i2c_master_write(link, s_oled_buffer, sizeof(s_oled_buffer), true);
    i2c_master_stop(link);
    i2c_master_cmd_begin(I2C_MASTER_NUM, link, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(link);
}

/* -------------------------------------------------------------------------- */
/* Hardware Actuator & GPIO Drivers                                          */
/* -------------------------------------------------------------------------- */
static void actuators_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GREEN_PIN) | (1ULL << LED_YELLOW_PIN) |
                        (1ULL << LED_RED_PIN)   | (1ULL << BUZZER_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    gpio_set_level(LED_GREEN_PIN, 0);
    gpio_set_level(LED_YELLOW_PIN, 0);
    gpio_set_level(LED_RED_PIN, 0);
    gpio_set_level(BUZZER_PIN, 0);
}

static void set_led_severity(heat_severity_t sev) {
    switch (sev) {
        case SEVERITY_LOW:
            gpio_set_level(LED_GREEN_PIN, 1);
            gpio_set_level(LED_YELLOW_PIN, 0);
            gpio_set_level(LED_RED_PIN, 0);
            gpio_set_level(BUZZER_PIN, 0);
            break;
        case SEVERITY_MODERATE:
            gpio_set_level(LED_GREEN_PIN, 0);
            gpio_set_level(LED_YELLOW_PIN, 1);
            gpio_set_level(LED_RED_PIN, 0);
            gpio_set_level(BUZZER_PIN, 0);
            break;
        case SEVERITY_HIGH:
            gpio_set_level(LED_GREEN_PIN, 0);
            gpio_set_level(LED_YELLOW_PIN, 0);
            gpio_set_level(LED_RED_PIN, 1);
            break;
        default:
            /* Fault: All flash briefly */
            gpio_set_level(LED_GREEN_PIN, 0);
            gpio_set_level(LED_YELLOW_PIN, 0);
            gpio_set_level(LED_RED_PIN, 0);
            gpio_set_level(BUZZER_PIN, 0);
            break;
    }
}

/* -------------------------------------------------------------------------- */
/* I2C Bus Initialization                                                     */
/* -------------------------------------------------------------------------- */
static void i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

/* -------------------------------------------------------------------------- */
/* Telemetry FreeRTOS Task in Pure Embedded C                                 */
/* -------------------------------------------------------------------------- */
void uhi_telemetry_task(void *pvParameters) {
    uint32_t packet_counter = 0;
    char line_buf[32];
    char json_buffer[256];

    ESP_LOGI(TAG, "UHI Telemetry Task running at 0.5Hz on FreeRTOS Core %d", xPortGetCoreID());

    while (1) {
        sensor_telemetry_t tele;
        tele.packet_id = ++packet_counter;
        tele.uptime_ms = esp_timer_get_time() / 1000;
        tele.is_valid = dht22_read_raw(DHT22_GPIO_PIN, &tele.temperature_c, &tele.relative_humidity_pct);

        oled_clear_buffer();

        if (tele.is_valid) {
            tele.heat_index_c = compute_heat_index(tele.temperature_c, tele.relative_humidity_pct);

            /* Classify Severity Tier */
            if (tele.temperature_c >= THRESHOLD_HIGH_C || tele.heat_index_c >= 38.0f) {
                tele.severity = SEVERITY_HIGH;
            } else if (tele.temperature_c >= THRESHOLD_MODERATE_C || tele.heat_index_c >= 33.0f) {
                tele.severity = SEVERITY_MODERATE;
            } else {
                tele.severity = SEVERITY_LOW;
            }

            /* Update Actuators */
            set_led_severity(tele.severity);

            /* Acoustic Buzzer trigger for extreme temperature */
            if (tele.temperature_c >= THRESHOLD_BUZZER_C) {
                gpio_set_level(BUZZER_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(100));
                gpio_set_level(BUZZER_PIN, 0);
            }

            uhi_features_t features = {
                .lst_c = tele.temperature_c + 8.0f,
                .ambient_temp_c = tele.temperature_c,
                .humidity_pct = tele.relative_humidity_pct,
                .ndvi = 0.20f,
                .ndbi = 0.35f,
                .albedo = 0.25f,
                .building_density_pct = 70.0f,
                .tree_canopy_cover_pct = 15.0f,
                .traffic_density = 0.0f
            };
            uhi_assessment_t assessment = uhi_assess_zone(&features);

            /* Emit a self-contained serial contract for embedded consumers. */
            const char *sev_str = (tele.severity == SEVERITY_HIGH) ? "HIGH" :
                                  (tele.severity == SEVERITY_MODERATE) ? "MODERATE" : "LOW";

            snprintf(json_buffer, sizeof(json_buffer),
                "{\"packet_id\":%lu,\"node_id\":\"ESP32_UHI_NODE_C\",\"ambient_temp_c\":%.2f,"
                "\"relative_humidity_pct\":%.2f,\"heat_index_c\":%.2f,\"severity\":\"%s\","
                "\"thermal_stress_index\":%.2f,\"expected_drop_c\":%.2f,\"hvac_savings_pct\":%.2f,"
                "\"recommendation\":\"%s\",\"uptime_ms\":%lld}\n",
                tele.packet_id, tele.temperature_c, tele.relative_humidity_pct, tele.heat_index_c, sev_str,
                assessment.thermal_stress_index, assessment.expected_drop_c,
                assessment.hvac_savings_pct, uhi_recommendation_name(assessment.recommendation), tele.uptime_ms);

            printf("%s", json_buffer);

            /* Render to OLED Framebuffer */
            oled_draw_string(4, 0, "== UHI MICRO-NODE ==");
            
            snprintf(line_buf, sizeof(line_buf), "TEMP: %.1f C", tele.temperature_c);
            oled_draw_string(4, 2, line_buf);

            snprintf(line_buf, sizeof(line_buf), "HUM : %.1f %%", tele.relative_humidity_pct);
            oled_draw_string(4, 4, line_buf);

            snprintf(line_buf, sizeof(line_buf), "HEAT: %.1f C", tele.heat_index_c);
            oled_draw_string(4, 5, line_buf);

            snprintf(line_buf, sizeof(line_buf), "STATUS: [%s]", sev_str);
            oled_draw_string(4, 7, line_buf);

        } else {
            tele.severity = SEVERITY_SENSOR_FAULT;
            set_led_severity(SEVERITY_SENSOR_FAULT);

            printf("{\"packet_id\":%lu,\"error\":\"DHT22 Read Fault / Checksum Timeout\"}\n", tele.packet_id);

            oled_draw_string(10, 2, "SENSOR ERROR!");
            oled_draw_string(10, 4, "Check DHT22 Wire");
        }

        oled_update_screen();

        /* Non-blocking FreeRTOS delay */
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }
}

/* -------------------------------------------------------------------------- */
/* Embedded C Application Entry Point                                         */
/* -------------------------------------------------------------------------- */
void app_main(void) {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, " Smart Cooling Strategy - ESP32 Native C Firmware ");
    ESP_LOGI(TAG, "==================================================");

    /* 1. Initialize Hardware Subsystems */
    actuators_init();
    i2c_master_init();
    ssd1306_init();

    /* 2. Display Startup Splash */
    oled_clear_buffer();
    oled_draw_string(10, 1, "UHI SENSING NODE");
    oled_draw_string(10, 3, "Native C Firmware");
    oled_draw_string(10, 5, "Connecting Sensors");
    oled_update_screen();
    vTaskDelay(pdMS_TO_TICKS(1500));

    /* 3. Launch FreeRTOS Real-Time Telemetry Task */
    xTaskCreatePinnedToCore(
        uhi_telemetry_task,
        "uhi_telemetry",
        4096,
        NULL,
        5,
        NULL,
        1 /* Pin to Core 1 */
    );
}
