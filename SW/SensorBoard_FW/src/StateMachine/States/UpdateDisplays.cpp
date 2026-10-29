#include "StateMachine/States/UpdateDisplays.hpp"

// Project includes
#include "StateMachine/StateEvents.hpp"

// espidf includes
#include "esp_event.h"

// Logging
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"

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
	if (++s_counter >= 1) {
		s_counter = 0;
		cb();
		return;
	}
}

/*
 *   Public Function Implementations
 */
UpdateDisplays::UpdateDisplays(Filesystem* p_fs, Config* p_config, Can* p_can, std::vector<Display>* p_displays) :
	CanReceiver(p_can)
{
	fs_ = p_fs;
	config_ = p_config;
	displays_ = p_displays;

	esp_log_level_set(TAG, ESP_LOG_DEBUG);
}

void UpdateDisplays::enter()
{
	ESP_LOGI(TAG, "Entered UpdateDisplays State");

	timeoutTimer_ =
		new TimeoutTimer(60000,
						 []()
						 {
							 ESP_LOGE(TAG, "Timeout: Failed to update displays. Aborting.");
							 esp_event_post(STATE_EVENT_BASE, STATE_EVENT_ID::UPDATE_FAILED, nullptr, 0, portMAX_DELAY);
						 });

	startCanRxTask();
	enterNextState();
}

void UpdateDisplays::quit()
{
	enteredUpdateMode();

	delete (timeoutTimer_);
	timeoutTimer_ = nullptr;

	substate_ = UNDEFINED;

	unregisterWifiEventHandlers();
	stopCanRxTask();

	restartDisplays();
}

/*
 *   Private Function Implementations
 */
void UpdateDisplays::enterNextState()
{
	substate_++;
	timeoutTimer_->start();

	switch (substate_) {
		case STARTING_AP: {
			ESP_LOGD(TAG, "Entering STARTING_AP");

			startWifiAP();
		} break;

		case STARTING_WEBSERVER: {
			ESP_LOGD(TAG, "Entering STARTING_WEBSERVER");

			if (startWebserver()) {
				enterNextState();
				return;
			}
		} break;

		case ENTERING_UPDATE_MODE: {
			ESP_LOGD(TAG, "Entering ENTERING_UPDATE_MODE");

			enterUpdateMode();
		} break;

		case TRANSMITTING_SSID: {
			ESP_LOGD(TAG, "Entering TRANSMITTING_SSID");

			ssidPackages_ = getAsPackages(wifi_.getSsid());
			transmitSsidPackages();
		} break;

		case TRANSMITTING_PSSWD: {
			ESP_LOGD(TAG, "Entering TRANSMITTING_PSSWD");

			psswdPackages_ = getAsPackages(wifi_.getPassword());
			transmitPsswdPackages();
		} break;

		case TRANSMITTING_HOST_IP: {
			ESP_LOGD(TAG, "Entering TRANSMITTING_HOST_IP");

			transmitHostIp();
		} break;

		case JOIN_WIFI: {
			ESP_LOGD(TAG, "Entering JOIN_WIFI");

			orderDisplaysToJoinWifi();
		} break;

		case EXECUTING_UPDATE: {
			ESP_LOGD(TAG, "Entering EXECUTING_UPDATE");

			unregisterWifiEventHandlers();
			orderDisplaysToExecuteUpdate();
		} break;

		default: {
		}
	}
}

void UpdateDisplays::handleCanFrame(const Can::Frame_t& frame)
{
	ESP_LOGI(TAG, "Received CAN frame: %s", frame.toString().c_str());

	// Ignore irrelevant frames
	if (frame.group != CanFrameGroups::GROUP::UPDATE_DISPLAY || frame.answer == 0) {
		return;
	}

	switch (substate_) {
		case ENTERING_UPDATE_MODE: {
			// Ignore irrelevant frames
			if (frame.function != CanFrameGroups::FUNC::ENTER_UPDATE_MODE) {
				return;
			}

			doAt3Call(
				[this]()
				{
					enteredUpdateMode();
					enterNextState();
				});
		} break;

		case TRANSMITTING_SSID: {
			// Ignore irrelevant frames
			if (frame.function != CanFrameGroups::FUNC::CONFIRM_SSID) {
				return;
			}

			doAt3Call([this]() { enterNextState(); });
		} break;

		case TRANSMITTING_PSSWD: {
			// Ignore irrelevant frames
			if (frame.function != CanFrameGroups::FUNC::CONFIRM_PSSWD) {
				return;
			}

			doAt3Call([this]() { enterNextState(); });
		} break;

		case TRANSMITTING_HOST_IP: {
			// Ignore irrelevant frames
			if (frame.function != CanFrameGroups::FUNC::CONFIRM_HOST_IP) {
				return;
			}

			doAt3Call([this]() { enterNextState(); });
		} break;

		case JOIN_WIFI: {
			// Ignore irrelevant frames
			if (frame.function != CanFrameGroups::FUNC::JOIN_WIFI) {
				return;
			}

			doAt3Call([this]() { enterNextState(); });
		} break;

		case EXECUTING_UPDATE: {
			if (frame.function != CanFrameGroups::FUNC::EXECUTE_UPDATE) {
				return;
			}

			doAt3Call(
				[this]()
				{
					fs_->deleteFile("display_update.bin", Filesystem::DATA_PARTITION);
					esp_restart();
				});
		} break;

		default: {
		}
	}
}

void UpdateDisplays::restartDisplays()
{
	for (auto& d : *displays_) {
		d.turnOff();
	}

	// Restart them after a short delay
	xTimerCreate("RestartDisplays", pdMS_TO_TICKS(100), false, displays_,
				 [](TimerHandle_t p_timer)
				 {
					 auto displays = static_cast<std::vector<Display>*>(pvTimerGetTimerID(p_timer));

					 for (auto& d : *displays) {
						 d.turnOn();
					 }

					 xTimerDelete(p_timer, 0);
				 });
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
 *	STARTING_WEBSERVER
 */
bool UpdateDisplays::startWebserver()
{
	// Start server
	if (httpd_start(&webserver_, &webserverConfig_) != ESP_OK) {
		ESP_LOGW(TAG, "Failed to start Webserver");
		return false;
	}

	// The handler that transmits the display_update.bin file
	auto updateHandler = [](httpd_req_t* p_req) -> esp_err_t
	{
		if (p_req == nullptr || p_req->user_ctx == nullptr) {
			return ESP_FAIL;
		}

		auto fs = static_cast<Filesystem*>(p_req->user_ctx);

		// Open the display_update.bin file
		FILE* updateFile = fs->openFile("display_update.bin", "r", Filesystem::DATA_PARTITION);
		if (updateFile == nullptr) {
			httpd_resp_send_err(p_req, HTTPD_404_NOT_FOUND, "display_update.bin not found");
			return ESP_FAIL;
		}

		// Set answer type (binary)
		httpd_resp_set_type(p_req, "application/octet-stream");

		// Send in chunks
		static char s_fileChunk[1024]; // static: der httpd-Task hat nur ~4 KB Stack
		size_t chunkSize = 0;
		do {
			chunkSize = fread(s_fileChunk, 1, sizeof(s_fileChunk), updateFile);

			if (chunkSize <= 0) {
				continue;
			}

			// Send
			if (httpd_resp_send_chunk(p_req, s_fileChunk, chunkSize) != ESP_OK) {
				// ERRPR
				fclose(updateFile);
				return ESP_FAIL;
			}
		}
		while (chunkSize != 0);

		// Finished
		fclose(updateFile);
		httpd_resp_send_chunk(p_req, nullptr, 0); // Empty packet = Client knows transmission is completed
		return ESP_OK;
	};

	// Register URI
	updateUri_ = {.uri = "/display_update.bin",
				  .method = HTTP_GET,
				  .handler = updateHandler,
				  .user_ctx = fs_,
				  .is_websocket = false};

	return httpd_register_uri_handler(webserver_, &updateUri_) == ESP_OK;
}

/*
 *	ENTERING_UPDATE_MODE
 */
void UpdateDisplays::enterUpdateMode()
{
	/*
	 *	Build Task function
	 */
	auto enterUpdateMode = [](void* p_param)
	{
		if (p_param == nullptr) {
			vTaskDelete(nullptr);
		}

		auto can = static_cast<Can*>(p_param);

		// Build request frame
		Can::Frame_t frame;
		frame.sender = CAN_MASTER_ID;
		frame.target = CAN_BROADCAST_ID;
		frame.group = CanFrameGroups::GROUP::UPDATE_DISPLAY;
		frame.function = CanFrameGroups::FUNC::ENTER_UPDATE_MODE;
		frame.dataLengthCode = 0;
		frame.answer = false;

		while (true) {
			// Stop if ordered to
			if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000)) != 0) {
				vTaskDelete(nullptr);
			}

			can->queueFrame(frame);
		}
	};

	/*
	 *	Start Task
	 */
	if (xTaskCreate(enterUpdateMode, "EnterUpdateMode", 4096, can_, 2, &enterUpdateModeTask_) == pdFALSE) {
		ESP_LOGE(TAG, "Failed to start Task \'EnterUpdateMode\'");
		return;
	}
}

void UpdateDisplays::enteredUpdateMode()
{
	if (enterUpdateModeTask_ == nullptr) {
		return;
	}

	xTaskNotifyGive(enterUpdateModeTask_);
	enterUpdateModeTask_ = nullptr;
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
void UpdateDisplays::transmitSsidPackages()
{
	/*
	 *	Build Task function
	 */
	auto transmitSsidLambda = [](void* p_param)
	{
		if (p_param == nullptr) {
			vTaskDelete(nullptr);
		}

		auto state = static_cast<UpdateDisplays*>(p_param);

		// Send SSID Packages
		for (const auto& package : state->ssidPackages_) {
			Can::Frame_t frame;
			frame.sender = CAN_MASTER_ID;
			frame.target = CAN_BROADCAST_ID;
			frame.group = CanFrameGroups::GROUP::UPDATE_DISPLAY;
			frame.function = CanFrameGroups::FUNC::SET_SSID;
			frame.dataLengthCode = package.size();

			// Fill in the SSID package
			std::copy(package.begin(), package.end(), frame.data);

			// Send the frame
			state->can_->queueFrame(frame);
		}

		// Request Ok from Displays
		Can::Frame_t frame;
		frame.sender = CAN_MASTER_ID;
		frame.target = CAN_BROADCAST_ID;
		frame.group = CanFrameGroups::GROUP::UPDATE_DISPLAY;
		frame.function = CanFrameGroups::FUNC::CONFIRM_SSID;
		frame.dataLengthCode = 0;
		state->can_->queueFrame(frame);

		vTaskDelete(nullptr);
	};

	/*
	 *	Start Task
	 */
	if (xTaskCreate(transmitSsidLambda, "TransmitSsidTask", 4096, this, 2, nullptr) == pdFALSE) {
		ESP_LOGE(TAG, "Failed to start Task \'TransmitSsidTask\'");
		return;
	}
}

/*
 *	TRANSMITTING_PSSWD
 */
void UpdateDisplays::transmitPsswdPackages()
{
	/*
	 *	Build Task function
	 */
	auto transmitPsswdLambda = [](void* p_param)
	{
		if (p_param == nullptr) {
			vTaskDelete(nullptr);
		}

		auto state = static_cast<UpdateDisplays*>(p_param);

		// Send PSSWD Packages
		for (const auto& package : state->psswdPackages_) {
			Can::Frame_t frame;
			frame.sender = CAN_MASTER_ID;
			frame.target = CAN_BROADCAST_ID;
			frame.group = CanFrameGroups::GROUP::UPDATE_DISPLAY;
			frame.function = CanFrameGroups::FUNC::SET_PSSWD;
			frame.dataLengthCode = package.size();

			// Fill in the PSSWD package
			std::copy(package.begin(), package.end(), frame.data);

			// Send the frame
			state->can_->queueFrame(frame);
		}

		// Request Ok from Displays
		Can::Frame_t frame;
		frame.sender = CAN_MASTER_ID;
		frame.target = CAN_BROADCAST_ID;
		frame.group = CanFrameGroups::GROUP::UPDATE_DISPLAY;
		frame.function = CanFrameGroups::FUNC::CONFIRM_PSSWD;
		frame.dataLengthCode = 0;
		state->can_->queueFrame(frame);

		vTaskDelete(nullptr);
	};

	/*
	 *	Start Task
	 */
	if (xTaskCreate(transmitPsswdLambda, "TransmitPsswdTask", 4096, this, 2, nullptr) == pdFALSE) {
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
	ipFrame.group = CanFrameGroups::GROUP::UPDATE_DISPLAY;
	ipFrame.function = CanFrameGroups::FUNC::SET_HOST_IP;
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
	okFrame.group = CanFrameGroups::GROUP::UPDATE_DISPLAY;
	okFrame.function = CanFrameGroups::FUNC::CONFIRM_HOST_IP;
	okFrame.dataLengthCode = 0;
	can_->queueFrame(okFrame);
}

/*
 *	JOIN_WIFI
 */
void UpdateDisplays::orderDisplaysToJoinWifi()
{
	Can::Frame_t frame;
	frame.sender = CAN_MASTER_ID;
	frame.target = CAN_BROADCAST_ID;
	frame.group = CanFrameGroups::GROUP::UPDATE_DISPLAY;
	frame.function = CanFrameGroups::FUNC::JOIN_WIFI;
	frame.dataLengthCode = 0;
	can_->queueFrame(frame);
}

void UpdateDisplays::unregisterWifiEventHandlers()
{
	if (wifiEventHandler_ == nullptr) {
		return;
	}

	esp_event_handler_instance_unregister(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, wifiEventHandler_);
	wifiEventHandler_ = nullptr;
}

/*
 *	EXECUTE
 */
void UpdateDisplays::orderDisplaysToExecuteUpdate()
{
	Can::Frame_t frame;
	frame.sender = CAN_MASTER_ID;
	frame.target = CAN_BROADCAST_ID;
	frame.group = CanFrameGroups::GROUP::UPDATE_DISPLAY;
	frame.function = CanFrameGroups::FUNC::EXECUTE_UPDATE;
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
