idf_component_register(SRCS "src/Filesystem.cpp" "src/Config.cpp" "src/SdCard.cpp"
        REQUIRES littlefs fatfs ArduinoJson
        REQUIRES driver
        INCLUDE_DIRS "."
)
