#include "Sensors/HwSensor/FuelLevel.hpp"

// C++ includes
#include <cmath>

// espidf includes
#include "esp_event.h"
#include "esp_log.h"

/*
 *	constexpr
 */
constexpr auto TAG = "FuelLevel";

constexpr uint16_t R1 = 240;
constexpr float BOOST_RESISTOR = 96.8f;
constexpr float EMA_ALPHA = 0.01f;

constexpr auto NVS_NAMESPACE = "FuelLevel";
constexpr auto NVS_LAST_VALUE = "LastValue";

constexpr uint8_t VOLTAGE_OFFSET_WHEN_ENGINE_RUNNING_MV = 27;
constexpr float HYSTERESE_THRESHOLD = 0.3f;

/*
 *	Public Function Implementations
 */
FuelLevel::FuelLevel(adc_oneshot_unit_handle_t p_adc) : PassiveSensor(GPIO_NUM_1, ADC_CHANNEL_0, p_adc, ADC_ATTEN_DB_6)
{
	nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvsHandle_);

	// Load last value
	uint8_t lastValue = 0;
	if (nvsHandle_ != 0) {
		nvs_get_u8(nvsHandle_, NVS_LAST_VALUE, &lastValue);
	}
	levelInPercent_ = static_cast<float>(lastValue);
	displayedLevelInPercent_ = lastValue;
}

FuelLevel::~FuelLevel() { nvs_close(nvsHandle_); }

int FuelLevel::get() { return displayedLevelInPercent_; }

void FuelLevel::setEngineRunning(bool running) { engineRunning_ = running; }

/*
 *	Private Function Implementations
 */
void FuelLevel::specificRead()
{
	resistance_ = filterResistance();

	const float percentage = calculatePercentage();

	// Xt = a * xt + (1 - a) * Xt-1
	// Xt = new value
	// xt = new data point
	// xt-1 = previous data point
	levelInPercent_ = (EMA_ALPHA * percentage) + ((1.0f - EMA_ALPHA) * levelInPercent_);

	// Hysterese
	if (std::abs(levelInPercent_ - static_cast<float>(displayedLevelInPercent_)) > 0.5f + HYSTERESE_THRESHOLD) {
		displayedLevelInPercent_ = std::lround(levelInPercent_);

		if (nvsHandle_ != 0) {
			nvs_set_u8(nvsHandle_, NVS_LAST_VALUE, static_cast<uint8_t>(displayedLevelInPercent_));
			nvs_commit(nvsHandle_);
		}
	}

	firstRead_ = false;
}

float FuelLevel::filterResistance()
{
	resistance_ =
		calcVoltageDividerR2(engineRunning_ ? voltage_ - VOLTAGE_OFFSET_WHEN_ENGINE_RUNNING_MV : voltage_, R1) -
		BOOST_RESISTOR;

	// Use the median filter
	if (firstRead_) {
		medianFilter_.fill(resistance_);
	}
	medianFilter_.add(resistance_);
	return medianFilter_.get();
}

float FuelLevel::calculatePercentage()
{
	float percent = (-0.000102885f * resistance_ * resistance_ * resistance_) +
		(0.0246611f * resistance_ * resistance_) + (-2.44333f * resistance_) + 107.321f;

	// Too low
	if (percent < 0.0f) {
		percent = 0.0f;
	}

	// Too high
	if (percent > 100.0f) {
		percent = 100.0f;
	}

	// If its the first read and the read value is 15% higher (or more),
	// override the ema value with it
	if (firstRead_ && levelInPercent_ < percent * 0.85) {
		levelInPercent_ = percent;
	}

	return percent;
}
