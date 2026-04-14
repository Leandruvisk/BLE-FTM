#include "routine_micro.h"
// #include "payload.h"
#include "protocol_ble.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ROUTINE";

EventGroupHandle_t system_events;

// Declarar as variáveis que vêm do protocol_ftm.c e do MAX30102
extern float g_last_ftm_distance;
extern int g_last_ftm_rtt;
extern int g_last_ftm_rssi;
extern float temp_heartrate;
float heartrate_now = 0; 

static void routine_task(void *pvParameters)
{
    while (1) {
        xEventGroupWaitBits(
            system_events,
            EVT_FTM_READY, 
            pdTRUE,
            pdFALSE,
            portMAX_DELAY
        );

        uint8_t payload[32]; // Reduzido, pois a string será bem curta
        memset(payload, 0, sizeof(payload));

        int dist_cm = (int)g_last_ftm_distance; 
        heartrate_now = temp_heartrate;

        // Enviando apenas os valores: RTT, RSSI, DIST, BPM
        // Exemplo de saída: "14,-30,210,88"
        int len = snprintf((char *)payload, sizeof(payload), 
                           "%d,%d,%d,%d", 
                           (int)g_last_ftm_rtt, 
                           (int)g_last_ftm_rssi, 
                           dist_cm,
                           (int)temp_heartrate);

        ESP_LOGI("PAYLOAD_DEBUG", "String gerada: %s", (char *)payload);

        // Importante: Não zere a temp_heartrate aqui se for usar na calibração!
        heartrate_now = 0; 

        if (len > 0) {
            ble_send_data(payload, len);
        }
    }
}

void routine_micro_start(void)
{
    system_events = xEventGroupCreate();
    xTaskCreate(routine_task, "routine_task", 4096, NULL, 5, NULL);
}