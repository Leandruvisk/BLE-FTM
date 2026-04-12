#ifndef ROUTINE_MICRO_H
#define ROUTINE_MICRO_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define EVT_BLE_READY      BIT0
#define EVT_FTM_READY      BIT1
#define EVT_SENSOR_READY   BIT2

extern EventGroupHandle_t system_events;

void routine_micro_start(void);
void build_payload(uint8_t *buffer);
#endif // ROUTINE_MICRO_H