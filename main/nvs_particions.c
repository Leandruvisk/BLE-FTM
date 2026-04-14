#include "nvs_particions.h"
#include "calibration_max30102.h"
#include "max30102.h"

esp_err_t ret;
esp_err_t init_nvs()
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    return ret;
}

static const char *TAG = "CALIB";
extern float heartrate; // Variável global atualizada pela max30102_task

void realizar_calibracao(const char* nvs_key) {
    calibration_data_t data = {.hr_max = 0, .hr_min = 250};
    int64_t start_time = esp_timer_get_time();
    
    ESP_LOGI(TAG, "Iniciando calibração de 5 segundos para chave: %s", nvs_key);
    
    // Loop de 5 segundos
    while ((esp_timer_get_time() - start_time) < 5000000) {
        // NÃO chame read_max30102() aqui! 
        // A max30102_task já está fazendo isso em paralelo.
        
        int hr_atual = (int)heartrate; 

        if (hr_atual > 40 && hr_atual < 200) { // Filtro de valores plausíveis
            if (hr_atual > data.hr_max) data.hr_max = hr_atual;
            if (hr_atual < data.hr_min) data.hr_min = hr_atual;
        }
        
        // Delay curto para permitir que outras tasks rodem
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }

    // Salva na NVS
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        nvs_set_i32(my_handle, nvs_key, (int32_t)data.hr_max);
        // Opcional: nvs_set_i32(my_handle, "nvs_key_min", (int32_t)data.hr_min);
        
        nvs_commit(my_handle);
        nvs_close(my_handle);
        
        ESP_LOGW(TAG, "CALIBRAÇÃO CONCLUÍDA -> Chave: %s | Max: %d | Min: %d", 
                 nvs_key, data.hr_max, data.hr_min);
        
        // Teste de leitura imediata para confirmar
        int32_t check_val = 0;
        nvs_open("storage", NVS_READONLY, &my_handle);
        nvs_get_i32(my_handle, nvs_key, &check_val);
        nvs_close(my_handle);
        ESP_LOGI(TAG, "Confirmação da Flash: %ld", check_val);
    }
}