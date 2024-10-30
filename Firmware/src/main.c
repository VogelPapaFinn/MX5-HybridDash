#include <esp_flash.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include <esp_psram.h>

// Prototypes
int loop();

// Main function
int app_main() {
    // Search for the PSRAM
    if(esp_psram_init() == ESP_OK) {
        // Found
        printf("PSRAM found with %d MB", esp_psram_get_size());
    } else {
        // Not found
        printf("ERROR: PSRAM not found! ABORTING!");
        return -1;
    }

    return loop();
}

// Main loop
int loop() {
    return 0;
}
