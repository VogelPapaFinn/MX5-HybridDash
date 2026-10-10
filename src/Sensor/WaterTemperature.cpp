#include "Sensor/WaterTemperature.hpp"

// Project includes
#include "Events.hpp"

// C++ includes
#include <iterator>

// espidf includes
#include "esp_event.h"
#include "esp_log.h"

/*
 *	Private typedefs
 */
typedef struct
{
	uint8_t temp;
	uint16_t r;
} TempResistanceTuple_t;

/*
 *	constexpr
 */
constexpr auto TAG = "WaterTemperature";

constexpr uint16_t R1 = 3000;

constexpr TempResistanceTuple_t TEMP_RESISTANCE_TUPLES[] = {
	{0, 5743}, {5, 4627}, {10, 3749}, {15, 3053}, {20, 2499}, {25, 2056}, {30, 1700}, {35, 1412}, {40, 1178},
	{45, 987}, {50, 830}, {55, 701},  {60, 595},  {65, 507},  {70, 433},  {75, 371},  {80, 319},  {85, 276},
	{90, 239}, {95, 208}, {100, 181}, {105, 158}, {110, 139}, {115, 122}, {120, 108}};
constexpr uint8_t AMOUNT_TEMP_TUPLES = std::size(TEMP_RESISTANCE_TUPLES);

/*
 *	Public Function Implementations
 */
WaterTemperature::WaterTemperature(adc_oneshot_unit_handle_t p_adc) :
	PassiveSensor(SENSOR::TYPE::WATER_TEMPERATURE, GPIO_NUM_6, ADC_CHANNEL_5, p_adc)
{
}

int WaterTemperature::get() { return temperature_; }

void WaterTemperature::specificRead() { resistance_ = calcVoltageDividerR2(voltage_, R1); }

/*
 *	Private Function Implementations
 */
void WaterTemperature::calcTemperature(const uint16_t r)
{
	// Below our range
	if (r > TEMP_RESISTANCE_TUPLES[0].r) {
		temperature_ = TEMP_RESISTANCE_TUPLES[0].temp;
		return;
	}

	// Above our range
	if (r < TEMP_RESISTANCE_TUPLES[AMOUNT_TEMP_TUPLES - 1].r) {
		temperature_ = TEMP_RESISTANCE_TUPLES[AMOUNT_TEMP_TUPLES - 1].temp + 1;
		return;
	}
	// Iterate through all entries
	for (int i = 0; i < AMOUNT_TEMP_TUPLES - 1; i++) {
		const uint16_t r1 = TEMP_RESISTANCE_TUPLES[i].r;
		const uint16_t r2 = TEMP_RESISTANCE_TUPLES[i + 1].r;

		// Check if the passed resistance is between this and the next entry
		if (r <= r1 && r >= r2) {
			const uint8_t temp1 = TEMP_RESISTANCE_TUPLES[i].temp;
			const uint8_t temp2 = TEMP_RESISTANCE_TUPLES[i + 1].temp;

			// Calculate the value with linear interpolation
			// y = y1 + (x - x1) * (y2 - y1) / (x2 - x1)
			temperature_ = temp1 + ((r - r1) * ((temp2 - temp1)) / (r2 - r1));
			return;
		}
	}
}

void WaterTemperature::notifyAboutNewValue()
{
	calcTemperature(resistance_);
	const auto& value = get();

	esp_event_post(SYSTEM_EVENT_BASE, WATER_TEMP_CHANGED, &value, sizeof(value), portMAX_DELAY);
}
