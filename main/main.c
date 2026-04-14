#include "main.h"
#include "protocol_ble.h"
#include "protocol_ftm.h"
#include "nvs_particions.h"
#include "max30102.h"
#include "routine_micro.h"


void app_main(void)
{
    init_nvs();
    max30102_start();

    routine_micro_start();

    ble_init();

    xTaskCreatePinnedToCore(max30102_task, "max_task", 4096, NULL, 10, NULL, 1);
    // xTaskCreate(ftm_task, "ftm", 4096, NULL, 5, NULL);
    xTaskCreate(spp_heartbeat_task, "spp_heartbeat_task", 1024*6, NULL, 5, NULL);
    
}