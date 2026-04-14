#ifndef CALIBRATION_MAX30102_H
#define CALIBRATION_MAX30102_H

#include <stdint.h>
#include <stdbool.h>

// Definição dos estados para calibração
typedef enum {
    CALIB_RESTING = 1,
    CALIB_STANDING = 2,
    CALIB_WALKING = 3
} calib_state_t;

/**
 * @brief Inicia o processo de amostragem de 5s e salva na NVS
 * @param state Estado vindo do comando BLE
 */
void start_calibration_process(calib_state_t state);
bool get_last_calibration_results(calib_state_t state, int32_t *max, int32_t *min);
void get_calibration_data(const char* k_max, const char* k_min);



#endif