#include "Sensors/HwSensor/WaterTemperature.hpp"

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

constexpr uint16_t R1 = 240;
constexpr TempResistanceTuple_t TEMP_RESISTANCE_TUPLES[] = {
	{2, 1850}, {8, 1500}, {12, 1200}, {16, 1000}, {20, 800}, {25, 640}, {29, 520},
	{32, 430}, {36, 300}, {39, 250},  {43, 200},  {48, 150}, {53, 120}, {56, 100},
	{61, 80},  {64, 60},  {68, 45},	  {73, 30},	  {77, 20},	 {82, 10},	{84, 5},
};
constexpr uint8_t AMOUNT_TEMP_TUPLES = std::size(TEMP_RESISTANCE_TUPLES);

/*
 *	Public Function Implementations
 */
WaterTemperature::WaterTemperature(adc_oneshot_unit_handle_t p_adc) : PassiveSensor(GPIO_NUM_6, ADC_CHANNEL_5, p_adc) {}

int WaterTemperature::get() { return calculateTemp(resistance_); }

void WaterTemperature::specificRead() { resistance_ = calcVoltageDividerR2(voltage_, R1); }

/*
 *	Private Function Implementations
 */
int WaterTemperature::calculateTemp(const uint16_t r)
{
	int temp = 0;

	// Below our range
	if (r > TEMP_RESISTANCE_TUPLES[0].r) {
		temp = TEMP_RESISTANCE_TUPLES[0].temp;
		return temp;
	}

	// Above our range
	if (r < TEMP_RESISTANCE_TUPLES[AMOUNT_TEMP_TUPLES - 1].r) {
		temp = TEMP_RESISTANCE_TUPLES[AMOUNT_TEMP_TUPLES - 1].temp + 1;
		return temp;
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
			temp = temp1 + ((r - r1) * ((temp2 - temp1)) / (r2 - r1));
			return temp;
		}
	}

	return temp;
}
