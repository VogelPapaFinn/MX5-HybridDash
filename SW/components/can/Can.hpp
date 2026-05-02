#pragma once

// C and C++ includes
#include <vector>

// espidf includes
#include <cstring>

#include "esp_attr.h"
#include "esp_twai_onchip.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

class Can
{
public:
	/*
	 *	Public Struct
	 */
	typedef struct
	{
		uint8_t id : 2 = 0;

		uint8_t group : 4 = 0;

		uint8_t function : 4 = 0;

		bool answer = false;

		uint16_t dataLengthCode = 0;

		bool extendedFrameFormat = true;

		bool remoteFrame = false;

		bool fdFormat = false;

		bool bitRateShift = false;

		bool errorIndicator = false;

		uint64_t timestamp;

		uint32_t triggerTime;

		std::vector<uint8_t> data;

		bool transmitting = false;

		twai_frame_t twaiFrame;

		void generateTwaiFrame()
		{
			// Header
			twaiFrame.header.id = (id << 9) + (group << 5) + (function << 1) + static_cast<uint8_t>(answer);
			twaiFrame.header.dlc = dataLengthCode;
			twaiFrame.header.ide = extendedFrameFormat;
			twaiFrame.header.rtr = remoteFrame;
			twaiFrame.header.fdf = fdFormat;
			twaiFrame.header.brs = bitRateShift;
			twaiFrame.header.esi = errorIndicator;
			twaiFrame.header.timestamp = timestamp;
			twaiFrame.header.trigger_time = triggerTime;

			// Body
			if (dataLengthCode > 0) {
				twaiFrame.buffer = static_cast<uint8_t*>(malloc(twaiFrame.header.dlc));
				memset(twaiFrame.buffer, 0, 8);

				twaiFrame.buffer_len = dataLengthCode;

				for (uint8_t i = 0; i < data.size() && i < 8; i++) {
					twaiFrame.buffer[i] = data.at(i);
				}
				twaiFrame.buffer_len = data.size();
			} else {
				twaiFrame.buffer = nullptr;
				twaiFrame.buffer_len = 0;
			}
		}

		void fromTwaiFrame(const twai_frame_t& twaiFrame)
		{
			id = (twaiFrame.header.id >> 9) & 0b0011;
			group = (twaiFrame.header.id >> 5) & 0b1111;
			function = (twaiFrame.header.id >> 1) & 0b1111;
			answer = twaiFrame.header.id & 0b0001;
			dataLengthCode = twaiFrame.header.dlc;
			extendedFrameFormat = twaiFrame.header.ide;
			remoteFrame = twaiFrame.header.rtr;
			fdFormat = twaiFrame.header.fdf;
			bitRateShift = twaiFrame.header.brs;
			errorIndicator = twaiFrame.header.esi;
			timestamp = twaiFrame.header.timestamp;
			triggerTime = twaiFrame.header.trigger_time;
			for (uint16_t i = 0; i < twaiFrame.header.dlc; i++) {
				data.push_back(twaiFrame.buffer[i]);
			}
		}
	} Frame;

	/*
	 *	Public Callback functions
	 */
	IRAM_ATTR bool receivedFrameCb(twai_node_handle_t nodeHandle, const twai_rx_done_event_data_t* p_eventData, void* p_userCtx);

	IRAM_ATTR bool transmittedFrameCb(twai_node_handle_t nodeHandle, const twai_tx_done_event_data_t* p_eventData, void* p_userCtx);

	IRAM_ATTR bool stateChangedCb(twai_node_handle_t nodeHandle, const twai_state_change_event_data_t* p_eventData, void* p_userCtx);

	/*
	 *	Public Functions
	 */
	Can(gpio_num_t rxGpio, gpio_num_t txGpio);

	bool initialize();

	void deinitialize() const;

	bool enable();

	void registerRxCbQueue(QueueHandle_t* queueHandle);

	void deregisterRxCbQueue(const QueueHandle_t* queueHandle);

	void queueFrame(Frame& canFrame);
private:
	/*
	 *	Private Member variables
	 */
	gpio_num_t rxGpio_ = GPIO_NUM_NC;
	gpio_num_t txGpio_ = GPIO_NUM_NC;

	twai_node_handle_t nodeHandle_;
	twai_onchip_node_config_t nodeConfig_ = {};
	twai_event_callbacks_t nodeCallbacks_;

	bool enabled_ = false;

	std::vector<QueueHandle_t*> rxCbQueueHandles_;

	std::vector<Frame> pendingFrames_;
};
