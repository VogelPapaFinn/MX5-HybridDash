#include "StateMachine/States/Operation.hpp"

/*
 *   Public Function Implementations
 */
Operation::Operation(Can* p_can, GuiHandler* p_guiHandler) : MessageReceiver(p_can, CanFrameGroups::GROUP::SENSOR)
{
	can_ = p_can;
	guiHandler_ = p_guiHandler;
}

void Operation::enter() { startCanRxTask(); }

void Operation::quit() { stopCanRxTask(); }

/*
 *   Private Function Implementations
 */
void Operation::handleCanFrame(const Can::Frame_t& frame)
{
	// Ignore irrelevant frames
	if (frame.group != CanFrameGroups::GROUP::SENSOR) {
		return;
	}

	if (frame.function != CanFrameGroups::SENSOR::BROADCAST_DATA) {
		return;
	}

	// Get data
	const uint8_t fuelLevelP = frame.data[0];
	const bool oilPressureB = frame.data[1];
	const uint8_t waterTempC = frame.data[2];
	const uint16_t rpm = (frame.data[3] << 8) + frame.data[4];
	const uint8_t speed = frame.data[5];
	const bool leftIndic = frame.data[6];
	const bool rightIndic = frame.data[7];

	// Update GUI
	guiHandler_->setFuelLevel(fuelLevelP);
	guiHandler_->setOilPressure(oilPressureB);
	guiHandler_->setWaterTemperature(waterTempC);
	guiHandler_->setRpm(rpm);
	guiHandler_->setSpeed(speed);
	guiHandler_->setLeftIndicatorActive(leftIndic);
	guiHandler_->setRightIndicatorActive(rightIndic);
}
