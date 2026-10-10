#include "Sensor/PassiveSensor.hpp"

// Project includes
#include "Events.hpp"

// espidf includes
#include "esp_event.h"
#include "esp_log.h"

/*
 *	constexpr
 */
constexpr auto TAG = "PassiveSensor";
constexpr double VOLTAGE = 3.3;
constexpr unsigned int ADC_SAMPLE_COUNT = 10;

/*
 *	Public Function Implementations
 */
PassiveSensor::PassiveSensor(SENSOR::TYPE type, gpio_num_t gpio, adc_channel_t adcChannel, adc_oneshot_unit_handle_t p_adc,
							 adc_unit_t unit) : Sensor(type)
{
	gpio_ = gpio;
	channel_ = adcChannel;
	adc_ = p_adc;
	unit_ = unit;

	// Configure the channel
	if (adc_oneshot_config_channel(adc_, channel_, &channelConfig_) != ESP_OK) {
		ESP_LOGW(TAG, "Couldn't set adc config for channel %d", channel_);
		return;
	}

	// Calibrate
	const adc_cali_curve_fitting_config_t calibrationConfig = {
		.unit_id = unit_,
		.chan = channel_,
		.atten = ADC_ATTEN_DB_12,
		.bitwidth = ADC_BITWIDTH_DEFAULT,
	};
	if (adc_cali_create_scheme_curve_fitting(&calibrationConfig, &calibrationHandle_) != ESP_OK) {
		ESP_LOGW(TAG, "Couldn't calibrate adc channel %d", channel_);
		return;
	}

	setup_ = true;
}

void PassiveSensor::read()
{
	if (!setup_) {
		return;
	}

	/*
	 *	Read value from ADC
	 */
	int value = 0;
	int valueSum = 0;
	int successfullReads = 0;

	// Take a sample amount
	for (unsigned int i = 0; i < ADC_SAMPLE_COUNT; i++) {
		// Read from the adc
		if (adc_oneshot_read(adc_, channel_, &value) == ESP_OK) {
			successfullReads++;
			valueSum += value;
			continue;
		}

		// Something went wrong
		ESP_LOGW(TAG, "Failed to read channel %d", channel_);
	}

	// Not a single read succeeded. Stop here!
	if (successfullReads == 0) {
		ESP_LOGW(TAG, "Failed to read any data from channel %d", channel_);
		return;
	}

	/*
	 *	Calculate the adc value to a voltage
	 */
	if (adc_cali_raw_to_voltage(calibrationHandle_, valueSum / successfullReads, &voltage_) != ESP_OK) {
		// Something went wrong. Stop here!
		voltage_ = 0;
		ESP_LOGW(TAG, "Couldn't convert adc channel %d value to voltage", channel_);
		return;
	}

	/*
	 *	Add an event to the loop if the value changed
	 */
	if (lastVoltage_ == voltage_) {
		return;
	}

	/*
	 *	Call the sensor specific read logic
	 */
	specificRead();

	lastVoltage_ = voltage_;
	notifyAboutNewValue();
}

int PassiveSensor::get() { return voltage_; }

double PassiveSensor::getResistance() const {
	return resistance_;
}

void PassiveSensor::specificRead() {}

double PassiveSensor::calcVoltageDividerR2(const int voltageMv, const int r1)
{
	const double vOut = static_cast<double>(voltageMv) / 1000.0;
	const double r = r1;

	// Prevents divison by 0
	if (vOut >= VOLTAGE) {
		return 0.0;
	}

	// R2 = R1 * (voltageMv / (preR1VoltageV - voltageMv))
	return r * (vOut / (VOLTAGE - vOut));
}

void PassiveSensor::notifyAboutNewValue()
{
	esp_event_post(SYSTEM_EVENT_BASE, PASSIVE_SENSOR_VALUE_CHANGED, &voltage_, sizeof(voltage_), portMAX_DELAY);
}
