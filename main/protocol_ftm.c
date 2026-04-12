#include "protocol_ftm.h"
#include "routine_micro.h"
#include "protocol_ble.h"
#include "globals.h"
#include <string.h>

#define FTM_SSID       "FTM"
#define FTM_PASS       "12345678"
#define FTM_DELAY_MS   1000
#define WIFI_STABLE_MS 2000
#define FTM_TIMEOUT_MS 5000
#define FTM_MAX_RETRY  1

// --- Variáveis Globais Exportadas para o Payload ---
float g_last_ftm_distance = -1.0;
int g_last_ftm_rtt = 0;
int g_last_ftm_rssi = 0;

uint8_t s_ap_channel = 0;
volatile bool ftm_running = false;
volatile bool ble_tx_paused = false;

extern EventGroupHandle_t system_events;

wifi_ftm_initiator_cfg_t ftmi_cfg = {
    .frm_count = 16,
    .burst_period = 2,
    .use_get_report_api = false,
};

// ============================================================================
// PROTÓTIPOS (Isso mata o erro de implicit declaration)
// ============================================================================
void wifi_init_sta(void);
void wifi_connect(void);
bool ftm_try_once(void);
bool ftm_perform(void);
void ftm_measure(void);
void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

// ============================================================================
// IMPLEMENTAÇÕES
// ============================================================================

void event_handler(void *arg, esp_event_base_t event_base,
                   int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_CONNECTED: {
                wifi_event_sta_connected_t *event = event_data;
                ESP_LOGI(TAG_STA, "Connected to %s (ch=%d)", event->ssid, event->channel);
                memcpy(s_ap_bssid, event->bssid, ETH_ALEN);
                s_ap_channel = event->channel;
                xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
                break;
            }
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGW(TAG_STA, "Disconnected → reconnecting");
                esp_wifi_connect();
                xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
                break;
            case WIFI_EVENT_FTM_REPORT: {
                wifi_event_ftm_report_t *event = event_data;
                if (event->status == FTM_STATUS_SUCCESS) {
                    s_rtt_est  = event->rtt_est;
                    s_dist_est = event->dist_est;
                    xEventGroupSetBits(s_ftm_event_group, FTM_REPORT_BIT);
                } else {
                    ESP_LOGW(TAG_STA, "FTM FAIL (Status=%d)", event->status);
                    xEventGroupSetBits(s_ftm_event_group, FTM_FAILURE_BIT);
                }
                break;
            }
            default:
                break;
        }
    }
}

void wifi_init_sta(void)
{
    static bool initialized = false;
    if (initialized) return;

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    s_wifi_event_group = xEventGroupCreate();
    s_ftm_event_group  = xEventGroupCreate();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    initialized = true;
}

void wifi_connect(void)
{
    wifi_config_t wifi_config = {0};

    strcpy((char *)wifi_config.sta.ssid, FTM_SSID);
    strcpy((char *)wifi_config.sta.password, FTM_PASS);

    // Ajustado de ESP_IF_WIFI_STA para WIFI_IF_STA conforme exigência do ESP-IDF v5+
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG_STA, "Connecting to %s...", FTM_SSID);

    xEventGroupWaitBits(s_wifi_event_group,
                        CONNECTED_BIT,
                        false,
                        true,
                        portMAX_DELAY);

    ESP_LOGI(TAG_STA, "WiFi connected");

    vTaskDelay(pdMS_TO_TICKS(WIFI_STABLE_MS));
}

bool ftm_try_once(void)
{
    EventBits_t bits;

    xEventGroupClearBits(s_ftm_event_group,
                         FTM_REPORT_BIT | FTM_FAILURE_BIT);

    memcpy(ftmi_cfg.resp_mac, s_ap_bssid, ETH_ALEN);
    ftmi_cfg.channel = s_ap_channel;

    ESP_LOGI(TAG_STA, "FTM start (ch=%d)", ftmi_cfg.channel);

    if (esp_wifi_ftm_initiate_session(&ftmi_cfg) != ESP_OK) {
        return false;
    }

    bits = xEventGroupWaitBits(
        s_ftm_event_group,
        FTM_REPORT_BIT | FTM_FAILURE_BIT,
        pdTRUE,
        pdFALSE,
        pdMS_TO_TICKS(4000)
    );

    return (bits & FTM_REPORT_BIT);
}

bool ftm_perform(void)
{
    ftm_running = true;
    ble_tx_paused = true;

    vTaskDelay(pdMS_TO_TICKS(100));

    for (int i = 0; i < FTM_MAX_RETRY; i++) {
        if (ftm_try_once()) {
            
            // Salva na variável global para o build_payload ler
            // g_last_ftm_distance = s_dist_est / 100.0;
            g_last_ftm_distance = s_dist_est;
            g_last_ftm_rtt = s_rtt_est;
            
            wifi_ap_record_t ap;
            esp_wifi_sta_get_ap_info(&ap);
            g_last_ftm_rssi = ap.rssi;

            ESP_LOGI(TAG_STA, "Dist: %.2f m | RTT: %d ns", g_last_ftm_distance, g_last_ftm_rtt);

            ftm_running = false;
            ble_tx_paused = false;
            return true;
        }

        ESP_LOGW(TAG_STA, "Retry %d/%d", i+1, FTM_MAX_RETRY);
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    ftm_running = false;
    ble_tx_paused = false;
    return false;
}

void ftm_measure(void)
{
    wifi_init_sta();
    wifi_connect();

    vTaskDelay(pdMS_TO_TICKS(200));

    ftm_perform();

    xEventGroupSetBits(system_events, EVT_FTM_READY);
}

void ftm_task(void *pvParameters)
{
    while (1) {
        ftm_measure();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}