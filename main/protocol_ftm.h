#ifndef FTM_PROTOCOL_H
#define FTM_PROTOCOL_H

#include "main.h"

#include "argtable3/argtable3.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_spiffs.h"
#include "driver/gpio.h"



typedef struct {
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_end *end;
} wifi_args_t;

typedef struct {
    struct arg_str *ssid;
    struct arg_end *end;
} wifi_scan_arg_t;

typedef struct {
    struct arg_lit *initiator;
    struct arg_int *frm_count;
    struct arg_int *burst_period;
    struct arg_str *ssid;
} wifi_ftm_args_t;

static wifi_scan_arg_t scan_args;
static wifi_ftm_args_t ftm_args;

#define ETH_ALEN 6
#define MAX_CONNECT_RETRY_ATTEMPTS  5
#define BUFFER_SIZE 50
#define LED_PIN 8

static float FTMBuffer0[BUFFER_SIZE];
static float FTMBuffer1[BUFFER_SIZE];

static bool s_reconnect = true;
static int s_retry_num = 0;
static const char *TAG_STA = "ftm_station";

static EventGroupHandle_t s_wifi_event_group;
static const int CONNECTED_BIT = BIT0;
static const int DISCONNECTED_BIT = BIT1;

static EventGroupHandle_t s_ftm_event_group;
static const int FTM_REPORT_BIT = BIT0;
static const int FTM_FAILURE_BIT = BIT1;
static wifi_ftm_report_entry_t *s_ftm_report;
static uint8_t s_ftm_report_num_entries;
static uint32_t s_rtt_est, s_dist_est;
static bool s_ap_started;
extern uint8_t s_ap_channel;
static uint8_t s_ap_bssid[ETH_ALEN];



extern uint16_t g_scan_ap_num;
extern wifi_ap_record_t *g_ap_list_buffer;
extern char SSID[32];
extern const int g_report_lvl;
extern volatile bool ftm_running;





void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void ftm_process_report(void);
void initialise_wifi(void);
bool wifi_cmd_sta_join(const char *ssid, const char *pass);
bool wifi_perform_scan(const char *ssid, bool internal);
void wifi_cmd_scan(void);
wifi_ap_record_t *find_ftm_responder_ap(const char *ssid);
void wifi_cmd_ftm(void);
void register_wifi(void);
void ftm_measure(void);
void ftm_task(void *pvParameters);


#endif /* FTM_PROTOCOL_H */