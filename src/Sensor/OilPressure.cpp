#include "Sensor/OilPressure.hpp"

// Project includes
#include "Events.hpp"

// espidf includes
#include "esp_event.h"

/*
 *	constexpr
 */
constexpr uint16_t R1 = 240;
constexpr uint8_t ENGINE_OFF_MV = 200;	// no oil pressure -> 0 ohms
constexpr uint16_t ENGINE_ON_MV = 2800;  // with oil pressure -> 5.1k ohms

/*
 *	Public Function Implementations
 */
OilPressure::OilPressure(adc_oneshot_unit_handle_t p_adc) : PassiveSensor(SENSOR::TYPE::OIL_PRESSURE, GPIO_NUM_2, ADC_CHANNEL_1, p_adc, ADC_UNIT_1) {}

int OilPressure::get()
{
	return pressure_;
}

void OilPressure::specificRead()
{
    resistance_ = calcVoltageDividerR2(voltage_, R1);
	pressure_ = voltage_ > ENGINE_OFF_MV && voltage_ >= ENGINE_ON_MV;
}

void OilPressure::notifyAboutNewValue()
{
	esp_event_post(SYSTEM_EVENT_BASE, OIL_PRESSURE_CHANGED, &pressure_, sizeof(pressure_), portMAX_DELAY);
}
