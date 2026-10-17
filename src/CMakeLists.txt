set(CMAKE_C_STANDARD 23)

set(IDF_TOOLCHAIN "clang")

set(FILES
        # Start of Application
        "main.cpp"

        # Drivers
        "Driver/Display.cpp"
        "Driver/KLine.cpp"

        # WebInterface
        "WebInterface/WebInterface.cpp"

        # Events
        "Events.cpp"

        # States
        "State/State.cpp"
        "State/Registration.cpp"
        "State/Operation.cpp"

        # Handlers
        "Handler/RegistrationHandler.cpp"

        # Sensors
        "Sensor/SensorHandler.cpp"
        "Sensor/ActiveSensor.cpp"
        "Sensor/PassiveSensor.cpp"

        "Sensor/OilPressure.cpp"
        "Sensor/WaterTemperature.cpp"
        "Sensor/FuelLevel.cpp"

        "Sensor/Speed.cpp"
        "Sensor/Rpm.cpp"
        "Sensor/LeftIndicator.cpp"
        "Sensor/RightIndicator.cpp"
)

idf_component_register(SRCS ${FILES}
        REQUIRES src driver spi_flash esp_psram esp_adc esp_wifi esp_http_server nvs_flash can esp_timer app_update esp_http_client esp_https_ota mbedtls esp-tls ArduinoJson filesystem wifi
        INCLUDE_DIRS "../include" "."
        EMBED_TXTFILES "" # For Files
        EMBED_FILES "") # For images

spiffs_create_partition_image(data "../data/" FLASH_INTO_PROJECT)

spiffs_create_partition_image(config "../config/" FLASH_INTO_PROJECT)

add_custom_target(app-flash-monitor)
add_dependencies(app-flash-monitor
        app
        flash
        monitor
)