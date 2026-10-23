#include "Driver/ST77916.hpp"

// Project includes
#include "Driver/ST77916_InitSequence.h"

// espidf includes
#include "display/lv_display.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "soc/gpio_num.h"

/*
 *	constexpr
 */
constexpr auto TAG = "ST77916";

constexpr spi_host_device_t SPI_HOST = SPI2_HOST;

constexpr gpio_num_t GPIO_CLK = GPIO_NUM_10;
constexpr gpio_num_t GPIO_D0 = GPIO_NUM_14;
constexpr gpio_num_t GPIO_D1 = GPIO_NUM_13;
constexpr gpio_num_t GPIO_D2 = GPIO_NUM_12;
constexpr gpio_num_t GPIO_D3 = GPIO_NUM_11;
constexpr gpio_num_t GPIO_CS = GPIO_NUM_8;
constexpr gpio_num_t GPIO_RST = GPIO_NUM_9;
constexpr gpio_num_t GPIO_TE = GPIO_NUM_18;

constexpr gpio_num_t GPIO_BL = GPIO_NUM_7;
constexpr uint16_t BL_PWM_FQ_HZ = 5000;
constexpr ledc_timer_t BL_PWM_TIMER = LEDC_TIMER_0;
constexpr ledc_mode_t BL_PWM_MODE = LEDC_LOW_SPEED_MODE;
constexpr ledc_channel_t BL_PWM_CHANNEL = LEDC_CHANNEL_0;
constexpr ledc_timer_bit_t BL_PWM_RES = LEDC_TIMER_8_BIT;
constexpr ledc_timer_config_t LEDC_TIMER_CONFIG = {.speed_mode = BL_PWM_MODE,
												   .duty_resolution = BL_PWM_RES,
												   .timer_num = BL_PWM_TIMER,
												   .freq_hz = BL_PWM_FQ_HZ,
												   .clk_cfg = LEDC_AUTO_CLK};
constexpr ledc_channel_config_t LEDC_CHANNEL_CONFIG = {.gpio_num = GPIO_BL,
													   .speed_mode = BL_PWM_MODE,
													   .channel = BL_PWM_CHANNEL,
													   .intr_type = LEDC_INTR_DISABLE,
													   .timer_sel = BL_PWM_TIMER,
													   .duty = 0,
													   .hpoint = 0};

constexpr st77916_vendor_config_t VENDOR_CONFIG = {
	.init_cmds = ST77916_INIT_SEQUENCE,
	.init_cmds_size = sizeof(ST77916_INIT_SEQUENCE) / sizeof(st77916_lcd_init_cmd_t),
	.flags =
		{
			.use_qspi_interface = 1,
		},
};
constexpr esp_lcd_panel_dev_config_t PANEL_CONFIG = {
	.reset_gpio_num = GPIO_RST,
	.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
	.bits_per_pixel = 16,
	.vendor_config = (void*)&VENDOR_CONFIG,
};
static esp_lcd_panel_io_spi_config_t g_ioConfig = {};

constexpr spi_bus_config_t QSPI_BUS_CONFIG = {.data0_io_num = GPIO_D0,
											  .data1_io_num = GPIO_D1,
											  .sclk_io_num = GPIO_CLK,
											  .data2_io_num = GPIO_D2,
											  .data3_io_num = GPIO_D3,
											  .max_transfer_sz = 4092};

/*
 *	Public Function Implementations
 */
ST77916::ST77916()
{
	if (!initSpiBus()) {
		ESP_LOGE(TAG, "Failed to initialize the SPI bus");
		return;
	}

	if (!initPanelIo()) {
		ESP_LOGE(TAG, "Failed to initialize LCD panel IO handle");
		return;
	}

	if (!initLcdPanel()) {
		ESP_LOGE(TAG, "Failed to initialize LCD panel");
		return;
	}

	spiMutex_ = xSemaphoreCreateMutex();

	if (!setupBacklight()) {
		ESP_LOGE(TAG, "Failed to setup Backlight controlling");
		return;
	}

	if (!setupTeGpio()) {
		ESP_LOGE(TAG, "Failed to setup TE GPIO");
		return;
	}

	// Start flush task
	if (xTaskCreate(flushToDisplayTask, "FlushToDisplay", 4096, this, 4, &drawToDisplayTaskHandle_) != pdPASS) {
		ESP_LOGE(TAG, "Failed to create draw to display task");
		esp_restart();
	}

	initialized_ = true;
}

void ST77916::setLvglDisplay(lv_display_t* p_lvDisplay) { lvDisplay_ = p_lvDisplay; }

void ST77916::setBacklightLevel(uint8_t percent)
{
	if (percent >= 100) {
		percent = 100;
	}

	constexpr uint32_t maxDuty = (1 << BL_PWM_RES) - 1;
	const uint32_t duty = (percent * maxDuty) / 100;

	// Neuen PWM-Wert setzen und anwenden
	ledc_set_duty(BL_PWM_MODE, BL_PWM_CHANNEL, duty);
	ledc_update_duty(BL_PWM_MODE, BL_PWM_CHANNEL);
}

void ST77916::drawBitmap(const lv_area_t* p_area, uint8_t* p_pxMap)
{
	drawData_.area = *p_area;
	drawData_.pixelData = p_pxMap;
	drawData_.valid = true;
}

void ST77916::setRotated(const bool& rotated) const
{
	if (spiMutex_ == nullptr) {
		return;
	}

	// Wait until frame was drawn
	xSemaphoreTake(spiMutex_, portMAX_DELAY);
	while (isDrawing_) {
		vTaskDelay(pdMS_TO_TICKS(1));
	}

	// Rotate the display
	if (esp_lcd_panel_mirror(panelHandle_, rotated, rotated) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to rotate LCD panel");
	}

	xSemaphoreGive(spiMutex_);
}

bool ST77916::initSpiBus() { return spi_bus_initialize(SPI_HOST, &QSPI_BUS_CONFIG, SPI_DMA_CH_AUTO) == ESP_OK; }

bool ST77916::initPanelIo()
{
	g_ioConfig = ST77916_PANEL_IO_QSPI_CONFIG(GPIO_CS, nullptr, this);
	g_ioConfig.on_color_trans_done = onFrameDrawn;

	return esp_lcd_new_panel_io_spi(SPI_HOST, &g_ioConfig, &ioHandle_) == ESP_OK;
}

bool ST77916::initLcdPanel()
{
	if (esp_lcd_new_panel_st77916(ioHandle_, &PANEL_CONFIG, &panelHandle_) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to create LCD panel");
		return false;
	}

	if (esp_lcd_panel_reset(panelHandle_) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to reset LCD panel");
		return false;
	}
	if (esp_lcd_panel_init(panelHandle_) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize LCD panel");
		return false;
	}
	if (esp_lcd_panel_disp_on_off(panelHandle_, true) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to turn on LCD panel");
		return false;
	}

	return true;
}

bool ST77916::setupBacklight()
{
	if (ledc_timer_config(&LEDC_TIMER_CONFIG) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize BL timer");
		return false;
	}

	if (ledc_channel_config(&LEDC_CHANNEL_CONFIG) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize the BL timer channel");
		return false;
	}

	return true;
}

bool ST77916::setupTeGpio()
{
	if (gpio_install_isr_service(ESP_INTR_FLAG_IRAM) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to install the ISR service");
		return false;
	}

	if (gpio_set_direction(GPIO_TE, GPIO_MODE_INPUT) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to set the GPIO direction of the TE GPIO");
		return false;
	}

	if (gpio_set_intr_type(GPIO_TE, GPIO_INTR_POSEDGE) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to set the GPIO interruption type of the TE GPIO");
		return false;
	}

	if (gpio_isr_handler_add(GPIO_TE, teGpioIsr, this->drawToDisplayTaskHandle_) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to enable the ISR");
		return false;
	}

	return true;
}

/*
 *	Private Static Function Implementations
 */
void ST77916::teGpioIsr(void* p_arg)
{
	if (p_arg == nullptr) {
		return;
	}

	const auto taskHandle = static_cast<TaskHandle_t>(p_arg);

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	vTaskNotifyGiveFromISR(taskHandle, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void ST77916::flushToDisplayTask(void* p_param)
{
	if (p_param == nullptr) {
		return;
	}
	const auto driver = static_cast<ST77916*>(p_param);

	while (true) {
		// Task gets notified when new data is available
		if (!ulTaskNotifyTake(pdTRUE, portMAX_DELAY)) {
			continue;
		}

		// Ignore invalid data
		if (!driver->drawData_.valid) {
			continue;
		}

		xSemaphoreTake(driver->spiMutex_, portMAX_DELAY);
		driver->isDrawing_ = true;

		// Draw data into the physical display buffer
		esp_lcd_panel_draw_bitmap(driver->panelHandle_, driver->drawData_.area.x1, driver->drawData_.area.y1,
								  driver->drawData_.area.x2 + 1, driver->drawData_.area.y2 + 1,
								  driver->drawData_.pixelData);
		driver->drawData_.valid = false;

		xSemaphoreGive(driver->spiMutex_);
	}
}

bool ST77916::onFrameDrawn(esp_lcd_panel_io_handle_t p_panelIo, esp_lcd_panel_io_event_data_t* p_edata, void* p_userCtx)
{
	const auto st77916 = static_cast<ST77916*>(p_userCtx);
	if (st77916 == nullptr) {
		return false;
	}

	st77916->isDrawing_ = false;
	lv_display_flush_ready(st77916->lvDisplay_);

	return false;
}
