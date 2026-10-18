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
constexpr float NEW_VALUES_DAMPENER = 0.01f;

/*
 *	Public Function Implementations
 */
FuelLevel::FuelLevel(adc_oneshot_unit_handle_t p_adc) : PassiveSensor(GPIO_NUM_1, ADC_CHANNEL_0, p_adc) {}

int FuelLevel::get()
{
	calcLevel();

	const float median = getMedianElement(lastLevels_);

	return getDampenedValue(median);
}

/*
 *	Private Function Implementations
 */
void FuelLevel::specificRead() { resistance_ = calcVoltageDividerR2(voltage_, R1); }

void FuelLevel::calcLevel()
{
	int levelInPercent = std::round((-0.000102885 * resistance_ * resistance_ * resistance_) +
									(0.0246611 * resistance_ * resistance_) + (-2.44333 * resistance_) + 107.321);

	// Too low
	if (levelInPercent < 0.0) {
		levelInPercent = 0.0;
	}

	// Too high
	if (levelInPercent > 100.0) {
		levelInPercent = 100.0;
	}

	// Track the
	if (lastLevels_.size() >= 21) {
		lastLevels_.erase(lastLevels_.begin());
	}
	lastLevels_.push_back(levelInPercent);
}

float FuelLevel::getMedianElement(std::vector<uint8_t>& values)
{
	auto sortedLevels = values;

	// Sort - increasing
	std::ranges::sort(sortedLevels.begin(), sortedLevels.end());

	// Return median element
	return sortedLevels.at(static_cast<int>(sortedLevels.size() / 2));
}

int FuelLevel::getDampenedValue(const float raw)
{
	float smoothed = raw;

	// Calculate the dampened value
	smoothed = (NEW_VALUES_DAMPENER * raw) + ((1.0f - NEW_VALUES_DAMPENER) * smoothed);

	// Prevent unlogical value
	if (smoothed > 100.0f) {
		smoothed = 100.0f;
	}

	return static_cast<int>(smoothed);
}
