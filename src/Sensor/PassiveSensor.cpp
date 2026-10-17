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
constexpr unsigned int ADC_SAMPLE_SIZE = 10;

/*
 *	Public Function Implementations
 */
PassiveSensor::PassiveSensor(gpio_num_t gpio, adc_channel_t adcChannel, adc_oneshot_unit_handle_t p_adc,
							 adc_unit_t unit)
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

	// Get a sample
	const int sample = readSample(ADC_SAMPLE_SIZE);

	// Convert the adc value to a voltage
	voltage_ = convertSampleToVoltage(sample);

	// Call the sensor specific read logic
	specificRead();
}

/*
 *	Private Function Implementations
 */
uint16_t PassiveSensor::calcVoltageDividerR2(const int voltageMv, const int r1)
{
	const double vOut = static_cast<double>(voltageMv) / 1000.0;
	const double r = r1;

	// Prevents divison by 0
	if (vOut >= VOLTAGE) {
		return 0.0;
	}

	// R2 = R1 * (voltageMv / (preR1VoltageV - voltageMv))
	return static_cast<uint16_t>(r * (vOut / (VOLTAGE - vOut)));
}

int PassiveSensor::readSample(const unsigned int sampleSize)
{
	int sample = 0;
	int allSamples = 0;
	int successfullReads = 0;

	// Take a sample amount
	for (unsigned int i = 0; i < sampleSize; i++) {
		if (adc_oneshot_read(adc_, channel_, &sample) != ESP_OK) {
			continue;
		}

		// Add to sample data
		allSamples += sample;
		successfullReads++;
	}

	// Something went wrong
	if (successfullReads == 0) {
		ESP_LOGW(TAG, "Failed to read any sample from channel %d", channel_);
		return 0;
	}

	return allSamples / successfullReads;
}

int PassiveSensor::convertSampleToVoltage(const int sample)
{
	int voltage = 0;

	if (adc_cali_raw_to_voltage(calibrationHandle_, sample, &voltage_) != ESP_OK) {
		// Something went wrong
		ESP_LOGW(TAG, "Couldn't convert adc channel %d value to voltage", channel_);
		return 0;
	}

	return voltage;
}
