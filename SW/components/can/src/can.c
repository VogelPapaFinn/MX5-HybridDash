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
#define CAN_QUEUE_DEPTH 30
#define CAN_FRAME_DATA_LENGTH_B 8
#define CAN_FRAME_TRANSMIT_TIMEOUT_AFTER_MS 20

/*
 * Private structs
 */
//! \brief A struct which is used to keep track of message memory
typedef struct
{
	//! \brief A pointer to the CAN frame
	twai_frame_t* frame;

	//! \brief Should the message data memory be freed afterward?
	bool freeDataAfterwards;
} ActiveMessage_t;

/*
 *	Private variables
 */
//! \brief The handle of the can node
static twai_node_handle_t* g_nodeHandle = NULL;

//! \brief Indicates if the node is currently enabled or not
static bool g_nodeEnabled = false;

static TwaiFrame_t g_framesToTransmit[CAN_QUEUE_DEPTH];

//! \brief A list of FreeRTOS Queues that should be notified once a message was
//! received
static QueueHandle_t** g_queuesToNotify = NULL;
//! \brief The amount of registered FreeRTOS queue handles
static uint8_t g_amountOfQueuesToNotifyReceived = 0;

/*
 *	Private functions
 */
static TwaiFrame_t* keepTrackOfFrame(const TwaiFrame_t* frame)
{
	// Iterate through the frame array
	for (uint8_t i = 0; i < CAN_QUEUE_DEPTH; i++) {
		// Is the frame in use?
		if (g_framesToTransmit[i].transmitting) {
			continue;
		}

		// We now use this slot
		g_framesToTransmit[i].transmitting = true;

		// Copy the message here
		g_framesToTransmit[i] = *frame;

		// Return the pointer to this address
		return &g_framesToTransmit[i];
	}

	ESP_LOGE("CanDriver", "Cant keep track of frame. MAX Can queue depth reached!");

	return NULL;
}

//! \brief Callback function used to free can bus messages after they were sent
// ReSharper disable once CppDFAConstantFunctionResult
static IRAM_ATTR bool transmitOfMessageDoneCb(twai_node_handle_t nodeHandle,
                                              const twai_tx_done_event_data_t* p_eventData, void* p_userCtx)
{
	if (nodeHandle != *g_nodeHandle) {
		return false;
	}

	if (p_eventData == NULL) {
		return false;
	}

	// esp_rom_printf("Transmitting frame with message id %d with buffer %d %d %d %d %d %d %d %d \n", p_eventData->done_tx_frame->header.id >> 21, p_eventData->done_tx_frame->buffer[0],p_eventData->done_tx_frame->buffer[1], p_eventData->done_tx_frame->buffer[2], p_eventData->done_tx_frame->buffer[3], p_eventData->done_tx_frame->buffer[4], p_eventData->done_tx_frame->buffer[5], p_eventData->done_tx_frame->buffer[6], p_eventData->done_tx_frame->buffer[7]);


	// Iterate through the frame array
	for (uint8_t i = 0; i < CAN_QUEUE_DEPTH; i++) {
		// Is the frame in use?
		if (!g_framesToTransmit[i].transmitting	) {
			continue;
		}

		// Is it the frame which was sent?
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
			// Yes, so mark it as unused
			g_framesToTransmit[i].transmitting = false;

			return false;
		}
	}

	return false;
}

static IRAM_ATTR bool receivedMessageCb(twai_node_handle_t nodeHandle, const twai_rx_done_event_data_t* eventData,
                                        void* user_ctx)
{
	// Is it our handle?
	if (nodeHandle != *g_nodeHandle)
		return false;

	// NULL check
	if (eventData == NULL)
		return false;

	// Get the message
	uint8_t buffer[CAN_FRAME_DATA_LENGTH_B] = { 0x00 };
	twai_frame_t rxFrame = {
		.buffer = &buffer[0],
		.buffer_len = CAN_FRAME_DATA_LENGTH_B,
	};
	twai_node_receive_from_isr(nodeHandle, &rxFrame);

	// esp_rom_printf("Received frame with message id %d from sender %d with buffer %d %d %d %d %d %d %d %d \n", rxFrame.header.id >> 21, rxFrame.header.id & 0x1FFFFF, rxFrame.buffer[0], rxFrame.buffer[1], rxFrame.buffer[2], rxFrame.buffer[3], rxFrame.buffer[4], rxFrame.buffer[5], rxFrame.buffer[6], rxFrame.buffer[7]);

	// Do we have any queues to notify?
	if (g_amountOfQueuesToNotifyReceived == 0)
		return false;

	// Build the event
	QueueEvent_t event;
	event.command = RECEIVED_NEW_CAN_MESSAGE;
	event.canFrame = rxFrame;

	// Yes, notify all of them
	BaseType_t xHigherPriorityTaskWoken;
	for (uint8_t i = 0; i < g_amountOfQueuesToNotifyReceived; i++) {
		xQueueSendFromISR(*g_queuesToNotify[i], &event, &xHigherPriorityTaskWoken);
	}

	return xHigherPriorityTaskWoken;
}

bool onCanStateChanged(twai_node_handle_t handle, const twai_state_change_event_data_t *edata, void *user_ctx) {
	// Check if the CAN bus crashed otherwise ignore the state change
	if (edata->new_sta != TWAI_ERROR_BUS_OFF) {
		return false;
	}

	esp_rom_printf("State changed from %d to %d\n", edata->old_sta, edata->new_sta);
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

bool onCanError(twai_node_handle_t handle, const twai_error_event_data_t *edata, void *user_ctx)
{
	// esp_rom_printf("Acknowledge error: %d\n", edata->err_flags.ack_err);
	// esp_rom_printf("Arb lost: %d\n", edata->err_flags.arb_lost);
	// esp_rom_printf("Bit error: %d\n", edata->err_flags.bit_err);
	// esp_rom_printf("Form error: %d\n", edata->err_flags.form_err);
	// esp_rom_printf("Stuff error: %d\n", edata->err_flags.stuff_err);

	return false;
}

/*
 *	Public functions
 */
void canInitiateFrame(TwaiFrame_t* frame, const uint8_t frameId, const uint32_t senderId, const uint8_t bufferLen)
{
	if (frame == NULL) {
		return;
	}

	// Activate the 29-bit ID format
	frame->espidfFrame.header.ide = true;

	// Build the frame ID
	frame->espidfFrame.header.id = 0;
	frame->espidfFrame.header.id += frameId << CAN_MESSAGE_ID_OFFSET;
	frame->espidfFrame.header.id += senderId & 0x1FFFFF; // Zero the top 8-bit

	// Set the buffer ptr
	frame->espidfFrame.buffer = &frame->buffer[0];

	// Set all unused buffer bytes to 0
	for (uint8_t i = bufferLen; i < CAN_FRAME_MAX_BUFFER_LENGTH_B; i++) {
		frame->espidfFrame.buffer[i] = 0;
	}

	// Set the buffer length
	frame->espidfFrame.header.dlc = bufferLen;
	frame->espidfFrame.buffer_len = bufferLen;

	// Not yet transmitted
	frame->transmitting = false;
}

bool canQueueFrame(const TwaiFrame_t* frame)
{
	if (g_nodeHandle == NULL) {
		return false;
	}

	// Keep track of the frame
	const TwaiFrame_t* trackedFrameInstance = keepTrackOfFrame(frame);
	if (trackedFrameInstance == NULL) {
		return false;
	}

	// Send the frame via can bus
	return twai_node_transmit(*g_nodeHandle, &trackedFrameInstance->espidfFrame, CAN_FRAME_TRANSMIT_TIMEOUT_AFTER_MS) == ESP_OK;
}

twai_node_handle_t* canInitializeNode(const uint8_t txGpio, const uint8_t rxGpio)
{
	// Allocate memory for the node handler
	g_nodeHandle = (twai_node_handle_t*)malloc(sizeof(twai_node_handle_t));
	memset(g_nodeHandle, 0, sizeof(twai_node_handle_t));

	// Did it work?
	if (g_nodeHandle == NULL) {
		return g_nodeHandle;
	}

	// Create the node config
	const twai_onchip_node_config_t nodeConfig = {
		.io_cfg.tx = txGpio,
		.io_cfg.rx = rxGpio,
		.bit_timing.bitrate = CAN_BUS_SPEED,
		.tx_queue_depth = CAN_QUEUE_DEPTH,
	};

	// Initialize the node
	if (twai_new_node_onchip(&nodeConfig, g_nodeHandle) != ESP_OK) {
		// It failed, so destroy it
		canDestroyNode();

		return NULL;
	}

	// Register callbacks
	const twai_event_callbacks_t cbs = {
		.on_tx_done = transmitOfMessageDoneCb,
		.on_rx_done = receivedMessageCb,
		.on_state_change = onCanStateChanged,
		.on_error = onCanError,
	};
	if (twai_node_register_event_callbacks(*g_nodeHandle, &cbs, NULL) != ESP_OK) {
		// Destroy the node
		canDestroyNode();

		return NULL;
	}

	return g_nodeHandle;
}

void canDestroyNode()
{
	// Is it NULL?
	if (g_nodeHandle == NULL)
		return;

	// Delete the can node
	twai_node_delete(*g_nodeHandle);

	// Then free the memory
	free(g_nodeHandle);

	// Finally set it to NULL
	g_nodeHandle = NULL;
}

bool canEnableNode()
{
	if (g_nodeHandle == NULL)
		return false;

	g_nodeEnabled = twai_node_enable(*g_nodeHandle) == ESP_OK;

	return g_nodeEnabled;
}

void canDisableNode()
{
	if (g_nodeHandle == NULL)
		return;

	twai_node_disable(*g_nodeHandle);

	g_nodeEnabled = false;
}

esp_err_t canRecoverDriver()
{
	return twai_node_recover(*g_nodeHandle);
}

bool canRegisterRxCbQueue(QueueHandle_t* queueHandle)
{
	// Is the node enabled?
	const bool wasEnabledBefore = g_nodeEnabled;
	if (g_nodeEnabled) {
		// Stop it
		canDisableNode();
	}

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

	// Re-enable node if necessary
	if (wasEnabledBefore) {
		// Start it
		canEnableNode();
	}

	return true;
}
