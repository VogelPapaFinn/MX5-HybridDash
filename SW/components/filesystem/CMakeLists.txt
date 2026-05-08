idf_component_register(SRCS "src/Filesystem.cpp" "src/Config.cpp"
        REQUIRES spiffs fatfs ArduinoJson
        PRIV_REQUIRES src driver nvs_flash
        INCLUDE_DIRS "."
)
