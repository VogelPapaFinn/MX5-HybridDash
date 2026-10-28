idf_component_register(SRCS "src/Can.cpp" "src/CanReceiver.cpp"
        PRIV_REQUIRES src driver esp_event
        INCLUDE_DIRS "."
)
