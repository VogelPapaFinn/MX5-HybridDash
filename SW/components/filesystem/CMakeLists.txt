idf_component_register(SRCS "src/Filesystem.cpp" "src/Config.cpp"
        REQUIRES spiffs fatfs ArduinoJson
        PRIV_REQUIRES src driver
        INCLUDE_DIRS "."
)
