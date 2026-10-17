#include "Sensor/OilPressure.hpp"

// Project includes
#include "Events.hpp"

// espidf includes
#include "esp_event.h"

/*
 *	constexpr
 */
constexpr uint16_t R1 = 240;
constexpr uint8_t ENGINE_OFF_MV = 200; // no oil pressure -> 0 ohms
constexpr uint16_t ENGINE_ON_MV = 2800; // with oil pressure -> 5.1k ohms

/*
 *	Public Function Implementations
 */
OilPressure::OilPressure(adc_oneshot_unit_handle_t p_adc) : PassiveSensor(GPIO_NUM_2, ADC_CHANNEL_1, p_adc, ADC_UNIT_1)
{
}

int OilPressure::get() { return static_cast<int>(pressure_); }

void OilPressure::specificRead()
{
	// Calculate resistance
	resistance_ = calcVoltageDividerR2(voltage_, R1);

	// Check if its threshhold
	pressure_ = voltage_ > ENGINE_OFF_MV && voltage_ >= ENGINE_ON_MV;
}
