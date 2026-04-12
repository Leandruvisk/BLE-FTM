#include "protocol_ftm.h"

uint8_t s_ap_channel = 0;

void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
	if (event_id == WIFI_EVENT_STA_CONNECTED) {
        wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t *)event_data;

        ESP_LOGI(TAG_STA, "Connected to %s (BSSID: "MACSTR", Channel: %d)", event->ssid,
                 MAC2STR(event->bssid), event->channel);

        memcpy(s_ap_bssid, event->bssid, ETH_ALEN);
        s_ap_channel = event->channel;
        xEventGroupClearBits(s_wifi_event_group, DISCONNECTED_BIT);
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_reconnect && ++s_retry_num < MAX_CONNECT_RETRY_ATTEMPTS) {
            ESP_LOGI(TAG_STA, "sta disconnect, retry attempt %d...", s_retry_num);
            esp_wifi_connect();
        } else {
            ESP_LOGI(TAG_STA, "sta disconnected");
        }
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
        xEventGroupSetBits(s_wifi_event_group, DISCONNECTED_BIT);
    } else if (event_id == WIFI_EVENT_FTM_REPORT) {
        wifi_event_ftm_report_t *event = (wifi_event_ftm_report_t *) event_data;

        if (event->status == FTM_STATUS_SUCCESS) {
            s_rtt_est = event->rtt_est;
            s_dist_est = event->dist_est;
            s_ftm_report = event->ftm_report_data;
            s_ftm_report_num_entries = event->ftm_report_num_entries;
            xEventGroupSetBits(s_ftm_event_group, FTM_REPORT_BIT);
        } else {
            ESP_LOGI(TAG_STA, "FTM procedure with Peer("MACSTR") failed! (Status - %d)",
                     MAC2STR(event->peer_mac), event->status);
            xEventGroupSetBits(s_ftm_event_group, FTM_FAILURE_BIT);
        }
    } else if (event_id == WIFI_EVENT_AP_START) {
        s_ap_started = true;
    } else if (event_id == WIFI_EVENT_AP_STOP) {
        s_ap_started = false;
    }
}

void ftm_process_report(void)
{
    int i;
    char *log = NULL;

    if (!g_report_lvl)
        return;

    log = malloc(200);
    if (!log) {
        ESP_LOGE(TAG_STA, "Failed to alloc buffer for FTM report");
        return;
    }

    bzero(log, 200);
    sprintf(log, "%s%s%s%s", g_report_lvl & BIT0 ? " Diag |":"", g_report_lvl & BIT1 ? "   RTT   |":"",
                 g_report_lvl & BIT2 ? "       T1       |       T2       |       T3       |       T4       |":"",
                 g_report_lvl & BIT3 ? "  RSSI  |":"");
    ESP_LOGI(TAG_STA, "FTM Report:");
    ESP_LOGI(TAG_STA, "|%s", log);
    for (i = 0; i < s_ftm_report_num_entries; i++) {
        char *log_ptr = log;

        bzero(log, 200);
        if (g_report_lvl & BIT0) {
            log_ptr += sprintf(log_ptr, "%6d|", s_ftm_report[i].dlog_token);
        }
        if (g_report_lvl & BIT1) {
            log_ptr += sprintf(log_ptr, "%7" PRIu32 "  |", s_ftm_report[i].rtt);
        }
        if (g_report_lvl & BIT2) {
            log_ptr += sprintf(log_ptr, "%14llu  |%14llu  |%14llu  |%14llu  |", s_ftm_report[i].t1,
                                        s_ftm_report[i].t2, s_ftm_report[i].t3, s_ftm_report[i].t4);
        }
        if (g_report_lvl & BIT3) {
            log_ptr += sprintf(log_ptr, "%6d  |", s_ftm_report[i].rssi);
        }
        ESP_LOGI(TAG_STA, "|%s", log);
    }
    free(log);
}

void initialise_wifi(void)
{
    esp_log_level_set("wifi", ESP_LOG_WARN);
    static bool initialized = false;

    if (initialized) {
        return;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    s_wifi_event_group = xEventGroupCreate();
    s_ftm_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    initialized = true;
}

bool wifi_cmd_sta_join(const char *ssid, const char *pass)
{
    int bits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT, 0, 1, 0);

    wifi_config_t wifi_config = { 0 };

    strlcpy((char *) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    if (pass) {
        strlcpy((char *) wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
    }

    if (bits & CONNECTED_BIT) {
        s_reconnect = false;
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
        ESP_ERROR_CHECK( esp_wifi_disconnect() );
        xEventGroupWaitBits(s_wifi_event_group, DISCONNECTED_BIT, 0, 1, portTICK_PERIOD_MS);
    }

    s_reconnect = true;
    s_retry_num = 0;
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_connect() );

    xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT, 0, 1, 5000 / portTICK_PERIOD_MS);

    return true;
}


bool wifi_perform_scan(const char *ssid, bool internal)
{
    wifi_scan_config_t scan_config = { 0 };
    scan_config.ssid = (uint8_t *) ssid;
    uint8_t i;

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    esp_wifi_scan_start(&scan_config, true);
    esp_wifi_scan_get_ap_num(&g_scan_ap_num);
    g_ap_list_buffer = malloc(g_scan_ap_num * sizeof(wifi_ap_record_t));
    
    if (esp_wifi_scan_get_ap_records(&g_scan_ap_num, (wifi_ap_record_t *)g_ap_list_buffer) == ESP_OK) {
        for (i = 0; i < g_scan_ap_num; i++) {
               ESP_LOGI(TAG_STA, "[%s][rssi=%d]""%s", g_ap_list_buffer[i].ssid, g_ap_list_buffer[i].rssi, g_ap_list_buffer[i].ftm_responder ? "[FTM Responder]" : "");
           }
    }
    ESP_LOGI(TAG_STA, "sta scan done");
    return true;
}

void wifi_cmd_scan(void)
{

    ESP_LOGI(TAG_STA, "sta start to scan");
    if ( scan_args.ssid->count == 1 ) {
        wifi_perform_scan(scan_args.ssid->sval[0], false);
    } else {
        wifi_perform_scan(NULL, false);
    }
    //return 0;
}

wifi_ap_record_t *find_ftm_responder_ap(const char *ssid)
{
    bool retry_scan = false;
    uint8_t i;

    if (!ssid)
        return NULL;

retry:
    if (!g_ap_list_buffer || (g_scan_ap_num == 0)) {
        ESP_LOGI(TAG_STA, "Scanning for %s", ssid);
        if (false == wifi_perform_scan(ssid, true)) {
            return NULL;
        }
    }

    for (i = 0; i < g_scan_ap_num; i++) {
        if (strcmp((const char *)g_ap_list_buffer[i].ssid, ssid) == 0)
            return &g_ap_list_buffer[i];
    }

    if (!retry_scan) {
        retry_scan = true;
        if (g_ap_list_buffer) {
            free(g_ap_list_buffer);
            g_ap_list_buffer = NULL;
        }
        goto retry;
    }

    ESP_LOGI(TAG_STA, "No matching AP found");

    return NULL;
}

void wifi_cmd_ftm(void)
{
    
    wifi_ap_record_t *ap_record;
    EventBits_t bits;

    wifi_ftm_initiator_cfg_t ftmi_cfg = {
        .frm_count = 8,
        .burst_period = 10,
    };

    bits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT, 0, 1, 0);
    if (bits & CONNECTED_BIT && !ftm_args.ssid->count) {
        memcpy(ftmi_cfg.resp_mac, s_ap_bssid, ETH_ALEN);
        ftmi_cfg.channel = s_ap_channel;
    } else if (ftm_args.ssid->count == 1) {
        ap_record = find_ftm_responder_ap(ftm_args.ssid->sval[0]);
        if (ap_record) {
            memcpy(ftmi_cfg.resp_mac, ap_record->bssid, 6);
            ftmi_cfg.channel = ap_record->primary;
        } 
    } else {
        ESP_LOGE(TAG_STA, "Provide SSID of the AP in disconnected state!");
    }


    ESP_LOGI(TAG_STA, "Requesting FTM session with Frm Count - %d, Burst Period - %dmSec (0: No Preference)",
             ftmi_cfg.frm_count, ftmi_cfg.burst_period*100);

    if (ESP_OK != esp_wifi_ftm_initiate_session(&ftmi_cfg)) {
        ESP_LOGE(TAG_STA, "Failed to start FTM session");
    }

    bits = xEventGroupWaitBits(s_ftm_event_group, FTM_REPORT_BIT | FTM_FAILURE_BIT,
                                           pdTRUE, pdFALSE, portMAX_DELAY);
    
    if (bits & FTM_REPORT_BIT) {
        ftm_process_report();
        free(s_ftm_report);
        s_ftm_report = NULL;
        s_ftm_report_num_entries = 0;
        ESP_LOGI(TAG_STA, "Estimated RTT - %" PRId32 " nSec, Estimated Distance - %" PRId32 ".%02" PRId32 " meters",
                          s_rtt_est, s_dist_est / 100, s_dist_est % 100);
    }

}

void register_wifi(void)
{

    scan_args.ssid = arg_str0(NULL, NULL, "<ssid>", "SSID of AP want to be scanned");
    scan_args.end = arg_end(1);

    
    ftm_args.initiator = arg_lit0("I", "ftm_initiator", "FTM Initiator mode");
    ftm_args.ssid = arg_str0("s", "ssid", "SSID", "SSID of AP");
    ftm_args.frm_count = arg_int0("c", "frm_count", "<0/8/16/24/32/64>", "FTM frames to be exchanged (0: No preference)");
    ftm_args.burst_period = arg_int0("p", "burst_period", "<2-255 (x 100 mSec)>", "Periodicity of FTM bursts in 100's of miliseconds (0: No preference)");


}

void ftm_routine(void)
{
    // esp_err_t ret = nvs_flash_init();
    // if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    //     ESP_ERROR_CHECK(nvs_flash_erase());
    //     ret = nvs_flash_init();
    // }
    // ESP_ERROR_CHECK( ret );

    initialise_wifi();

    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    repl_config.prompt = "ftm>";
    // init console REPL environment
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
    
    register_system();
    register_wifi();
    
    gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_8, 0);

    printf("\n ==========================================================\n");
    printf(" |                      Steps to test FTM                 |\n");
    printf(" |  1. Use 'scan' command to search for external AP's     |\n");
    printf(" |  2. On second device initiate FTM with an AP using     |\n");
    printf(" |     command 'ftm -I -s <SSID>'                         |\n");
    printf(" ==========================================================\n\n");

    // start console REPL
    ESP_ERROR_CHECK(esp_console_start_repl(repl));

    wifi_cmd_sta_join("FTM", "12345678");
    ESP_LOGI(TAG_STA, "sta connecting to '%s'", "FTM");
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    uint16_t i;
    for (i = 0; i < BUFFER_SIZE; i++) {
        wifi_cmd_ftm();
        
        wifi_ap_record_t ap;
        esp_wifi_sta_get_ap_info(&ap);

        FTMBuffer0[i] = (s_dist_est);
        FTMBuffer1[i] = (ap.rssi);
        printf("Current value of i: %d\n", i);
    }

    printf("%s\n", "Distancia;");
    for (int i = 0; i < BUFFER_SIZE; i++) {
        printf("%.2f;\n", FTMBuffer0[i]/100);
    }
    printf("%s\n", "RSSI;");
    for (int i = 0; i < BUFFER_SIZE; i++) {
        printf("%.0f;\n", FTMBuffer1[i]);
    }
    gpio_set_level(GPIO_NUM_8, 1);


}
