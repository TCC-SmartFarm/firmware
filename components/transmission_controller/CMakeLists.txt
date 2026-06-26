idf_component_register(SRCS "payload_formatter.c" "serial_cli.c"
                    INCLUDE_DIRS "."
                    REQUIRES device_config driver)