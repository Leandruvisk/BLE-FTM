#include "main.h"
#include "protocol_ble.h"
#include "protocol_ftm.h"
#include "nvs_particions.h"
#include "max30102.h"
#include "routine_micro.h"




void console_init(void)
{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();

    repl_config.prompt = "ftm>";

    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

void app_main(void)
{
    init_nvs();
    console_init();

    routine_micro_start();

    ble_start();

    xTaskCreate(max30102_task, "max30102", 4096, NULL, 5, NULL);
    xTaskCreate(ftm_task, "ftm", 4096, NULL, 5, NULL);
}