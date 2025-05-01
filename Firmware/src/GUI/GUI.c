/* --- Includes --- */
#include "GUI/GUI.h"

#include <Logger/Logger.h>

/* --- Private Defines & Macros --- */

/* --- Private Variables, Typedefs etc. --- */

// General display stuff
const size_t drawBufferSize_ = GUI_LCD_RES * GUI_LCD_RES * 2;
SemaphoreHandle_t semaphoreLvTaskHandle_;

// Display 1 management stuff
esp_lcd_panel_io_handle_t lcdPanelIoHandle1_ = NULL;
esp_lcd_panel_handle_t lcdPanelHandle1_ = NULL;
lv_display_t *display1_ = NULL;
uint16_t *drawBuffer11_ = NULL;
uint16_t *drawBuffer12_ = NULL;
bool firstFrameDrawnD1_ = false;

// Display 2 management stuff
esp_lcd_panel_io_handle_t lcdPanelIoHandle2_ = NULL;
esp_lcd_panel_handle_t lcdPanelHandle2_ = NULL;
lv_display_t *display2_ = NULL;
uint16_t *drawBuffer21_ = NULL;
uint16_t *drawBuffer22_ = NULL;
bool firstFrameDrawnD2_ = false;

// Display 3 management stuff
esp_lcd_panel_io_handle_t lcdPanelIoHandle3_ = NULL;
esp_lcd_panel_handle_t lcdPanelHandle3_ = NULL;
lv_display_t *display3_ = NULL;
uint16_t *drawBuffer31_ = NULL;
uint16_t *drawBuffer32_ = NULL;
bool firstFrameDrawnD3_ = false;

// Variables indicating stati
bool initSuccessful_ = false;
int waitForFirstFrameCounter_ = 0;

/* --- Private Variables: GUI --- */

// Screen 1 - SPEEDOMETER
lv_obj_t *speedLabel_ = NULL;
lv_style_t speedLabelStyle_;
lv_obj_t *kmhLabel_ = NULL;
lv_style_t kmhLabelStyle_;
lv_obj_t *blinkerRight_ = NULL;

// Screen 2 - RPM
lv_obj_t *rpmLabel_ = NULL;
lv_style_t rpmLabelStyle_;
lv_obj_t *rpmTitleLabel_ = NULL;
lv_style_t rpmTitleStyle_;
lv_obj_t *blinkerLeft_ = NULL;

// Screen 3 - Temp and Fuel
lv_obj_t *tempLabel_ = NULL;
lv_style_t tempLabelStyle_;
lv_obj_t *celsiusLabel_ = NULL;
lv_style_t celsiusStyle_;
lv_obj_t *fuelLevelArcs_[10];
lv_style_t fuelLevelArcStyle_;
lv_obj_t *fuelLevelInPercentLabel_ = NULL;
lv_obj_t *fuelLevelInLitreLabel_ = NULL;
lv_style_t fuelLevelLabelStyle_;
int lastFuelInPercent_ = -1;

/* --- Private function prototypes --- */

//! \brief Initializes the three SPI displays
//! \retval A boolean indicating if the operation was successful
bool initDisplays(void);

//! \brief Callback function for LVGL to draw to the first physical display
//! \param display A pointer to the lvgl display which is drawn too
//! \param area The area which is updated
//! \param pxMap An array which contains the colors for each pixel
void flushToDisplay1(lv_display_t *display, const lv_area_t *area, uint8_t *pxMap);

//! \brief Callback function for LVGL to draw to the second physical display
//! \param display A pointer to the lvgl display which is drawn too
//! \param area The area which is updated
//! \param pxMap An array which contains the colors for each pixel
void flushToDisplay2(lv_display_t *display, const lv_area_t *area, uint8_t *pxMap);

//! \brief Callback function for LVGL to draw to the third physical display
//! \param display A pointer to the lvgl display which is drawn too
//! \param area The area which is updated
//! \param pxMap An array which contains the colors for each pixel
void flushToDisplay3(lv_display_t *display, const lv_area_t *area, uint8_t *pxMap);

//! \brief Initializes the LVGL library
//! \retval A boolean indicating if the operation was successful
bool initLvgl(void);

//! \brief Builds the screen containing the speedometer
//! \param display The display the screen should be displayed on
void createAndShowSpeedometerScreen(lv_display_t *display);

//! \brief Builds the screen containing the RPM
//! \param display The display the screen should be displayed on
void createAndShowRpmScreen(lv_display_t *display);

//! \brief Builds the screen containing the water temperature and fuel level
//! \param display The display the screen should be displayed on
void createAndShowTempScreen(lv_display_t *display);

/* --- Tasks --- */

//! \brief Task which is needed for lvgl to work
//! \param params void* needed for FreeRTOS to accept this function as task!
void IRAM_ATTR taskUpdateLvgl(void *params) {
    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        // Wait 10ms
        vTaskDelay(pdMS_TO_TICKS(10));

        // Try to get the semaphore mutex
        if (xSemaphoreTake(semaphoreLvTaskHandle_, portMAX_DELAY) == pdTRUE) {
            // Then run the lvgl task handler
            lv_task_handler();
            xSemaphoreGive(semaphoreLvTaskHandle_);
        }
    }
}

//! \brief A task started when LVGL is initialized. It checks if the first frame (for each)
//!        display was drawn and if so turn the display on. Otherwise, it will display nonsense
//!        at startup!
//! \param params void* needed for FreeRTOS to accept this function as task!
//! \note This task ends itself!
void IRAM_ATTR taskWaitForFirstFrameDrawn(void *params) {
    // Bools used to prevent double-checking
    bool display1Completed = false;
    bool display2Completed = false;
    bool display3Completed = false;

    while (1) {
        // Wait 100ms before next try
        vTaskDelay(pdMS_TO_TICKS(100));

        //! Check display 1
        if (firstFrameDrawnD1_ && !display1Completed) {
            ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcdPanelHandle1_, true));
            display1Completed = true;
        }

        //! Check display 2
        if (firstFrameDrawnD2_ && !display2Completed) {
            ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcdPanelHandle2_, true));
            display2Completed = true;
        }

        //! Check display 3
        if (firstFrameDrawnD3_ && !display3Completed) {
            ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcdPanelHandle3_, true));
            display3Completed = true;
        }

        // Check if counter > 10 was reached
        if (waitForFirstFrameCounter_++ > 10) {
            // Just turn them on, otherwise they will never light up
            ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcdPanelHandle1_, true));
            ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcdPanelHandle2_, true));
            ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcdPanelHandle3_, true));
            vTaskDelete(NULL);
        }
    }
}

/* --- Function implementations --- */

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

    // Everything was successful
    return true;
}

void flushToDisplay1(lv_display_t *display, const lv_area_t *area, uint8_t *pxMap) {
    // Swap the color channels as needed
    lv_draw_sw_rgb565_swap(pxMap, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));

    // Then draw the bitmap to the physical display (+1 needed, otherwise the image is distorted)
    esp_lcd_panel_draw_bitmap(lcdPanelHandle1_, area->x1, area->y1, area->x2 + 1, area->y2 + 1, pxMap);
    lv_display_flush_ready(display);
    firstFrameDrawnD1_ = true;
}

void flushToDisplay2(lv_display_t *display, const lv_area_t *area, uint8_t *pxMap) {
    // Swap the color channels as needed
    lv_draw_sw_rgb565_swap(pxMap, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));

    // Then draw the bitmap to the physical display (+1 needed, otherwise the image is distorted)
    esp_lcd_panel_draw_bitmap(lcdPanelHandle2_, area->x1, area->y1, area->x2 + 1, area->y2 + 1, pxMap);
    lv_display_flush_ready(display);
    firstFrameDrawnD2_ = true;
}

void flushToDisplay3(lv_display_t *display, const lv_area_t *area, uint8_t *pxMap) {
    // Swap the color channels as needed
    lv_draw_sw_rgb565_swap(pxMap, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));

    // Then draw the bitmap to the physical display (+1 needed, otherwise the image is distorted)
    esp_lcd_panel_draw_bitmap(lcdPanelHandle3_, area->x1, area->y1, area->x2 + 1, area->y2 + 1, pxMap);
    lv_display_flush_ready(display);
    firstFrameDrawnD3_ = true;
}

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

    // Then start the lvgl task handler task on core 1
    if (xTaskCreatePinnedToCore(taskUpdateLvgl, "taskUpdateLvgl", 10000, NULL, 0, NULL, 1) != pdPASS) {
        // Logging
        loggerCritical("Failed to create task: \"taskUpdateLvgl\"!");

        return false;
    }

    // Everything was successful
    return true;
}

void createAndShowSpeedometerScreen(lv_display_t *display) {
    // Get the pointer to the active screen
    lv_obj_t *screen = lv_display_get_screen_active(display);

    // Create the speedometer label
    speedLabel_ = lv_label_create(screen);

    // Include fonts
    LV_FONT_DECLARE(E1234_80_FONT);
    LV_FONT_DECLARE(VCR_OSD_MONO_24_FONT);

    // Apply the speedometer label style
    lv_style_init(&speedLabelStyle_);
    lv_style_set_text_color(&speedLabelStyle_, lv_color_hex(0x008F3C));
    lv_style_set_text_font(&speedLabelStyle_, &E1234_80_FONT);
    lv_obj_add_style(speedLabel_, &speedLabelStyle_, LV_PART_MAIN);

    // Center the label
    lv_obj_center(speedLabel_);

    // Set its text
    lv_label_set_text(speedLabel_, "200");

    // Create the kmh label
    kmhLabel_ = lv_label_create(screen);

    // Apply its style
    lv_style_init(&kmhLabelStyle_);
    lv_style_set_text_color(&kmhLabelStyle_, lv_color_hex(0x008F3C));
    lv_style_set_text_font(&kmhLabelStyle_, &VCR_OSD_MONO_24_FONT);
    lv_obj_add_style(kmhLabel_, &kmhLabelStyle_, LV_PART_MAIN);

    // Position it above the speedometer label
    lv_obj_align(kmhLabel_, LV_ALIGN_CENTER, 0, -80);

    // Set its text
    lv_label_set_text(kmhLabel_, "kmh");

    // Create the blinker right arrow
    LV_IMAGE_DECLARE(blinkerRight);
    blinkerRight_ = lv_image_create(screen);
    lv_image_set_src(blinkerRight_, &blinkerRight);

    // Position it centered at the bottom
    lv_obj_align(blinkerRight_, LV_ALIGN_CENTER, 0, 90);

    // Disable the blinker visually
    guiSetRightBlinkerActive(false);
}

void createAndShowRpmScreen(lv_display_t *display) {
    // Get the pointer to the active screen
    lv_obj_t *screen = lv_display_get_screen_active(display);

    // Create the rpm label
    rpmLabel_ = lv_label_create(screen);

    // Include fonts
    LV_FONT_DECLARE(E1234_70_FONT);
    LV_FONT_DECLARE(VCR_OSD_MONO_24_FONT);

    // Apply the rpm label style
    lv_style_init(&rpmLabelStyle_);
    lv_style_set_text_color(&rpmLabelStyle_, lv_color_hex(0x008F3C));
    lv_style_set_text_font(&rpmLabelStyle_, &E1234_70_FONT);
    lv_obj_add_style(rpmLabel_, &rpmLabelStyle_, LV_PART_MAIN);

    // Center the label
    lv_obj_center(rpmLabel_);

    // Set its text
    lv_label_set_text(rpmLabel_, "7700");

    // Create the rpm title label
    rpmTitleLabel_ = lv_label_create(screen);

    // Apply its style
    lv_style_init(&rpmTitleStyle_);
    lv_style_set_text_color(&rpmTitleStyle_, lv_color_hex(0x008F3C));
    lv_style_set_text_font(&rpmTitleStyle_, &VCR_OSD_MONO_24_FONT);
    lv_obj_add_style(rpmTitleLabel_, &rpmTitleStyle_, LV_PART_MAIN);

    // Position it above the rpm label
    lv_obj_align(rpmTitleLabel_, LV_ALIGN_CENTER, 0, -80);

    // Set its text
    lv_label_set_text(rpmTitleLabel_, "RPM");

    // Create the blinker left arrow
    LV_IMAGE_DECLARE(blinkerLeft);
    blinkerLeft_ = lv_image_create(screen);
    lv_image_set_src(blinkerLeft_, &blinkerLeft);

    // Position it centered at the bottom
    lv_obj_align(blinkerLeft_, LV_ALIGN_CENTER, 0, 90);

    // Disable the blinker visually
    guiSetLeftBlinkerActive(false);
}

void createAndShowTempScreen(lv_display_t *display) {
    // Get the pointer to the active screen
    lv_obj_t *screen = lv_display_get_screen_active(display);

    // Create the temp label
    tempLabel_ = lv_label_create(screen);

    // Include fonts
    LV_FONT_DECLARE(E1234_80_FONT);
    LV_FONT_DECLARE(VCR_OSD_MONO_24_FONT);

    // Apply the temp label style
    lv_style_init(&tempLabelStyle_);
    lv_style_set_text_color(&tempLabelStyle_, lv_color_hex(0x008F3C));
    lv_style_set_text_font(&tempLabelStyle_, &E1234_80_FONT);
    lv_obj_add_style(tempLabel_, &tempLabelStyle_, LV_PART_MAIN);

    // Center the label
    lv_obj_align(tempLabel_, LV_ALIGN_CENTER, 10, 0);

    // Set its text
    lv_label_set_text(tempLabel_, "90");

    // Create the temp title label
    celsiusLabel_ = lv_label_create(screen);

    // Apply its style
    lv_style_init(&celsiusStyle_);
    lv_style_set_text_color(&celsiusStyle_, lv_color_hex(0x008F3C));
    lv_style_set_text_font(&celsiusStyle_, &VCR_OSD_MONO_24_FONT);
    lv_obj_add_style(celsiusLabel_, &celsiusStyle_, LV_PART_MAIN);

    // Position it above the Celsius label
    lv_obj_align(celsiusLabel_, LV_ALIGN_RIGHT_MID, -10, 20);

    // Set its text
    lv_label_set_text(celsiusLabel_, "°C");

    // Create the arc background style
    lv_style_init(&fuelLevelArcStyle_);
    lv_style_set_arc_color(&fuelLevelArcStyle_, lv_color_hex(0x008F3C));
    lv_style_set_arc_rounded(&fuelLevelArcStyle_, false);

    // Create the 10 arcs for the fuel level
    for (int i = 0; i < 10; i++) {
        // Create the arc
        fuelLevelArcs_[i] = lv_arc_create(screen);

        // Set its dimensions
        lv_obj_set_width(fuelLevelArcs_[i], 220);
        lv_obj_set_height(fuelLevelArcs_[i], 220);
        lv_obj_set_style_arc_width(fuelLevelArcs_[i], 20, LV_PART_MAIN);

        // Position it
        lv_obj_center(fuelLevelArcs_[i]);
        lv_arc_set_bg_angles(fuelLevelArcs_[i], 0, 12);
        lv_arc_set_rotation(fuelLevelArcs_[i], 100 + i * 16);// Initial rotation + offset per arc
        lv_arc_set_value(fuelLevelArcs_[i], 100);

        // Remove the knob & indicator
        lv_obj_set_style_opa(fuelLevelArcs_[i], LV_OPA_0, LV_PART_KNOB);
        lv_obj_set_style_opa(fuelLevelArcs_[i], LV_OPA_0, LV_PART_INDICATOR);

        // Apply the background style
        lv_obj_add_style(fuelLevelArcs_[i], &fuelLevelArcStyle_, LV_PART_MAIN);

        // Color them accordingly
        if (i == 0) {
            lv_obj_set_style_arc_color(fuelLevelArcs_[i], lv_color_hex(0x992600), LV_PART_MAIN);
        } else if (i <= 2) {
            lv_obj_set_style_arc_color(fuelLevelArcs_[i], lv_color_hex(0xC69800), LV_PART_MAIN);
        }

        // Debugging / Testing Stuff
        // if (i > 6) {
        //     lv_obj_set_style_arc_opa(fuelLevelArcs_[i], LV_OPA_20, LV_PART_MAIN);
        // }
    }

    // Create the fuel in percent label
    fuelLevelInPercentLabel_ = lv_label_create(screen);

    // Apply the label style
    lv_style_init(&fuelLevelLabelStyle_);
    lv_style_set_text_color(&fuelLevelLabelStyle_, lv_color_hex(0x008F3C));
    lv_style_set_text_font(&fuelLevelLabelStyle_, &VCR_OSD_MONO_24_FONT);
    lv_obj_add_style(fuelLevelInPercentLabel_, &fuelLevelLabelStyle_, LV_PART_MAIN);

    // Position
    lv_obj_align(fuelLevelInPercentLabel_, LV_ALIGN_TOP_MID, 15, 30);

    // Set its text
    lv_label_set_text(fuelLevelInPercentLabel_, "100%");

    // Create the fuel in litre label
    fuelLevelInLitreLabel_ = lv_label_create(screen);

    // Apply the label style
    lv_obj_add_style(fuelLevelInLitreLabel_, &fuelLevelLabelStyle_, LV_PART_MAIN);

    // Position
    lv_obj_align(fuelLevelInLitreLabel_, LV_ALIGN_BOTTOM_MID, 15, -30);

    // Set its text
    lv_label_set_text(fuelLevelInLitreLabel_, "50L");
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

    // Create the Semaphore needed for the lvgl task handler
    semaphoreLvTaskHandle_ = xSemaphoreCreateMutex();

    // Initialize LVGL
    if (!initLvgl()) {
        // Logging
        loggerCritical("Failed to initialize LVGL");

        return false;
    }

    // Set the background for each display
    lv_obj_set_style_bg_color(lv_display_get_screen_active(display1_), lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_color(lv_display_get_screen_active(display2_), lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_color(lv_display_get_screen_active(display3_), lv_color_hex(0x000000), LV_PART_MAIN);

    // Start the turn on display task
    xTaskCreate(&taskWaitForFirstFrameDrawn, "taskWaitForFirstFrameDrawn", 2024, NULL, 0, NULL);

    // Build the screens and put them on the displays
    createAndShowSpeedometerScreen(display2_);
    createAndShowRpmScreen(display3_);
    createAndShowTempScreen(display1_);

    // Was everything successful?
    initSuccessful_ = true;
    return initSuccessful_;
}

void guiDeInit(void) {
    // Delete the draw buffers
    free(drawBuffer11_);
    free(drawBuffer21_);
    free(drawBuffer31_);
    free(drawBuffer12_);
    free(drawBuffer22_);
    free(drawBuffer32_);
}

void guiSetRightBlinkerActive(const bool active) {
    if (active) {
        // Set its opacity to 100% (active)
        lv_obj_set_style_opa(blinkerRight_, LV_OPA_100, LV_PART_MAIN);
    } else {
        // Set its opacity to 20% (inactive)
        lv_obj_set_style_opa(blinkerRight_, LV_OPA_20, LV_PART_MAIN);
    }
}

void guiSetLeftBlinkerActive(const bool active) {
    if (active) {
        // Set its opacity to 100% (active)
        lv_obj_set_style_opa(blinkerLeft_, LV_OPA_100, LV_PART_MAIN);
    } else {
        // Set its opacity to 20% (inactive)
        lv_obj_set_style_opa(blinkerLeft_, LV_OPA_20, LV_PART_MAIN);
    }
}

void guiSetOilPressure(void *pressure) {
    // TODO: Show the NO OIL PRESSURE screen or hide it
}

void guiSetFuelLevelPercent(void *percent) {
    // Save the old value
    lastFuelInPercent_ = (int) percent;

    // Set the new text
    lv_label_set_text_fmt(fuelLevelInPercentLabel_, "%d%%", (int) percent);

    // Check if the fuel level increased (by more than 5%)
    if (lastFuelInPercent_ < (int) percent + 5) {
        // Reactivate all fuel blocks
        for (int i = 0; i < 10; i++) {
            lv_obj_set_style_arc_opa(fuelLevelArcs_[i], LV_OPA_100, LV_PART_MAIN);
        }
    }

    // Update the fuel blocks
    const int currActiveBlock = ((int) percent + 10) / 10;
    for (int i = 9; i >= currActiveBlock; i--) {
        lv_obj_set_style_arc_opa(fuelLevelArcs_[i], LV_OPA_20, LV_PART_MAIN);
    }
}

void guiSetFuelLevelLitre(void *litres) {
    // Set the new text
    lv_label_set_text_fmt(fuelLevelInLitreLabel_, "%dL", (int) litres);
}

void guiSetWaterTemperature(void *temp) {
    // Set the new text
    lv_label_set_text_fmt(tempLabel_, "%d", (int) temp);
}

void guiSetSpeed(void *speed) {
    // Set the new text
    lv_label_set_text_fmt(speedLabel_, "%d", (int) speed);
}

void guiSetRpm(void *rpm) {
    // Set the new text
    lv_label_set_text_fmt(rpmLabel_, "%d", (int) rpm);
}