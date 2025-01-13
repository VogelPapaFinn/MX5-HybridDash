/* --- Includes --- */
#include "GUI/GUI.h"

#include <Logger/Logger.h>

/* --- Private Defines & Macros --- */

/* --- Private Variables, Typedefs etc. --- */

// General display stuff
const size_t drawBufferSize_ = GUI_LCD_RES * GUI_LCD_RES * 2;

// Display 1 stuff
esp_lcd_panel_io_handle_t lcdPanelIoHandle1_ = NULL;
esp_lcd_panel_handle_t lcdPanelHandle1_ = NULL;
lv_display_t *display1_ = NULL;
uint16_t *drawBuffer11_ = NULL;
uint16_t *drawBuffer12_ = NULL;

// Display 2 stuff
esp_lcd_panel_io_handle_t lcdPanelIoHandle2_ = NULL;
esp_lcd_panel_handle_t lcdPanelHandle2_ = NULL;
lv_display_t *display2_ = NULL;
uint16_t *drawBuffer21_ = NULL;
uint16_t *drawBuffer22_ = NULL;

// Display 3 stuff
esp_lcd_panel_io_handle_t lcdPanelIoHandle3_ = NULL;
esp_lcd_panel_handle_t lcdPanelHandle3_ = NULL;
lv_display_t *display3_ = NULL;
uint16_t *drawBuffer31_ = NULL;
uint16_t *drawBuffer32_ = NULL;

// Variables indicating stati
bool initSuccessful_ = false;

/* --- Function implementations --- */

//! \brief Initializes the three SPI displays
//! \retval A boolean indicating if the operation was successful
bool initDisplays(void) {
    /*
     * --- --- FIRST DISPLAY
     */

    // Create the SPI config
    const esp_lcd_panel_io_spi_config_t lcdPanel1IoConfig = {
            .dc_gpio_num = GUI_GPIO_LCD_DC,
            .cs_gpio_num = GUI_GPIO_LCD1_CS,
            .pclk_hz = 60000000,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
            .spi_mode = 0,
            .trans_queue_depth = 10,
    };

    // Then attach it to the SPI bus
    if (esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) GUI_LCD_SPI_HOST, &lcdPanel1IoConfig, &lcdPanelIoHandle1_)) { return false; }

    // Then create the panel config
    const esp_lcd_panel_dev_config_t lcdPanelConfig = {
            .reset_gpio_num = GUI_GPIO_LCD_RST,
            .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
            .bits_per_pixel = 16,
    };

    // Then activate it
    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(lcdPanelIoHandle1_, &lcdPanelConfig, &lcdPanelHandle1_));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcdPanelHandle1_));
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcdPanelHandle1_));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(lcdPanelHandle1_, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(lcdPanelHandle1_, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcdPanelHandle1_, false));

    /*
     * --- --- SECOND DISPLAY
     */

    // Create the SPI config
    const esp_lcd_panel_io_spi_config_t lcdPanel2IoConfig = {
            .dc_gpio_num = GUI_GPIO_LCD_DC,
            .cs_gpio_num = GUI_GPIO_LCD2_CS,
            .pclk_hz = 60000000,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
            .spi_mode = 0,
            .trans_queue_depth = 10,
    };

    // Then attach it to the SPI bus
    if (esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) GUI_LCD_SPI_HOST, &lcdPanel2IoConfig, &lcdPanelIoHandle2_)) { return false; }

    // Then activate it
    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(lcdPanelIoHandle2_, &lcdPanelConfig, &lcdPanelHandle2_));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcdPanelHandle2_));
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcdPanelHandle2_));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(lcdPanelHandle2_, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(lcdPanelHandle2_, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcdPanelHandle2_, false));

    /*
     * --- --- THIRD DISPLAY
     */

    // Create the SPI config
    const esp_lcd_panel_io_spi_config_t lcdPanel3IoConfig = {
            .dc_gpio_num = GUI_GPIO_LCD_DC,
            .cs_gpio_num = GUI_GPIO_LCD3_CS,
            .pclk_hz = 60000000,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
            .spi_mode = 0,
            .trans_queue_depth = 10,
    };

    // Then attach it to the SPI bus
    if (esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) GUI_LCD_SPI_HOST, &lcdPanel3IoConfig, &lcdPanelIoHandle3_)) { return false; }

    // Then activate it
    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(lcdPanelIoHandle3_, &lcdPanelConfig, &lcdPanelHandle3_));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcdPanelHandle3_));
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcdPanelHandle3_));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(lcdPanelHandle3_, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(lcdPanelHandle3_, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcdPanelHandle3_, false));

    // Everything was successful
    return true;
}

//! \brief Callback function for LVGL to draw to the first physical display
//! \param display A pointer to the lvgl display which is drawn too
//! \param area The area which is updated
//! \param pxMap An array which contains the colors for each pixel
void flushToDisplay1(lv_display_t *display, const lv_area_t *area, uint8_t *pxMap) {
    // Swap the color channels as needed
    lv_draw_sw_rgb565_swap(pxMap, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));

    // Then draw the bitmap to the physical display (+1 needed, otherwise the image is distorted)
    esp_lcd_panel_draw_bitmap(lcdPanelHandle1_, area->x1, area->y1, area->x2 + 1, area->y2 + 1, pxMap);
    lv_display_flush_ready(display);
}

//! \brief Callback function for LVGL to draw to the second physical display
//! \param display A pointer to the lvgl display which is drawn too
//! \param area The area which is updated
//! \param pxMap An array which contains the colors for each pixel
void flushToDisplay2(lv_display_t *display, const lv_area_t *area, uint8_t *pxMap) {
    // Swap the color channels as needed
    lv_draw_sw_rgb565_swap(pxMap, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));

    // Then draw the bitmap to the physical display (+1 needed, otherwise the image is distorted)
    esp_lcd_panel_draw_bitmap(lcdPanelHandle2_, area->x1, area->y1, area->x2 + 1, area->y2 + 1, pxMap);
    lv_display_flush_ready(display);
}

//! \brief Callback function for LVGL to draw to the third physical display
//! \param display A pointer to the lvgl display which is drawn too
//! \param area The area which is updated
//! \param pxMap An array which contains the colors for each pixel
void flushToDisplay3(lv_display_t *display, const lv_area_t *area, uint8_t *pxMap) {
    // Swap the color channels as needed
    lv_draw_sw_rgb565_swap(pxMap, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));

    // Then draw the bitmap to the physical display (+1 needed, otherwise the image is distorted)
    esp_lcd_panel_draw_bitmap(lcdPanelHandle3_, area->x1, area->y1, area->x2 + 1, area->y2 + 1, pxMap);
    lv_display_flush_ready(display);
}

//! \brief Initializes the LVGL library
//! \retval A boolean indicating if the operation was successful
bool initLvgl(void) {
    lv_init();

    // Create the lvgl displays
    display1_ = lv_display_create(GUI_LCD_RES, GUI_LCD_RES);
    display2_ = lv_display_create(GUI_LCD_RES, GUI_LCD_RES);
    display3_ = lv_display_create(GUI_LCD_RES, GUI_LCD_RES);

    // Create the three draw buffers
    drawBuffer11_ = (uint16_t *) heap_caps_malloc(drawBufferSize_, MALLOC_CAP_SPIRAM);
    drawBuffer21_ = (uint16_t *) heap_caps_malloc(drawBufferSize_, MALLOC_CAP_SPIRAM);
    drawBuffer31_ = (uint16_t *) heap_caps_malloc(drawBufferSize_, MALLOC_CAP_SPIRAM);

    // Create the three secondary draw buffers
    drawBuffer12_ = (uint16_t *) heap_caps_malloc(drawBufferSize_, MALLOC_CAP_SPIRAM);
    drawBuffer22_ = (uint16_t *) heap_caps_malloc(drawBufferSize_, MALLOC_CAP_SPIRAM);
    drawBuffer32_ = (uint16_t *) heap_caps_malloc(drawBufferSize_, MALLOC_CAP_SPIRAM);

    // Check the draw buffers
    if (drawBuffer11_ == NULL || drawBuffer21_ == NULL || drawBuffer31_ == NULL) {
        // Logging
        loggerCritical("Failed to allocate display buffers!");
        return false;
    }

    // Clear the six buffers - they should sum up to around 0.69 mb
    memset(drawBuffer11_, 0, drawBufferSize_);
    memset(drawBuffer21_, 0, drawBufferSize_);
    memset(drawBuffer31_, 0, drawBufferSize_);
    memset(drawBuffer12_, 0, drawBufferSize_);
    memset(drawBuffer22_, 0, drawBufferSize_);
    memset(drawBuffer32_, 0, drawBufferSize_);

    // Draw a black screen to each display, otherwise they will start up with random colored pixels
    esp_lcd_panel_draw_bitmap(lcdPanelHandle1_, 0, 0, GUI_LCD_RES, GUI_LCD_RES, drawBuffer11_);
    esp_lcd_panel_draw_bitmap(lcdPanelHandle2_, 0, 0, GUI_LCD_RES, GUI_LCD_RES, drawBuffer21_);
    esp_lcd_panel_draw_bitmap(lcdPanelHandle3_, 0, 0, GUI_LCD_RES, GUI_LCD_RES, drawBuffer31_);

    // Pass LVGL the draw buffers
    lv_display_set_buffers(display1_, drawBuffer11_, drawBuffer12_, drawBufferSize_, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_buffers(display2_, drawBuffer21_, drawBuffer22_, drawBufferSize_, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_buffers(display3_, drawBuffer31_, drawBuffer32_, drawBufferSize_, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Set the color formats
    lv_display_set_color_format(display1_, LV_COLOR_FORMAT_RGB565);
    lv_display_set_color_format(display2_, LV_COLOR_FORMAT_RGB565);
    lv_display_set_color_format(display3_, LV_COLOR_FORMAT_RGB565);

    // Set the callback function, to draw to the physical displays
    lv_display_set_flush_cb(display1_, flushToDisplay1);
    lv_display_set_flush_cb(display2_, flushToDisplay2);
    lv_display_set_flush_cb(display3_, flushToDisplay3);

    // Set tick interface
    lv_tick_set_cb(xTaskGetTickCount);

    // Everything was successful
    return true;
}

bool guiInit(void) {
    // Initialize SPI bus
    const spi_bus_config_t spiBusConfig = {
            .sclk_io_num = GUI_GPIO_LCD_PCLK,
            .mosi_io_num = GUI_GPIO_LCD_DATA0,
            .miso_io_num = -1,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE//TEST_LCD_H_RES * 80 * sizeof(uint16_t),
    };

    // Check SPI bus initialization
    if (spi_bus_initialize(GUI_LCD_SPI_HOST, &spiBusConfig, SPI_DMA_CH_AUTO) != ESP_OK) {
        // Logging
        loggerCritical("Failed to initialize SPI bus");
        return false;
    }

    // Initialize the three displays
    if (!initDisplays()) {
        // Logging
        loggerCritical("Failed to initialize displays");

        return false;
    };

    // Initialize LVGL
    if (!initLvgl()) {
        // Logging
        loggerCritical("Failed to initialize LVGL");

        return false;
    };

    // Was everything successful?
    initSuccessful_ = true;
    return initSuccessful_;
}

void guiDeInit(void) {
    // Delete the draw buffers
    free(drawBuffer11_);
    free(drawBuffer21_);
    free(drawBuffer31_);
}