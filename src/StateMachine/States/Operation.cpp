#include "StateMachine/States/Operation.hpp"

// Project includes
#include "StateMachine/StateEvents.hpp"

/*
 *   Public Function Implementations
 */
Operation::Operation(Can* p_can, GuiHandler* p_guiHandler) : MessageReceiver(p_can)
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
	switch (frame.function) {
		case CanFrameGroups::FUNC::ENTER_UPDATE_MODE: {
			esp_event_post(STATE_EVENT_BASE, ENTER_UPDATE_MODE, nullptr, 0, portMAX_DELAY);
			return;
		} break;

		case CanFrameGroups::FUNC::BROADCAST_DATA: {
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
		} break;

		default: {
		} break;
	}
}
