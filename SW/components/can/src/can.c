// Project includes
#include "../can.h"

// C includes
#include <stdio.h>
#include <string.h>

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

//! \brief An array keeping track of all current active messages
static ActiveMessage_t* g_activeMessages[CAN_QUEUE_DEPTH];

//! \brief The last received message
static twai_frame_t g_lastReceivedMessage;
//! \brief Contains the data of the last received message, lastReceivedMessage_
//! keeps track of it
static uint8_t g_lastReceivedMessageData[8];

//! \brief A list of FreeRTOS Queues that should be notified once a message was
//! received
static QueueHandle_t** g_queuesToNotify = NULL;
//! \brief The amount of registered FreeRTOS queue handles
static uint8_t g_amountOfQueuesToNotifyReceived = 0;

/*
 *	Private functions
 */
//! \brief Callback function used to free can bus messages after they were sent
// ReSharper disable once CppDFAConstantFunctionResult
static IRAM_ATTR bool transmitOfMessageDoneCb(twai_node_handle_t nodeHandle,
                                              const twai_tx_done_event_data_t* eventData, void* userCtx)
{
	// Is it our handle?
	if (nodeHandle != *g_nodeHandle)
		return false;

	// NULL check
	if (eventData == NULL)
		return false;

	// Check if it was one of our messages
	for (uint8_t i = 0; i < CAN_QUEUE_DEPTH; i++) {
		// Is it a slot with a message in it?
		if (g_activeMessages[i] != NULL) {
			// Is it the same frame?
			const twai_frame_t* receivedFrame = eventData->done_tx_frame;
			if (g_activeMessages[i]->frame->header.id == receivedFrame->header.id &&
				g_activeMessages[i]->frame->buffer_len == receivedFrame->buffer_len &&
				g_activeMessages[i]->frame->buffer == receivedFrame->buffer) {
				// It is, so free it memory
				if (g_activeMessages[i]->freeDataAfterwards) {
					free(g_activeMessages[i]->frame->buffer);
				}
				free(g_activeMessages[i]);

				// Then set it to NULL
				g_activeMessages[i] = NULL;

				return false;
			}
		}
	}

	// No match
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
	twai_frame_t rxFrame = {
		.buffer = g_lastReceivedMessageData,
		.buffer_len = sizeof(g_lastReceivedMessageData),
	};
	twai_node_receive_from_isr(nodeHandle, &rxFrame);

	// Save the message
	g_lastReceivedMessage = rxFrame;

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

/*
 *	Public functions
 */
twai_node_handle_t* initializeCanNode(const uint8_t txGpio, const uint8_t rxGpio)
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
		destroyCanNode();

		return NULL;
	}

	// Register callbacks
	const twai_event_callbacks_t cbs = {
		.on_tx_done = transmitOfMessageDoneCb,
		.on_rx_done = receivedMessageCb,
		.on_state_change = onCanStateChanged,
		.on_error = NULL,
	};
	if (twai_node_register_event_callbacks(*g_nodeHandle, &cbs, NULL) != ESP_OK) {
		// Destroy the node
		destroyCanNode();

		return NULL;
	}

	return g_nodeHandle;
}

void destroyCanNode()
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

bool enableCanNode()
{
	if (g_nodeHandle == NULL)
		return false;

	g_nodeEnabled = twai_node_enable(*g_nodeHandle) == ESP_OK;

	return g_nodeEnabled;
}

void disableCanNode()
{
	if (g_nodeHandle == NULL)
		return;

	twai_node_disable(*g_nodeHandle);

	g_nodeEnabled = false;
}

esp_err_t recoverCanDriver()
{
	return twai_node_recover(*g_nodeHandle);
}

#include <esp_debug_helpers.h>
bool queueCanBusMessage(twai_frame_t* message, const bool freeMessageAfterwards, const bool freeMessageDataAfterwards)
{
	// NULL check
	if (g_nodeHandle == NULL) {
		return false;
	}

	// NULL check
	if (message == NULL) {
		return false;
	}

	// Queue it
	const bool transmitSuccessful = twai_node_transmit(*g_nodeHandle, message, 0) == ESP_OK;

	// Keep track of the message?
	if (transmitSuccessful && freeMessageAfterwards) {
		// Yes, so keep track of it
		for (uint8_t i = 0; i < CAN_QUEUE_DEPTH; i++) {
			// Is it a slot with a message in it?
			if (g_activeMessages[i] == NULL) {
				// Allocate the memory
				g_activeMessages[i] = malloc(sizeof(ActiveMessage_t));
				memset(g_activeMessages[i], 0, sizeof(ActiveMessage_t));

				// Set the data
				g_activeMessages[i]->frame = message;
				g_activeMessages[i]->freeDataAfterwards = freeMessageDataAfterwards;
			}
		}
	}
	else {
		esp_backtrace_print(5);
	}

	return transmitSuccessful;
}

bool registerCanRxCbQueue(QueueHandle_t* queueHandle)
{
	// Is the node enabled?
	const bool wasEnabledBefore = g_nodeEnabled;
	if (g_nodeEnabled) {
		// Stop it
		disableCanNode();
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
		enableCanNode();
	}

	return true;
}

twai_frame_t* generateCanFrame(const uint8_t messageID, const uint32_t senderID, uint8_t** buffer,
                               const uint8_t bufferLen)
{
	// Allocate the needed memory
	twai_frame_t* message = malloc(sizeof(twai_frame_t));
	if (message == NULL) {
		return NULL;
	}
	memset(message, 0, sizeof(twai_frame_t));

	// Activate the 29-Bit ID format
	message->header.ide = true;

	// Build the frame id
	message->header.id += messageID << 21;
	message->header.id += senderID & 0x1FFFFF; // Zero the upper 11 bits of the sender ID

	// Set the buffer
	if (buffer != NULL && bufferLen > 0) {
		// Set the length
		message->header.dlc = bufferLen;
		message->buffer_len = bufferLen;

		// Set the buffer
		message->buffer = (uint8_t*)*buffer;
	}
	else {
		// There is no data
		message->header.dlc = 0;
		message->buffer_len = 0;
	}

	return message;
}
