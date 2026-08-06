idf_component_register(SRCS "wifi_portal.c"
                    INCLUDE_DIRS "."
                    REQUIRES device_config esp_wifi esp_http_server esp_netif esp_event nvs_flash json mdns)