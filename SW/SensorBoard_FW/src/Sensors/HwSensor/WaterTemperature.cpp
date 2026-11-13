#include "Sensors/HwSensor/WaterTemperature.hpp"

// C++ includes
#include <cmath>

// espidf includes
#include "esp_event.h"
#include "esp_log.h"

/*
 *	constexpr
 */
constexpr auto TAG = "WaterTemperature";

constexpr uint16_t R1 = 240;

constexpr float A = 0.001883863080f;
constexpr float B = 0.0002251572018f;
constexpr float C = -0.00000003117864586f;

/*
 *	Public Function Implementations
 */
WaterTemperature::WaterTemperature(adc_oneshot_unit_handle_t p_adc) : PassiveSensor(GPIO_NUM_6, ADC_CHANNEL_5, p_adc) {}

int WaterTemperature::get() { return calculateTemp(resistance_); }

void WaterTemperature::specificRead() { resistance_ = calcVoltageDividerR2(voltage_, R1); }

/*
 *	Private Function Implementations
 */
int WaterTemperature::calculateTemp(const float r)
{
	if (r <= 10.0f) {
		return 0.0f;
	}

	const float logR = std::log(r);
	const float kelvin = 1 / (A + B * logR + C * std::pow(logR, 3));

	return std::lround(kelvin - 273.15f);
}
