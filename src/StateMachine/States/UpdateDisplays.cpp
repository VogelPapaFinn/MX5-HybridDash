#include "StateMachine/States/UpdateDisplays.hpp"

// espidf includes
#include "esp_event.h"

/*
 *   constexpr
 */
constexpr auto TAG = "UpdateDisplays";

/*
 *	Static Functions
 */
void doAt3Call(const std::function<void()>& cb)
{
	static uint8_t s_counter = 0;
	if (++s_counter >= 3) {
		s_counter = 0;
		cb();
		return;
	}
}

/*
 *   Public Function Implementations
 */
UpdateDisplays::UpdateDisplays(Config* p_config, Can* p_can) : MessageReceiver()
{
	config_ = p_config;
	can_ = p_can;
}

void UpdateDisplays::enter() { startCanRxTask(); }

void UpdateDisplays::quit() { stopCanRxTask(); }

/*
 *   Private Function Implementations
 */
void UpdateDisplays::startCanRxTask()
{
	if (xTaskCreate(canRxTask, "UpdateDisplaysCanRxTask", 4096, this, 2, &canRxTask_) == pdPASS) {
		can_->attach(this, CanFrameGroups::GROUP::DISPLAY_UPDATE);

		// Success
		return;
	}

	// Fail
	canRxTask_ = nullptr;
	ESP_LOGW(TAG, "Failed to start task which updates the passive sensor values");
}

void UpdateDisplays::stopCanRxTask()
{
	if (canRxTask_ == nullptr) {
		return;
	}

	can_->detach(this, CanFrameGroups::GROUP::DISPLAY_UPDATE);


	vTaskDelete(canRxTask_);
	canRxTask_ = nullptr;
}

void UpdateDisplays::enterNextState()
{
	substate_++;

	switch (substate_) {
		case STARTING_AP: {
			startWifiAP();
		} break;

		case ENTERING_UPDATE_MODE: {
			putDisplaysIntoUpdateMode();
		} break;

		case TRANSMITTING_SSID: {
			const auto packages = getAsPackages(wifi_.getSsid());
			transmitSsidPackages(packages);
		} break;

		case TRANSMITTING_PSSWD: {
			const auto packages = getAsPackages(wifi_.getPassword());
			transmitPsswdPackages(packages);
		} break;

		case TRANSMITTING_HOST_IP: {
			transmitHostIp();
		} break;

		case JOIN_WIFI: {
			registerWifiEventHandlers();
			orderDisplaysToJoinWifi();
		} break;

		case EXECUTING_UPDATE: {
			unregisterWifiEventHandlers();
			orderDisplaysToExecuteUpdate();
		} break;

		default: {
		}
	}
}

void UpdateDisplays::handleCanFrame(const Can::Frame_t& frame)
{
	// Ignore irrelevant frames
	if (frame.group != CanFrameGroups::GROUP::DISPLAY_UPDATE) {
		return;
	}

	switch (substate_) {
		case STARTING_AP: {
			startWifiAP();
		} break;

		case ENTERING_UPDATE_MODE: {
			// Ignore irrelevant frames
			if (frame.function != CanFrameGroups::DISPLAY_UPDATE::ENTER_UPDATE_MODE) {
				return;
			}

			doAt3Call([this]() { enterNextState(); });
		} break;

		case TRANSMITTING_SSID: {
			// Ignore irrelevant frames
			if (frame.function != CanFrameGroups::DISPLAY_UPDATE::CONFIRM_SSID && frame.answer != true) {
				return;
			}

			doAt3Call([this]() { enterNextState(); });
		} break;

		case TRANSMITTING_PSSWD: {
			// Ignore irrelevant frames
			if (frame.function != CanFrameGroups::DISPLAY_UPDATE::CONFIRM_PSSWD && frame.answer != true) {
				return;
			}

			doAt3Call([this]() { enterNextState(); });
		} break;

		case TRANSMITTING_HOST_IP: {
			// Ignore irrelevant frames
			if (frame.function != CanFrameGroups::DISPLAY_UPDATE::CONFIRM_HOST_IP && frame.answer != true) {
				return;
			}

			doAt3Call([this]() { enterNextState(); });
		} break;

		case JOIN_WIFI: {
			return;
		} break;

		case EXECUTING_UPDATE: {
			if (frame.function != CanFrameGroups::DISPLAY_UPDATE::EXECUTE_UPDATE || frame.answer == false) {
				return;
			}

			doAt3Call([]() { esp_restart(); });
		} break;

		default: {
		}
	}
}

/*
 *	STARTING_AP
 */
void UpdateDisplays::startWifiAP()
{
	const auto json = config_->getReadAccess();

	wifi_.setSsid(json["WifiHost"]["ssid"]);
	wifi_.setPassword(json["WifiHost"]["password"]);

	wifi_.callOnSuccess([this] { this->enterNextState(); });

	wifi_.start();
}

/*
 *	ENTERING_UPDATE_MODE
 */
void UpdateDisplays::putDisplaysIntoUpdateMode()
{
	// Build header
	Can::Frame_t frame;
	frame.sender = CAN_MASTER_ID;
	frame.target = CAN_BROADCAST_ID;
	frame.group = CanFrameGroups::GROUP::DISPLAY_UPDATE;
	frame.function = CanFrameGroups::DISPLAY_UPDATE::ENTER_UPDATE_MODE;
	frame.dataLengthCode = 0;
	frame.answer = false;

	// Transmit frame
	can_->queueFrame(frame);
}

std::vector<std::vector<char>> UpdateDisplays::getAsPackages(const std::string& src)
{
	std::vector<std::vector<char>> result;
	std::vector<char> currPackage;

	// Split the string up into packages with a size of max 8 bytes/chars
	for (const auto& c : src) {
		if (currPackage.size() >= 8) {
			result.push_back(currPackage);
			currPackage.clear();
		}

		currPackage.push_back(c);
	}

	// Add the last package too
	result.push_back(currPackage);

	return result;
}

/*
 *	TRANSMITTING_SSID
 */
void UpdateDisplays::transmitSsidPackages(const std::vector<std::vector<char>>& ssidPackages)
{
	/*
	 *	Build Task function
	 */
	struct TaskStuff
	{
		Can* can = nullptr;
		std::vector<std::vector<char>> ssidPackages;
	};

	TaskStuff taskStuff = {.can = can_, .ssidPackages = ssidPackages};

	auto transmitSsidLambda = [](void* p_param)
	{
		if (p_param == nullptr) {
			vTaskDelete(nullptr);
		}

		auto taskStuff = static_cast<TaskStuff*>(p_param);

		// Send SSID Packages
		for (const auto& package : taskStuff->ssidPackages) {
			Can::Frame_t frame;
			frame.sender = CAN_MASTER_ID;
			frame.target = CAN_BROADCAST_ID;
			frame.group = CanFrameGroups::GROUP::DISPLAY_UPDATE;
			frame.function = CanFrameGroups::DISPLAY_UPDATE::SET_SSID;
			frame.dataLengthCode = package.size();

			// Fill in the SSID package
			std::copy(package.begin(), package.end(), frame.data);

			// Send the frame
			taskStuff->can->queueFrame(frame);
		}

		// Request Ok from Displays
		Can::Frame_t frame;
		frame.sender = CAN_MASTER_ID;
		frame.target = CAN_BROADCAST_ID;
		frame.group = CanFrameGroups::GROUP::DISPLAY_UPDATE;
		frame.function = CanFrameGroups::DISPLAY_UPDATE::CONFIRM_SSID;
		frame.dataLengthCode = 0;
		taskStuff->can->queueFrame(frame);
	};

	/*
	 *	Start Task
	 */
	if (xTaskCreate(transmitSsidLambda, "TransmitSsidTask", 4096, &taskStuff, 2, nullptr) == pdFALSE) {
		ESP_LOGE(TAG, "Failed to start Task \'TransmitSsidTask\'");
		return;
	}
}

/*
 *	TRANSMITTING_PSSWD
 */
void UpdateDisplays::transmitPsswdPackages(const std::vector<std::vector<char>>& psswdPackages)
{
	/*
	 *	Build Task function
	 */
	struct TaskStuff
	{
		Can* can = nullptr;
		std::vector<std::vector<char>> psswdPackages;
	};

	TaskStuff taskStuff = {.can = can_, .psswdPackages = psswdPackages};

	auto transmitPsswdLambda = [](void* p_param)
	{
		if (p_param == nullptr) {
			vTaskDelete(nullptr);
		}

		auto taskStuff = static_cast<TaskStuff*>(p_param);

		// Send PSSWD Packages
		for (const auto& package : taskStuff->psswdPackages) {
			Can::Frame_t frame;
			frame.sender = CAN_MASTER_ID;
			frame.target = CAN_BROADCAST_ID;
			frame.group = CanFrameGroups::GROUP::DISPLAY_UPDATE;
			frame.function = CanFrameGroups::DISPLAY_UPDATE::SET_PSSWD;
			frame.dataLengthCode = package.size();

			// Fill in the PSSWD package
			std::copy(package.begin(), package.end(), frame.data);

			// Send the frame
			taskStuff->can->queueFrame(frame);
		}

		// Request Ok from Displays
		Can::Frame_t frame;
		frame.sender = CAN_MASTER_ID;
		frame.target = CAN_BROADCAST_ID;
		frame.group = CanFrameGroups::GROUP::DISPLAY_UPDATE;
		frame.function = CanFrameGroups::DISPLAY_UPDATE::CONFIRM_PSSWD;
		frame.dataLengthCode = 0;
		taskStuff->can->queueFrame(frame);
	};

	/*
	 *	Start Task
	 */
	if (xTaskCreate(transmitPsswdLambda, "TransmitPsswdTask", 4096, &taskStuff, 2, nullptr) == pdFALSE) {
		ESP_LOGE(TAG, "Failed to start Task \'TransmitPsswdTask\'");
		return;
	}
}

/*
 *	TRANSMITTING_HOST_IP
 */
void UpdateDisplays::transmitHostIp()
{
	const auto& ip = wifi_.getIp();

	/*
	 *	Transmit Host IP
	 */
	Can::Frame_t ipFrame;
	ipFrame.sender = CAN_MASTER_ID;
	ipFrame.target = CAN_BROADCAST_ID;
	ipFrame.group = CanFrameGroups::GROUP::DISPLAY_UPDATE;
	ipFrame.function = CanFrameGroups::DISPLAY_UPDATE::SET_HOST_IP;
	ipFrame.dataLengthCode = ip.size();

	// Fill in the Ip package
	std::copy(ip.begin(), ip.end(), ipFrame.data);

	// Send the frame
	can_->queueFrame(ipFrame);

	/*
	 *	Request Ok from Displays
	 */
	Can::Frame_t okFrame;
	okFrame.sender = CAN_MASTER_ID;
	okFrame.target = CAN_BROADCAST_ID;
	okFrame.group = CanFrameGroups::GROUP::DISPLAY_UPDATE;
	okFrame.function = CanFrameGroups::DISPLAY_UPDATE::CONFIRM_PSSWD;
	okFrame.dataLengthCode = 0;
	can_->queueFrame(okFrame);
}

/*
 *	JOIN_WIFI
 */
void UpdateDisplays::registerWifiEventHandlers()
{
	auto eventHandler = [](void* p_arg, esp_event_base_t, int32_t, void*)
	{
		doAt3Call(
			[p_arg]()
			{
				if (p_arg == nullptr) {
					return;
				}

				static_cast<UpdateDisplays*>(p_arg)->enterNextState();
			});
	};

	esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, eventHandler, this, &wifiEventHandler_);
}

void UpdateDisplays::orderDisplaysToJoinWifi()
{
	Can::Frame_t frame;
	frame.sender = CAN_MASTER_ID;
	frame.target = CAN_BROADCAST_ID;
	frame.group = CanFrameGroups::GROUP::DISPLAY_UPDATE;
	frame.function = CanFrameGroups::DISPLAY_UPDATE::JOIN_WIFI;
	frame.dataLengthCode = 0;
	can_->queueFrame(frame);
}

void UpdateDisplays::unregisterWifiEventHandlers()
{
	esp_event_handler_instance_unregister(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, wifiEventHandler_);
}

/*
 *	EXECUTE
 */
void UpdateDisplays::orderDisplaysToExecuteUpdate()
{
	Can::Frame_t frame;
	frame.sender = CAN_MASTER_ID;
	frame.target = CAN_BROADCAST_ID;
	frame.group = CanFrameGroups::GROUP::DISPLAY_UPDATE;
	frame.function = CanFrameGroups::DISPLAY_UPDATE::EXECUTE_UPDATE;
	frame.dataLengthCode = 0;
	can_->queueFrame(frame);
}

/*
 *   Private Task Function Implementations
 */
void UpdateDisplays::canRxTask(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	auto state = static_cast<UpdateDisplays*>(p_param);

	Can::Frame_t frame;
	while (true) {
		if (xQueueReceive(state->receivedFramesQueue_, &frame, portMAX_DELAY) != pdTRUE) {
			continue;
		}

		state->handleCanFrame(frame);
	}
}
