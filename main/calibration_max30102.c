#include "calibration_max30102.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern int heartrate; 
static const char *TAG = "CALIB_MAX";

/**
 * @brief Função para ler os valores da Flash (Teste de integridade)
 */
void get_calibration_data(const char* k_max, const char* k_min) {
    nvs_handle_t handle;
    int32_t val_max = 0;
    int32_t val_min = 0;

    esp_err_t err = nvs_open("storage", NVS_READONLY, &handle);
    if (err == ESP_OK) {
        nvs_get_i32(handle, k_max, &val_max);
        nvs_get_i32(handle, k_min, &val_min);
        nvs_close(handle);

        ESP_LOGW(TAG, "VERIFICAÇÃO FLASH -> [%s]: %ld | [%s]: %ld", k_max, val_max, k_min, val_min);
    } else {
        ESP_LOGE(TAG, "Erro ao abrir NVS para leitura: %s", esp_err_to_name(err));
    }
}

void start_calibration_process(calib_state_t state) {
    int32_t hr_max = 0;
    int32_t hr_min = 250;
    char *k_max;
    char *k_min;

    switch (state) {
        case CALIB_RESTING:  k_max = "rest_max";  k_min = "rest_min";  break;
        case CALIB_STANDING: k_max = "stand_max"; k_min = "stand_min"; break;
        case CALIB_WALKING:  k_max = "walk_max";  k_min = "walk_min";  break;
        default: return;
    }

    ESP_LOGI(TAG, "Iniciando janela de 5s para %s/%s", k_max, k_min);

    int64_t start_time = esp_timer_get_time();
    
    while ((esp_timer_get_time() - start_time) < 5000000) {
        int hr_sample = heartrate; 
        
        if (hr_sample > 40 && hr_sample < 220) {
            if (hr_sample > hr_max) hr_max = hr_sample;
            if (hr_sample < hr_min) hr_min = hr_sample;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        nvs_set_i32(handle, k_max, hr_max);
        nvs_set_i32(handle, k_min, hr_min);
        nvs_commit(handle);
        nvs_close(handle);
        
        ESP_LOGI(TAG, "Dados enviados para a Flash.");

        // --- TESTE DE VERIFICAÇÃO IMEDIATA ---
        get_calibration_data(k_max, k_min); 
    } else {
        ESP_LOGE(TAG, "Erro ao abrir NVS: %s", esp_err_to_name(err));
    }
}

bool get_last_calibration_results(calib_state_t state, int32_t *max, int32_t *min) 
{
    nvs_handle_t handle;
    char *k_max = (state == 1) ? "rest_max" : (state == 2) ? "stand_max" : "walk_max";
    char *k_min = (state == 1) ? "rest_min" : (state == 2) ? "stand_min" : "walk_min";

    if (nvs_open("storage", NVS_READONLY, &handle) == ESP_OK) {
        nvs_get_i32(handle, k_max, max);
        nvs_get_i32(handle, k_min, min);
        nvs_close(handle);
        return true;
    }
    return false;
}