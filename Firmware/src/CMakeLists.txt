set(CMAKE_C_STANDARD 23)

set(FILES
        # Start of Application
        "main.c"

        # Core
        "Core/Core.c"

        # Logger
        "Logger/Logger.c"

        # FileManager
        "FileManager/FileManager.c"

        # GUI
        "GUI/GUI.c"

        # SensorManager
        "SensorManager/SensorManager.c"

        # Utilities
        "../include/macros.h"

        # Fonts
        "../res/fonts/c/E1234_80.c"
        "../res/fonts/c/E1234_70.c"
        "../res/fonts/c/VCR_OSD_MONO_24.c"

        # Images
        "../res/img/c/blinkerRight.c"
        "../res/img/c/blinkerLeft.c"
)

idf_component_register(SRCS ${FILES}
        PRIV_REQUIRES src driver spi_flash esp_psram esp_adc fatfs lvgl esp_lcd esp_lcd_gc9a01 spiffs
        INCLUDE_DIRS "../include")