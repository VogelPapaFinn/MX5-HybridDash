#pragma once

// C and C++ includes
#include <vector>
#include <cstring>
#include <memory>

// Component includes
#include "CanGroupsAndFunctions.hpp"

// espidf includes
#include "esp_attr.h"
#include "esp_twai_onchip.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/*
 *	Public constexpr
 */
constexpr uint8_t CAN_MASTER_ID = 1;
constexpr uint8_t CAN_BROADCAST_ID = 0;


/*
 *	CAN Class
 */
class Can
{
public:
	/*
	 *	Public Struct
	 */
	typedef struct
	{
		/*
		 *	ID
		 */
		uint8_t sender : 3 = 0;

		uint8_t target : 3 = 0;

		uint8_t group : 4 = 0;

		uint8_t function : 5 = 0;

		uint8_t answer : 1 = false;

		/*
		 *	Header
		 */
		uint8_t dataLengthCode = 0;

		bool extendedFrameFormat = true;

		bool remoteFrame = false;

		bool fdFormat = false;

		bool bitRateShift = false;

		bool errorIndicator = false;

		uint64_t timestamp{};

		uint32_t triggerTime{};

		/*
		 *	Other Stuff
		 */
		uint8_t data[8] = {0x00};

		bool transmitting = false;

		twai_frame_t twaiFrame = {
			.buffer = data,
			.buffer_len = 8
		};

		/*
		 *	Functions
		 */
		uint32_t buildId() const
		{
			uint32_t h = 0;
			h += answer;
			h += function << 1;
			h += group << 6;
			h += target << 10;
			h += sender << 13;
			return h;
		}

		void buildTwaiFrame()
		{
			// Header
			twaiFrame.header.id = buildId();
			twaiFrame.header.dlc = dataLengthCode;
			twaiFrame.header.ide = extendedFrameFormat;
			twaiFrame.header.rtr = remoteFrame;
			twaiFrame.header.fdf = fdFormat;
			twaiFrame.header.brs = bitRateShift;
			twaiFrame.header.esi = errorIndicator;
			twaiFrame.header.timestamp = timestamp;
			twaiFrame.header.trigger_time = triggerTime;
			twaiFrame.buffer = data;
			twaiFrame.buffer_len = dataLengthCode;
		}

		void fromTwaiFrame()
		{
			const auto& id = twaiFrame.header.id;

			answer = id & 0b0001;
			function = (id >> 1) & 0b11111;
			group = (id >> 6) & 0b1111;
			target = (id >> 10) & 0b0111;
			sender = (id >> 13) & 0b0111;

			dataLengthCode = twaiFrame.header.dlc;
			extendedFrameFormat = twaiFrame.header.ide;
			remoteFrame = twaiFrame.header.rtr;
			fdFormat = twaiFrame.header.fdf;
			bitRateShift = twaiFrame.header.brs;
			errorIndicator = twaiFrame.header.esi;
			timestamp = twaiFrame.header.timestamp;
			triggerTime = twaiFrame.header.trigger_time;
			for (uint16_t i = 0; i < twaiFrame.header.dlc; i++) {
				data[i] = twaiFrame.buffer[i];
			}
		}

		std::string toString() const
		{
			std::string output = "";

			output += "ID: ";
			output += std::to_string(sender);
			output += std::to_string(target);
			output += std::to_string(group);
			output += std::to_string(function);
			output += std::to_string(answer);

			output += " DLC: ";
			output += std::to_string(dataLengthCode);
			output += " BUFFER: ";
			for (uint16_t i = 0; i < dataLengthCode; i++) {
				output += std::to_string(data[i]);
				output += ", ";
			}

			return output;
		}
	} Frame;

	/*
	 *	Public Callback functions
	 */
	IRAM_ATTR bool receivedFrameCb(twai_node_handle_t nodeHandle, const twai_rx_done_event_data_t* p_eventData,
	                               void* p_userCtx);

	void transmittedFrameCb(const twai_frame_t frame);

	IRAM_ATTR bool stateChangedCb(twai_node_handle_t nodeHandle, const twai_state_change_event_data_t* p_eventData,
	                              void* p_userCtx);

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

	QueueHandle_t* getFrameTransmittedQueue();

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

	std::vector<std::shared_ptr<Frame>> pendingFrames_;
	SemaphoreHandle_t pendingFramesMutex_ = nullptr;

	TaskHandle_t frameTransmittedTaskHandle_ = nullptr;
	QueueHandle_t frameTransmittedQueueHandle_ = nullptr;

};
