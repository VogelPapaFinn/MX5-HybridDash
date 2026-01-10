// Project includes
#include "../can.h"

// C includes
#include <stdio.h>
#include <string.h>

// espidf includes
#include <esp_debug_helpers.h>
#include <esp_log.h>

/*
 *	Private Defines
 */
#define CAN_BUS_SPEED 1000000 // 1 MBit/s
#define CAN_QUEUE_DEPTH 15
#define CAN_FRAME_DATA_LENGTH_B 8
#define CAN_FRAME_TRANSMIT_TIMEOUT_AFTER_MS 20

/*
 *	Private variables
 */
//! \brief The handle of the can node
static twai_node_handle_t g_nodeHandle;

//! \brief Bool indicating if the node is currently enabled
static bool g_nodeEnabled = false;

//! \brief Slots which are used to keep track of frames which should be sent
static TwaiFrame_t g_framesToTransmit[CAN_QUEUE_DEPTH];

//! \brief A list of FreeRTOS Queues that should be notified once a message was
//! received
static QueueHandle_t** g_queuesToNotify = NULL;

//! \brief The amount of registered FreeRTOS queue handles
static uint8_t g_amountOfQueuesToNotifyReceived = 0;

/*
 *	Callback functions
 */
// ReSharper disable once CppDFAConstantFunctionResult
//! \brief Callback function called when a frame was transmitted
//! \param nodeHandle The handle of the CAN/TWAI node
//! \param p_eventData A pointer to a struct which contains the sent CAN frame
//! \param p_userCtx Unused user parameters
//! \retval Bool indicating if a higher task was woken
static IRAM_ATTR bool transmittedFrameCb(twai_node_handle_t nodeHandle,
                                              const twai_tx_done_event_data_t* p_eventData, void* p_userCtx)
{
	if (nodeHandle != g_nodeHandle) {
		return false;
	}
	if (p_eventData == NULL) {
		return false;
	}

	// Debug logging
	// esp_rom_printf("Transmitted frame with message id %d with buffer %d %d %d %d %d %d %d %d \n", p_eventData->done_tx_frame->header.id >> 21, p_eventData->done_tx_frame->buffer[0],p_eventData->done_tx_frame->buffer[1], p_eventData->done_tx_frame->buffer[2], p_eventData->done_tx_frame->buffer[3], p_eventData->done_tx_frame->buffer[4], p_eventData->done_tx_frame->buffer[5], p_eventData->done_tx_frame->buffer[6], p_eventData->done_tx_frame->buffer[7]);

	// Iterate through the frame array
	for (uint8_t i = 0; i < CAN_QUEUE_DEPTH; i++) {
		// Is the frame waiting to be transmitted?
		if (!g_framesToTransmit[i].transmitting) {
			continue;
		}

		// Compare the frame to the one we sent
		bool equal = true;
		equal &= g_framesToTransmit[i].espidfFrame.header.id == p_eventData->done_tx_frame->header.id;
		equal &= g_framesToTransmit[i].espidfFrame.header.dlc == p_eventData->done_tx_frame->header.dlc;
		equal &= g_framesToTransmit[i].espidfFrame.buffer_len == p_eventData->done_tx_frame->buffer_len;
		equal &= g_framesToTransmit[i].espidfFrame.buffer[0] == p_eventData->done_tx_frame->buffer[0];
		equal &= g_framesToTransmit[i].espidfFrame.buffer[1] == p_eventData->done_tx_frame->buffer[1];
		equal &= g_framesToTransmit[i].espidfFrame.buffer[2] == p_eventData->done_tx_frame->buffer[2];
		equal &= g_framesToTransmit[i].espidfFrame.buffer[3] == p_eventData->done_tx_frame->buffer[3];
		equal &= g_framesToTransmit[i].espidfFrame.buffer[4] == p_eventData->done_tx_frame->buffer[4];
		equal &= g_framesToTransmit[i].espidfFrame.buffer[5] == p_eventData->done_tx_frame->buffer[5];
		equal &= g_framesToTransmit[i].espidfFrame.buffer[6] == p_eventData->done_tx_frame->buffer[6];
		equal &= g_framesToTransmit[i].espidfFrame.buffer[7] == p_eventData->done_tx_frame->buffer[7];

		if (equal) {
			// Allow the frame to be reused
			g_framesToTransmit[i].transmitting = false;

			return false;
		}
	}

	return false;
}

//! \brief Callback function called when a frame was received
//! \param nodeHandle The handle of the CAN/TWAI node
//! \param p_eventData A pointer to a struct which contains the received CAN frame
//! \param p_userCtx Unused user parameters
//! \retval Bool indicating if a higher task was woken
static IRAM_ATTR bool receivedFrameCb(twai_node_handle_t nodeHandle, const twai_rx_done_event_data_t* p_eventData,
                                      void* p_userCtx)
{
	// Is it our handle?
	if (nodeHandle != g_nodeHandle) {
		return false;
	}

	// NULL check
	if (p_eventData == NULL) {
		return false;
	}

	// Get the message
	uint8_t buffer[CAN_FRAME_DATA_LENGTH_B] = { 0x00 };
	twai_frame_t rxFrame = {
		.buffer = &buffer[0],
		.buffer_len = CAN_FRAME_DATA_LENGTH_B,
	};
	twai_node_receive_from_isr(nodeHandle, &rxFrame);

	// Debug logging
	// esp_rom_printf("Received frame with message id %d from sender %d with buffer %d %d %d %d %d %d %d %d \n", rxFrame.header.id >> 21, rxFrame.header.id & 0x1FFFFF, rxFrame.buffer[0], rxFrame.buffer[1], rxFrame.buffer[2], rxFrame.buffer[3], rxFrame.buffer[4], rxFrame.buffer[5], rxFrame.buffer[6], rxFrame.buffer[7]);

	// Do we have any queues to notify?
	if (g_amountOfQueuesToNotifyReceived == 0) {
		return false;
	}

	// Build the event
	QueueEvent_t event;
	event.command = RECEIVED_NEW_CAN_MESSAGE;
	event.canFrame = rxFrame;

	// Notify all registered queues
	BaseType_t xHigherPriorityTaskWoken;
	for (uint8_t i = 0; i < g_amountOfQueuesToNotifyReceived; i++) {
		xQueueSendFromISR(*g_queuesToNotify[i], &event, &xHigherPriorityTaskWoken);
	}

	return xHigherPriorityTaskWoken;
}

//! \brief Callback function called when the (error) state of the CAN node changed
//! \param nodeHandle The handle of the CAN/TWAI node
//! \param p_eventData A pointer to a struct which contains debug information
//! \param p_userCtx Unused user parameters
//! \retval Bool indicating if a higher task was woken
bool canStateChangedCb(twai_node_handle_t nodeHandle, const twai_state_change_event_data_t *p_eventData, void *p_userCtx) {
	// Check if the CAN bus crashed otherwise ignore the state change
	if (p_eventData->new_sta != TWAI_ERROR_BUS_OFF) {
		return false;
	}

	esp_rom_printf("State changed from %d to %d\n", p_eventData->old_sta, p_eventData->new_sta);
	// esp_rom_printf("An error occurred. Arbitration lost: %d \n Bit error: %d \n Form error: %d \n Stuff error: %d \n ACK error: %d \n val: %d", edata->err_flags.arb_lost, edata->err_flags.bit_err, edata->err_flags.form_err, edata->err_flags.stuff_err, edata->err_flags.ack_err, edata->err_flags.val);

	// Create the queue event
	QueueEvent_t event;
	event.command = CAN_DRIVER_CRASHED;

	// Send it to all registered cb queues
	BaseType_t xHigherPriorityTaskWoken;
	for (uint8_t i = 0; i < g_amountOfQueuesToNotifyReceived; i++) {
		xQueueSendFromISR(*g_queuesToNotify[i], &event, &xHigherPriorityTaskWoken);
	}

	return xHigherPriorityTaskWoken;
}

/*
 *	Private functions
 */
//! \brief Function used to copy a CAN frame, to keep pointers to it alive until it was sent
//! \param p_frame A pointer to the frame that should be copied and tracked
//! \retval A pointer to the tracked frame
static TwaiFrame_t* keepTrackOfFrame(const TwaiFrame_t* p_frame)
{
	// Iterate through the frame array
	for (uint8_t i = 0; i < CAN_QUEUE_DEPTH; i++) {
		// Is the frame waiting to be transmitted?
		if (g_framesToTransmit[i].transmitting) {
			continue;
		}

		// Use this frame
		g_framesToTransmit[i].transmitting = true;

		// Copy the message here
		g_framesToTransmit[i] = *p_frame;

		// Return the pointer to this address
		return &g_framesToTransmit[i];
	}

	ESP_LOGE("CanDriver", "Cant keep track of frame. All frame slots are occupied!");

	return NULL;
}

/*
 *	Public functions
 */
twai_node_handle_t* canInitializeNode(const uint8_t txGpio, const uint8_t rxGpio)
{
	// Create the node config
	const twai_onchip_node_config_t nodeConfig = {
		.io_cfg.tx = txGpio,
		.io_cfg.rx = rxGpio,
		.bit_timing.bitrate = CAN_BUS_SPEED,
		.tx_queue_depth = CAN_QUEUE_DEPTH,
	};

	// Initialize the node
	if (twai_new_node_onchip(&nodeConfig, &g_nodeHandle) != ESP_OK) {
		return NULL;
	}

	// Register rx and tx callbacks
	const twai_event_callbacks_t cbs = {
		.on_tx_done = transmittedFrameCb,
		.on_rx_done = receivedFrameCb,
		.on_state_change = canStateChangedCb,
		.on_error = NULL,
	};
	if (twai_node_register_event_callbacks(g_nodeHandle, &cbs, NULL) != ESP_OK) {
		return NULL;
	}

	return &g_nodeHandle;
}

bool canEnableNode()
{
	if (g_nodeHandle == NULL)
		return false;

	g_nodeEnabled = twai_node_enable(g_nodeHandle) == ESP_OK;

	return g_nodeEnabled;
}

esp_err_t canRecoverDriver()
{
	return twai_node_recover(g_nodeHandle);
}

bool canRegisterRxCbQueue(QueueHandle_t* queueHandle)
{
	// Make the array larger
	const void* newAddr = realloc(g_queuesToNotify, sizeof(QueueHandle_t*) * (g_amountOfQueuesToNotifyReceived + 1));

	// Did it work?
	if (newAddr != NULL) {
		g_queuesToNotify = (QueueHandle_t**)newAddr;
	}
	else {
		return false;
	}

	// Increase the counter
	g_amountOfQueuesToNotifyReceived++;

	// Then save the task handle
	g_queuesToNotify[g_amountOfQueuesToNotifyReceived - 1] = queueHandle;

	return true;
}

void canInitiateFrame(TwaiFrame_t* p_frame, const uint8_t frameId, const uint8_t bufferLen)
{
	if (p_frame == NULL) {
		return;
	}

	// Activate the 29-bit ID format
	p_frame->espidfFrame.header.ide = true;

	// Build the frame ID
	p_frame->espidfFrame.header.id = 0;
	p_frame->espidfFrame.header.id += frameId << CAN_MESSAGE_ID_OFFSET;
	p_frame->espidfFrame.header.id += g_ownCanComId & 0x1FFFFF; // Zero the top 8-bit

	// Set the buffer ptr
	p_frame->espidfFrame.buffer = &p_frame->buffer[0];

	// Set all unused buffer bytes to 0
	for (uint8_t i = bufferLen; i < CAN_FRAME_MAX_BUFFER_LENGTH_B; i++) {
		p_frame->espidfFrame.buffer[i] = 0;
	}

	// Set the buffer length
	p_frame->espidfFrame.header.dlc = bufferLen;
	p_frame->espidfFrame.buffer_len = bufferLen;

	// Not yet transmitted
	p_frame->transmitting = false;
}

bool canQueueFrame(const TwaiFrame_t* p_frame)
{
	if (g_nodeHandle == NULL) {
		return false;
	}

	// Keep track of the frame
	const TwaiFrame_t* trackedFrameInstance = keepTrackOfFrame(p_frame);
	if (trackedFrameInstance == NULL) {
		return false;
	}

	// Send the frame via can bus
	return twai_node_transmit(g_nodeHandle, &trackedFrameInstance->espidfFrame, CAN_FRAME_TRANSMIT_TIMEOUT_AFTER_MS) == ESP_OK;
}
