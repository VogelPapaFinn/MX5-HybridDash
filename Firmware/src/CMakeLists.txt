set(CMAKE_C_STANDARD 23)

set(FILES "main.c"
        "../include/macros.h"
)

idf_component_register(SRCS ${FILES}
        PRIV_REQUIRES src driver spi_flash esp_psram esp_adc fatfs lvgl esp_lcd
        INCLUDE_DIRS "../include")