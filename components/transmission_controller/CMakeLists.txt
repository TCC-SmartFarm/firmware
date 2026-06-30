idf_component_register(SRCS "EspHal.cpp" "lorawan_config.cpp" "payload_formatter.c"
                    INCLUDE_DIRS "."
                    REQUIRES jgromes__radiolib driver esp_timer sensor_controller)