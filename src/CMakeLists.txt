set(CMAKE_C_STANDARD 23)

set(IDF_TOOLCHAIN "clang")

set(FILES
        # Start of Application
        "main.cpp"

        # Drivers
        "Driver/ST77916.cpp"

        # StateMachine
        "StateMachine/StateMachine.cpp"
        "StateMachine/StateEvents.cpp"

        "StateMachine/States/Startup.cpp"
        "StateMachine/States/WaitingForConfiguration.cpp"
        "StateMachine/States/Configuration.cpp"
        "StateMachine/States/Operation.cpp"

        # Everything else
        "Handler/GuiHandler.cpp"
)

file(GLOB UI
        "../res/ui/*.h"
        "../res/ui/*.c"
)

idf_component_register(SRCS ${FILES} ${UI}
        PRIV_REQUIRES src driver spi_flash esp_psram lvgl esp_lcd esp_wifi can app_update esp_lcd_st77916 ArduinoJson filesystem wifi esp_http_client esp_https_ota
        INCLUDE_DIRS "../include/" "." "../res/")

spiffs_create_partition_image(config "../config/" FLASH_INTO_PROJECT)