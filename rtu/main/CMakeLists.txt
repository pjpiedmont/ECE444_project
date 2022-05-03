set(PROJECT_NAME "rtu")

idf_component_register(SRCS "INA209.c" "i2c.c" "sensors.c" "modbus.c" "services.c" "main.c"
                    INCLUDE_DIRS "." "../include")
