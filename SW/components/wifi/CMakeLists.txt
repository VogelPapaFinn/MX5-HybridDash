idf_component_register(SRCS "src/Wifi.cpp" "src/WifiHost.cpp" "src/WifiJoin.cpp"
        PRIV_REQUIRES driver esp-tls esp_wifi filesystem nvs_flash
        INCLUDE_DIRS "."
)
