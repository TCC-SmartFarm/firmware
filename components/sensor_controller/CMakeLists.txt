idf_component_register(SRCS "soil_sensor.c" "air_sensor.c"
                    INCLUDE_DIRS "."
                    REQUIRES dht driver)