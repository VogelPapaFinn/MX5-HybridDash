idf_component_register(SRCS "src/Filesystem.cpp"
        REQUIRES spiffs fatfs
        PRIV_REQUIRES src driver nvs_flash
        INCLUDE_DIRS "."
)
