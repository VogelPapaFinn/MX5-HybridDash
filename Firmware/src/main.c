#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_psram.h"

void app_main(void)
{
    // Get flash size
    uint32_t flash_size;
    esp_flash_get_size(NULL, &flash_size);

    // Get RAM size
    size_t psram_size = esp_psram_get_size();

    // Print
    printf("Found %lu bytes of flash and %i bytes of psram.", flash_size, psram_size);
    heap_caps_malloc(16, MALLOC_CAP_SPIRAM);
}
