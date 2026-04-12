idf_component_register(SRCS "main.c" "protocol_ble.c" "protocol_ftm.c" "routine_micro.c" "max30102.c" "i2c-driver.c" "nvs_particions.c" "payload.c"
                    INCLUDE_DIRS "."
                    REQUIRES 
                        nvs_flash 
                        esp_event 
                        esp_wifi 
                        esp_netif 
                        esp_common
                        console
                        bt
                        driver
                        spiffs
                    )
