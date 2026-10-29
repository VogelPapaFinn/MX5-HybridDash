#include "StateMachine/States/Update.hpp"

// Project includes
#include "StateMachine/StateEvents.hpp"

// espidf includes
#include "esp_http_client.h"
#include "esp_https_ota.h"

/*
 *   constexpr
 */
constexpr auto TAG = "UpdateState";

/*
 *   Public Function Implementations
 */
Update::Update(Can* p_can) : CanReceiver(p_can) {}

void Update::enter()
{
	ESP_LOGI(TAG, "Entered Update State");

	startCanRxTask();
	broadcastEnteredUpdateMode();
}

void Update::quit()
{
	wifi_.stop();
	stopCanRxTask();
}

/*
 *   Private Function Implementations
 */
void Update::handleCanFrame(const Can::Frame_t& frame)
{
	ESP_LOGI(TAG, "Received CAN frame: %s", frame.toString().c_str());

	switch (substate_) {
		case RECEIVING_SSID: {
			if (frame.function == CanFrameGroups::SET_SSID) {
				for (uint8_t i = 0; i < frame.dataLengthCode; i++) {
					ssid_ += frame.data[i];
				}

				return;
			}

			if (frame.function == CanFrameGroups::FUNC::CONFIRM_SSID) {
				Can::Frame_t frame;
				frame.target = CAN_MASTER_ID;
				frame.group = CanFrameGroups::GROUP::UPDATE_DISPLAY;
				frame.function = CanFrameGroups::FUNC::CONFIRM_SSID;
				frame.answer = true;
				can_->queueFrame(frame);

				substate_ = RECEIVING_PSSWD;
			}
		} break;

		case RECEIVING_PSSWD: {
			if (frame.function == CanFrameGroups::SET_PSSWD) {
				for (uint8_t i = 0; i < frame.dataLengthCode; i++) {
					psswd_ += frame.data[i];
				}

				return;
			}

			if (frame.function == CanFrameGroups::FUNC::CONFIRM_PSSWD) {
				Can::Frame_t frame;
				frame.target = CAN_MASTER_ID;
				frame.group = CanFrameGroups::GROUP::UPDATE_DISPLAY;
				frame.function = CanFrameGroups::FUNC::CONFIRM_PSSWD;
				frame.answer = true;
				can_->queueFrame(frame);

				substate_ = RECEIVING_HOST_IP;
			}
		} break;

		case RECEIVING_HOST_IP: {
			if (frame.function == CanFrameGroups::SET_HOST_IP) {
				masterIp_[0] = frame.data[0];
				masterIp_[1] = frame.data[1];
				masterIp_[2] = frame.data[2];
				masterIp_[3] = frame.data[3];

				return;
			}

			if (frame.function == CanFrameGroups::FUNC::CONFIRM_HOST_IP) {
				Can::Frame_t frame;
				frame.target = CAN_MASTER_ID;
				frame.group = CanFrameGroups::GROUP::UPDATE_DISPLAY;
				frame.function = CanFrameGroups::FUNC::CONFIRM_HOST_IP;
				frame.answer = true;
				can_->queueFrame(frame);

				substate_ = JOIN_WIFI;
			}
		} break;

		case JOIN_WIFI: {
			if (frame.function == CanFrameGroups::JOIN_WIFI) {
				substate_ = EXECUTING_UPDATE;

				joinWifi();

				return;
			}
		} break;

		case EXECUTING_UPDATE: {
			if (frame.function == CanFrameGroups::EXECUTE_UPDATE) {
				executeUpdate();

				return;
			}
		} break;

		default: {
		};
	}
}

void Update::broadcastEnteredUpdateMode()
{
	Can::Frame_t frame;
	frame.target = CAN_MASTER_ID;
	frame.group = CanFrameGroups::GROUP::UPDATE_DISPLAY;
	frame.function = CanFrameGroups::FUNC::ENTER_UPDATE_MODE;
	frame.answer = true;
	can_->queueFrame(frame);

	substate_ = RECEIVING_SSID;
}

void Update::joinWifi()
{
	wifi_.setSsid(ssid_);
	wifi_.setPassword(psswd_);
	wifi_.setMasterIp(masterIp_);

	const auto cb = [this]()
	{
		Can::Frame_t frame;
		frame.target = CAN_MASTER_ID;
		frame.group = CanFrameGroups::GROUP::UPDATE_DISPLAY;
		frame.function = CanFrameGroups::FUNC::JOIN_WIFI;
		frame.answer = true;
		can_->queueFrame(frame);
	};

	wifi_.callOnSuccess(cb);

	wifi_.start();
}

void Update::executeUpdate()
{
	ESP_LOGI(TAG, "Starting OTA update");

	// Build webaddress
	std::string downloadPath =
		std::format("http://{}.{}.{}.{}/display_update.bin", masterIp_[0], masterIp_[1], masterIp_[2], masterIp_[3]);

	// Build configurations
	const esp_http_client_config_t config = {
		.url = downloadPath.c_str(),
		.timeout_ms = 5000,
		.keep_alive_enable = true,
	};
	const esp_https_ota_config_t otaConfig = {
		.http_config = &config,
	};

	// Execute Update via HTTPS OTA
	esp_err_t updateSuccess = esp_https_ota(&otaConfig);

	// Fail
	if (updateSuccess != ESP_OK) {
		ESP_LOGE(TAG, "OTA Update error: %s", esp_err_to_name(updateSuccess));

		esp_event_post(STATE_EVENT_BASE, UPDATE_FAILED, nullptr, 0, portMAX_DELAY);

		return;
	}
	ESP_LOGI(TAG, "OTA Update sucessful!");

	reportUpdateSuccess();
}

void Update::reportUpdateSuccess()
{
	Can::Frame_t frame;
	frame.target = CAN_MASTER_ID;
	frame.group = CanFrameGroups::GROUP::UPDATE_DISPLAY;
	frame.function = CanFrameGroups::FUNC::EXECUTE_UPDATE;
	frame.answer = true;

	can_->queueFrame(frame);
}
