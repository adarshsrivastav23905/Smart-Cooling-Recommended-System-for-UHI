#ifndef UHI_RANDOM_FOREST_H
#define UHI_RANDOM_FOREST_H

#include <stdint.h>
#include <stdbool.h>
#include "uhi_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RF_NUM_TREES 25
#define RF_NUM_CLASSES 3
#define RF_NUM_FEATURES 10

typedef enum {
    RF_CLASS_LOW = 0,
    RF_CLASS_MODERATE = 1,
    RF_CLASS_HIGH = 2
} rf_class_t;

typedef struct {
    rf_class_t predicted_class;
    const char *class_label;
    float probabilities[RF_NUM_CLASSES]; /* Softmax probability for Low, Moderate, High */
    float confidence;                   /* Highest class probability (0.0 to 1.0) */
    uint16_t tree_votes[RF_NUM_CLASSES];/* Distribution of tree votes */
} rf_prediction_t;

typedef struct {
    const char *feature_names[RF_NUM_FEATURES];
    float importances[RF_NUM_FEATURES];
} rf_feature_importance_t;

/**
 * @brief Evaluates the Random Forest ensemble on the input feature vector.
 * @param features Pointer to the 10-parameter micro-climate feature vector.
 * @return rf_prediction_t Prediction result with class, probabilities, and votes.
 */
rf_prediction_t uhi_rf_predict(const uhi_features_t *features);

/**
 * @brief Returns feature importance weights computed via Gini impurity reduction.
 */
rf_feature_importance_t uhi_rf_get_feature_importances(void);

/**
 * @brief Returns human-readable label for a given RF class.
 */
const char *uhi_rf_class_name(rf_class_t rf_class);

#ifdef __cplusplus
}
#endif

#endif /* UHI_RANDOM_FOREST_H */
