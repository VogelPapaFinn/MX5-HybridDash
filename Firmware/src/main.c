#include "esp_chip_info.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>
#include <string.h>
#include <sys/unistd.h>

#include "esp_flash.h"
#include "esp_psram.h"

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "math.h"
#include "sdmmc_cmd.h"

#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "macros.h"

#include "driver/spi_common.h"
#include "esp_lcd_gc9a01.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_dev.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_types.h"
#include "esp_timer.h"
#include "lvgl.h"

#define TEST_LCD_HOST SPI2_HOST
#define TEST_LCD_H_RES (240)
#define TEST_LCD_V_RES (240)
#define TEST_LCD_BIT_PER_PIXEL (16)

#define TEST_PIN_NUM_LCD_CS (GPIO_NUM_40)
#define TEST_PIN_NUM_LCD_PCLK (GPIO_NUM_42)
#define TEST_PIN_NUM_LCD_DATA0 (GPIO_NUM_38)
#define TEST_PIN_NUM_LCD_RST (GPIO_NUM_43)
#define TEST_PIN_NUM_LCD_DC (GPIO_NUM_44)

esp_lcd_panel_handle_t panel_handle = NULL;
void flush(lv_display_t *display, const lv_area_t *area, uint8_t *px_map) {
    lv_draw_sw_rgb565_swap(px_map, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));
    // +1 needed, otherwise the image is distorted
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);
    lv_display_flush_ready(display);
}

static void lv_tick_task(void *arg) {
    (void) arg;

    lv_tick_inc(1);
}

SemaphoreHandle_t xGuiSemaphore;

void app_main(void) {
    xGuiSemaphore = xSemaphoreCreateMutex();

    spi_bus_config_t buscfg = {
        .sclk_io_num = TEST_PIN_NUM_LCD_PCLK,
        .mosi_io_num = TEST_PIN_NUM_LCD_DATA0,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE //TEST_LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(TEST_LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = TEST_PIN_NUM_LCD_DC,
        .cs_gpio_num = TEST_PIN_NUM_LCD_CS,
        .pclk_hz = 60000000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) TEST_LCD_HOST, &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = TEST_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));

    /* INIT LVGL */

    lv_init();
    lv_display_t *display = lv_display_create(TEST_LCD_H_RES, TEST_LCD_V_RES);
    size_t draw_buffer_sz = (TEST_LCD_H_RES * TEST_LCD_V_RES * 2);

    uint16_t *buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM);
    memset(buf1, 0, draw_buffer_sz);
    uint16_t *buf2 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM);
    memset(buf2, 0, draw_buffer_sz);

    // This prevents that the display shows random colored pixels for a split of a second, during startup
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 240, 240, buf1);

    // initialize LVGL draw buffers
    lv_display_set_buffers(display, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_DIRECT);
    // set color depth
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    // set the callback which can copy the rendered image to an area of the display
    lv_display_set_flush_cb(display, flush);

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1 * 1000));

    /*Create a white label, set its text and align it to the center*/
    //lv_obj_t *label = lv_label_create(lv_screen_active());
    //lv_label_set_text(label, "Hello world");
    //lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0x000000), LV_PART_MAIN);
    //lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    //lv_display_flush_ready(display);

    // Clear the screen
    //lv_obj_clean(lv_screen_active());
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), LV_PART_MAIN);
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    lv_style_t style;
    lv_style_init(&style);
    lv_style_set_line_color(&style, lv_color_make(255, 0, 0)); // RGB
    lv_style_set_line_width(&style, 2);
    lv_style_set_line_rounded(&style, false);

    lv_obj_t *vline = lv_line_create(lv_screen_active());
    static lv_point_precise_t line_points[] = {{120, 0}, {120, 240}};
    lv_line_set_points(vline, line_points, 2);
    lv_obj_add_style(vline, &style, 0);

    lv_style_t style2;
    lv_style_init(&style2);
    lv_style_set_line_color(&style2, lv_color_make(0, 255, 0)); // RBG
    lv_style_set_line_width(&style2, 2);
    lv_style_set_line_rounded(&style2, false);

    lv_obj_t *hline = lv_line_create(lv_screen_active());
    static lv_point_precise_t line_points2[] = {{0, 120}, {240, 120}};
    lv_line_set_points(hline, line_points2, 2);
    lv_obj_add_style(hline, &style2, 0);

    lv_obj_t *btn = lv_button_create(lv_screen_active()); /*Add a button the current screen*/
    lv_obj_set_pos(btn, 10, 10); /*Set its position*/
    lv_obj_set_size(btn, 120, 50); /*Set its size*/
    lv_obj_add_event_cb(btn, NULL, LV_EVENT_ALL, NULL); /*Assign a callback to the button*/
    lv_obj_center(btn);

    lv_obj_t *label = lv_label_create(btn); /*Add a label to the button*/
    lv_label_set_text(label, "Button"); /*Set the labels text*/
    lv_obj_center(label);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));

        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
    }
}

/*void app_main(void) {
    // Get flash size
    uint32_t flash_size;
    esp_flash_get_size(NULL, &flash_size);

    // Get RAM size
    size_t psram_size = esp_psram_get_size();

    // Print
    printf("Found %lu bytes of flash and %i bytes of psram.\n", flash_size, psram_size);

    // ADC2 Init
    adc_oneshot_unit_handle_t adc2_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
            .unit_id = ADC_UNIT_1,
            .ulp_mode = ADC_ULP_MODE_DISABLE};
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc2_handle));

    // ADC2 Config
    adc_oneshot_chan_cfg_t config = {
            .bitwidth = ADC_BITWIDTH_12,
            .atten = ADC_ATTEN_DB_6,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, ADC_CHANNEL_6, &config));

    adc_cali_handle_t adc2_cali_handle = NULL;
    adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT_2,
            .chan = ADC_CHANNEL_6,
            .atten = ADC_ATTEN_DB_6,
            .bitwidth = ADC_BITWIDTH_12,
    };
    adc_cali_create_scheme_curve_fitting(&cali_config, &adc2_cali_handle);

    // SD Card
    sdmmc_card_t *card = NULL;
    sdmmc_host_t sdhost = SDMMC_HOST_DEFAULT();
    sdhost.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
    sdhost.slot = SDMMC_HOST_SLOT_0;
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4;
    slot_config.clk = GPIO_NUM_16;
    slot_config.cmd = GPIO_NUM_17;
    slot_config.d0 = GPIO_NUM_4;
    slot_config.d1 = GPIO_NUM_5;
    slot_config.d2 = GPIO_NUM_8;
    slot_config.d3 = GPIO_NUM_18;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
            .format_if_mount_failed = true,
            .max_files = 5,
            .allocation_unit_size = 16 * 1024};
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &sdhost, &slot_config, &mount_config, &card);
    if (ret == ESP_OK) {
        printf("Mounted SDCard!");
        sdmmc_card_print_info(stdout, card);
        ;
    } else
        printf("Mounting SDCard FAILED!");

    int rawAdcValue = 0;
    int voltage = 0;
    double voltage2 = 0.0;

    while (true) {
        if (adc_oneshot_read(adc2_handle, ADC_CHANNEL_6, &rawAdcValue) == ESP_OK) {
            printf("\n\nThe raw adc value is %d\n", rawAdcValue);
        }

        adc_cali_raw_to_voltage(adc2_cali_handle, rawAdcValue, &voltage);
        printf("The calculated voltage is %dmV\n", voltage);
        printf("The calculated temperature is %f °C\n", (((double) voltage - 540.0) / 10.0));

        voltage2 = rawAdcValue * (1.906 / pow(2, config.bitwidth));// We calculated 1906. With a raw value of 1591 it should calc 741mV.
        printf("The calculated voltage is %fmV\n", voltage2);
        printf("The calculated temperature is %f °C\n", ((voltage2 - 0.54) / 0.01));// We subtract a 540mV from the voltage, otherwise our temperature is way too high

        // Wait 1s
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}*/
