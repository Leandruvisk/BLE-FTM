#ifndef NVS_PARTICIONS_H
#define NVS_PARTICIONS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "esp_log.h"

esp_err_t init_nvs(void);

typedef struct {
    int hr_max;
    int hr_min;
} calibration_data_t;

#endif // NVS_PARTICIONS_H