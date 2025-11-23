#include "../can.h"
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
twai_node_handle_t* nodeHandle_ = NULL;

//! \brief Indicates if the node is currently enabled or not
bool nodeEnabled_ = false;

//! \brief An array keeping track of all current active messages
ActiveMessage_t* activeMessages_[CAN_QUEUE_DEPTH];

//! \brief The last received message
twai_frame_t lastReceivedMessage_;
//! \brief Contains the data of the last received message, lastReceivedMessage_
//! keeps track of it
uint8_t lastReceivedMessageData_[8];

//! \brief A list of FreeRTOS task that should be notified once a message was
//! received
TaskHandle_t** tasksToNotifyOnMsgReceived_ = NULL;
//! \brief The amount of registered FreeRTOS task handles
uint8_t amountOfTasksToNotifyOnMsgReceived_ = 0;

/*
 *	Private functions
 */
//! \brief Callback function used to free can bus messages after they were sent
static IRAM_ATTR bool transmitOfMessageDoneCb(const twai_node_handle_t nodeHandle,
											  const twai_tx_done_event_data_t* eventData, void* userCtx)
{
	// Is it our handle?
	if (nodeHandle != *nodeHandle_)
		return false;

	// NULL check
	if (eventData == NULL)
		return false;

	// Check if it was one of our messages
	for (uint8_t i = 0; i < CAN_QUEUE_DEPTH; i++) {
		// Is it a slot with a message in it?
		if (activeMessages_[i] != NULL) {
			// Is it the same frame?
			const twai_frame_t* receivedFrame = eventData->done_tx_frame;
			if (activeMessages_[i]->frame->header.id == receivedFrame->header.id &&
				activeMessages_[i]->frame->buffer_len == receivedFrame->buffer_len &&
				activeMessages_[i]->frame->buffer == receivedFrame->buffer) {
				// It is, so free it memory
				if (activeMessages_[i]->freeDataAfterwards) {
					free(activeMessages_[i]->frame->buffer);
				}
				free(activeMessages_[i]);

				// Then set it to NULL
				activeMessages_[i] = NULL;

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
	if (nodeHandle != *nodeHandle_)
		return false;

	// NULL check
	if (eventData == NULL)
		return false;

	// Get the message
	twai_frame_t rxFrame = {
		.buffer = lastReceivedMessageData_,
		.buffer_len = sizeof(lastReceivedMessageData_),
	};
	twai_node_receive_from_isr(nodeHandle, &rxFrame);

	// Save the message
	lastReceivedMessage_ = rxFrame;

	// Do we have any tasks to notify?
	if (amountOfTasksToNotifyOnMsgReceived_ == 0)
		return false;

	// Yes, notify all of them
	BaseType_t higherPrioWoken = pdFALSE;
	for (uint8_t i = 0; i < amountOfTasksToNotifyOnMsgReceived_; i++) {
		BaseType_t higherPrioWokenIt = pdFALSE;

		// Notify the task
		xTaskNotifyFromISR(*tasksToNotifyOnMsgReceived_[i], 0, eNoAction, &higherPrioWokenIt);

		higherPrioWoken |= higherPrioWokenIt;
	}

	return higherPrioWoken;
}

/*
 *	Public functions
 */

twai_node_handle_t* initializeCanNode(const uint8_t txGpio, const uint8_t rxGpio)
{
	// Allocate memory for the node handler
	nodeHandle_ = (twai_node_handle_t*)malloc(sizeof(twai_node_handle_t));
	memset(nodeHandle_, 0, sizeof(twai_node_handle_t));

	// Did it work?
	if (nodeHandle_ == NULL) {
		return nodeHandle_;
	}

	// Create the node config
	const twai_onchip_node_config_t nodeConfig = {
		.io_cfg.tx = txGpio,
		.io_cfg.rx = rxGpio,
		.bit_timing.bitrate = CAN_BUS_SPEED,
		.tx_queue_depth = CAN_QUEUE_DEPTH,
	};

	// Initialize the node
	if (twai_new_node_onchip(&nodeConfig, nodeHandle_) != ESP_OK) { // It failed, so destroy it
		destroyCanNode();

		return NULL;
	}

	// Register callbacks
	const twai_event_callbacks_t cbs = {
		.on_tx_done = transmitOfMessageDoneCb,
		.on_rx_done = receivedMessageCb,
		.on_state_change = NULL,
		.on_error = NULL,
	};
	if (twai_node_register_event_callbacks(*nodeHandle_, &cbs, NULL) != ESP_OK) {
		// Destroy the node
		destroyCanNode();

		return NULL;
	}

	return nodeHandle_;
}

void destroyCanNode()
{
	// Is it NULL?
	if (nodeHandle_ == NULL)
		return;

	// Delete the can node
	twai_node_delete(*nodeHandle_);

	// Then free the memory
	free(nodeHandle_);

	// Finally set it to NULL
	nodeHandle_ = NULL;
}

bool enableCanNode()
{
	if (nodeHandle_ == NULL)
		return false;

	nodeEnabled_ = twai_node_enable(*nodeHandle_) == ESP_OK;

	return nodeEnabled_;
}

void disableCanNode()
{
	if (nodeHandle_ == NULL)
		return;

	twai_node_disable(*nodeHandle_);

	nodeEnabled_ = false;
}

bool queueCanBusMessage(twai_frame_t* message, const bool freeMessageAfterwards, const bool freeMessageDataAfterwards)
{
	// NULL check
	if (nodeHandle_ == NULL)
		return false;

	// NULL check
	if (message == NULL)
		return false;

	// Queue it
	const bool transmitSuccessful = twai_node_transmit(*nodeHandle_, message, 0) == ESP_OK;

	// Keep track of the message?
	if (transmitSuccessful && freeMessageAfterwards) {
		// Yes, so keep track of it
		for (uint8_t i = 0; i < CAN_QUEUE_DEPTH; i++) {
			// Is it a slot with a message in it?
			if (activeMessages_[i] == NULL) {
				// Allocate the memory
				activeMessages_[i] = malloc(sizeof(ActiveMessage_t));
				memset(activeMessages_[i], 0, sizeof(ActiveMessage_t));

				// Set the data
				activeMessages_[i]->frame = message;
				activeMessages_[i]->freeDataAfterwards = freeMessageDataAfterwards;
			}
		}
	}

	return transmitSuccessful;
}

bool registerMessageReceivedCb(TaskHandle_t* taskToNotify)
{
	// Is the node enabled?
	bool wasEnabledBefore = nodeEnabled_;
	if (nodeEnabled_) {
		// Stop it
		disableCanNode();
	}

	// Make the array larger
	const void* newAddr =
		realloc(tasksToNotifyOnMsgReceived_, sizeof(TaskHandle_t*) * (amountOfTasksToNotifyOnMsgReceived_ + 1));

	// Did it work?
	if (newAddr != NULL) {
		tasksToNotifyOnMsgReceived_ = (TaskHandle_t**)newAddr;
	}
	else {
		return false;
	}

	// Increase the counter
	amountOfTasksToNotifyOnMsgReceived_++;

	// Then save the task handle
	tasksToNotifyOnMsgReceived_[amountOfTasksToNotifyOnMsgReceived_ - 1] = taskToNotify;

	// Re-enable node if necessary
	if (wasEnabledBefore) {
		// Start it
		enableCanNode();
	}

	return true;
}

twai_frame_t getLastReceivedMessage() { return lastReceivedMessage_; }
