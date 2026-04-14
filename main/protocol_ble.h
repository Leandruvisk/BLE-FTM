#ifndef BLE_SPP_SERVER_DEMO_H
#define BLE_SPP_SERVER_DEMO_H

#include "main.h"



#include "esp_bt.h"
#include "driver/uart.h"
#include "string.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"


/*
 * DEFINES
 ****************************************************************************************
 */
// #define SUPPORT_HEARTBEAT
//#define SPP_DEBUG_MODE

#define spp_sprintf(s,...)         sprintf((char*)(s), ##__VA_ARGS__)
#define SPP_DATA_MAX_LEN           (512)
#define SPP_CMD_MAX_LEN            (20)
#define SPP_STATUS_MAX_LEN         (20)
#define SPP_DATA_BUFF_MAX_LEN      (2*1024)
///Attributes State Machine



void uart_task(void *pvParameters);
void spp_uart_init(void);
void spp_cmd_task(void * arg);
void spp_task_init(void);
void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
void ble_init();
void ble_send_data(uint8_t *data, uint16_t len);
void spp_heartbeat_task(void * arg);
static inline bool ble_can_send(void);




#endif // BLE_SPP_SERVER_DEMO_H