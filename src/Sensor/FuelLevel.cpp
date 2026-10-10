#include "Sensor/FuelLevel.hpp"

// Project includes
#include "Events.hpp"

// C++ includes
#include <algorithm>
#include <cmath>

// espidf includes
#include "esp_event.h"
#include <esp_log.h>

/*
 *	Private typedefs
 */
typedef struct
{
	uint8_t level; // %
	float r;
} LevelResistanceTuple_t;

/*
 *	constexpr
 */
constexpr auto TAG = "FuelLevel";
constexpr uint16_t R1 = 240;
constexpr float NEW_VALUES_DAMPENER = 0.01f;
constexpr LevelResistanceTuple_t LEVEL_RESISTANCE_TUPLES[] = {{100, 3}, {75, 15.8}, {50, 32.5}, {25, 64.2}, {0, 110}};
constexpr uint8_t AMOUNT_LEVEL_TUPLES = std::size(LEVEL_RESISTANCE_TUPLES);

/*
 *	Public Function Implementations
 */
FuelLevel::FuelLevel(adc_oneshot_unit_handle_t p_adc) : PassiveSensor(SENSOR::TYPE::FUEL_LEVEL, GPIO_NUM_1, ADC_CHANNEL_0, p_adc) {}

int FuelLevel::get()
{
	// Calculate the level
	calcLevel();

	/*
	 *	Median Filter
	 */
	auto sortedLevels = lastLevels_;
	std::ranges::sort(sortedLevels.begin(), sortedLevels.end());

	// Get the element in the middle
	const float median = sortedLevels.at(static_cast<int>(sortedLevels.size() / 2));

	/*
	 *	Dampening new values
	 */
	// Initial value
	if (smoothedValue_ < 0) {
		smoothedValue_ = median;
	}

	// Calculate the dampened value
	smoothedValue_ = (NEW_VALUES_DAMPENER * median) + ((1.0f - NEW_VALUES_DAMPENER) * smoothedValue_);
	if (smoothedValue_ > 100.0f) {
		smoothedValue_ = 100.0f;
	}

	return static_cast<int>(smoothedValue_);
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

void FuelLevel::notifyAboutNewValue()
{
	const auto& value = get();

	esp_event_post(SYSTEM_EVENT_BASE, FUEL_LEVEL_CHANGED, &value, sizeof(value), portMAX_DELAY);
}
