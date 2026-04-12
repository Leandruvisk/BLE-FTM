#include "routine_micro.h"
#include "payload.h"
#include "protocol_ble.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "payload.h"

static const char *TAG = "ROUTINE";

EventGroupHandle_t system_events;

static void routine_task(void *pvParameters)
{
    while (1) {

        xEventGroupWaitBits(
            system_events,
            EVT_SENSOR_READY | EVT_FTM_READY,
            pdTRUE,
            pdTRUE,
            portMAX_DELAY
        );

        ESP_LOGI(TAG, "Dados prontos -> enviando via BLE");

        uint8_t payload[50];

        build_payload(payload);

        ble_send_data(payload, sizeof(payload));

    }
}

void routine_micro_start(void)
{
    system_events = xEventGroupCreate();

    xTaskCreate(routine_task, "routine_task", 4096, NULL, 5, NULL);
}