#include "Sensors/HwSensor/FuelLevel.hpp"

// C++ includes
#include <algorithm>
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
constexpr float EMA_ALPHA = 0.005f;

constexpr auto NVS_NAMESPACE = "FuelLevel";
constexpr auto NVS_LAST_VALUE = "LastValue";

constexpr uint8_t VOLTAGE_OFFSET_WHEN_ENGINE_RUNNING_MV = 27;

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
	emaValue_ = static_cast<float>(lastValue);
}

FuelLevel::~FuelLevel() { nvs_close(nvsHandle_); }

int FuelLevel::get()
{
	const auto level = static_cast<int>(std::lround(emaValue_));

	if (nvsHandle_ != 0) {
		nvs_set_u8(nvsHandle_, NVS_LAST_VALUE, static_cast<uint8_t>(level));
		nvs_commit(nvsHandle_);
	}

	return level;
}

void FuelLevel::setEngineRunning(bool running) { engineRunning_ = running; }

/*
 *	Private Function Implementations
 */
void FuelLevel::specificRead()
{
	resistance_ =
		calcVoltageDividerR2(engineRunning_ ? voltage_ - VOLTAGE_OFFSET_WHEN_ENGINE_RUNNING_MV : voltage_, R1) -
		BOOST_RESISTOR;

	float levelInPercent = (-0.000102885f * resistance_ * resistance_ * resistance_) +
		(0.0246611f * resistance_ * resistance_) + (-2.44333f * resistance_) + 107.321f;

	// Too low
	if (levelInPercent < 0.0f) {
		levelInPercent = 0.0f;
	}

	// Too high
	if (levelInPercent > 100.0f) {
		levelInPercent = 100.0f;
	}

	// If its the first read and the read value is 15% higher (or more),
	// override the ema value with it
	if (firstRead_) {
		if (emaValue_ < levelInPercent * 0.85) {
			emaValue_ = levelInPercent;
		}

		firstRead_ = false;
	}

	// Xt = a * xt + (1 - a) * Xt-1
	// Xt = new value
	// xt = new data point
	// xt-1 = previous data point
	emaValue_ = (EMA_ALPHA * levelInPercent) + ((1.0f - EMA_ALPHA) * emaValue_);
}
