#include "routine_micro.h"
#include "protocol_ble.h"
#include "protocol_ftm.h"
#include "calibration_max30102.h" // Importante para ler os limites da NVS
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ROUTINE";
EventGroupHandle_t system_events;

extern float g_last_ftm_distance;
extern int g_last_ftm_rtt;
extern int g_last_ftm_rssi;
extern float temp_heartrate;

// Função externa que dispara o FTM no seu protocol_ftm.c
// extern void wifi_ftm_start_session(); 

static void routine_task(void *pvParameters)
{
    int32_t r_max = 0, r_min = 0;
    
    while (1) {
        // 1. Carrega os limites de calibração salvos (Resting)

        ESP_LOGI(TAG, "Monitorando... HR: %.1f | Limite: %ld", temp_heartrate, r_max);
        
        if (r_max == 0) {
            get_last_calibration_results(CALIB_RESTING, &r_max, &r_min);
        }

        // 2. Lógica de Inferência: Se sair do descanso, força o FTM
        // Usamos uma margem de 5 BPM acima do seu r_max calibrado (92 + 5)
        if (temp_heartrate > (r_max + 5) && temp_heartrate < 220) {
            ESP_LOGW(TAG, "Detectada atividade! HR: %.1f > Limite: %ld. Disparando FTM...", temp_heartrate, r_max);
            
            // Chama a função que inicia o FTM (ajuste o nome para a sua função real)
            ftm_measure();

            // 3. Espera o FTM concluir (EVT_FTM_READY) para coletar os dados
            xEventGroupWaitBits(
                system_events,
                EVT_FTM_READY, 
                pdTRUE,
                pdFALSE,
                pdMS_TO_TICKS(3000) // Timeout de 3s para não travar a task
            );
        } else {
            // Se estiver em descanso, apenas espera um FTM periódico ou aguarda
            ESP_LOGI(TAG, "Em repouso... HR: %.1f", temp_heartrate);
            vTaskDelay(pdMS_TO_TICKS(2000)); 
            continue; // Volta para o início do loop
        }

        // 4. Montagem do Payload enxuto para o BLE
        uint8_t payload[32];
        memset(payload, 0, sizeof(payload));

        int dist_cm = (int)g_last_ftm_distance; 

        int len = snprintf((char *)payload, sizeof(payload), 
                           "%d,%d,%d,%d", 
                           (int)g_last_ftm_rtt, 
                           (int)g_last_ftm_rssi, 
                           dist_cm,
                           (int)temp_heartrate);

        ESP_LOGI("PAYLOAD_DEBUG", "String gerada: %s", (char *)payload);

        if (len > 0) {
            ble_send_data(payload, len);
        }

        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}

void routine_micro_start(void)
{
    system_events = xEventGroupCreate();
    xTaskCreate(routine_task, "routine_task", 4096, NULL, 5, NULL);
}