#pragma once

// espidf includes
#include "esp_twai.h"
#include "esp_twai_onchip.h"

// FreeRTOS includes
#include "freertos/FreeRTOS.h"

// C includes
#include "can_messages.h"

// Defines
#define CAN_BUS_SPEED 1000000 // 1 MBit/s
#define CAN_QUEUE_DEPTH 5

/*
 *	Static Variables
 */


/*
 *  Functions
 */
//! \brief Creates and initializes a can bus node handle and returns it
//! \param txGpio The GPIO used to send messages
//! \param rxGpio The GPIO used for receiving messages
//! \retval A ptr to a twai_node_handle_t on success. NULL on fail
twai_node_handle_t* initializeCanNode(const uint8_t txGpio, const uint8_t rxGpio);

//! \brief Destroys the can bus node handle and frees all of its memory
void destroyCanNode();

//! \brief Enables the node so it can be used for transmitting/receiving
//! \retval Boolean indicating if it worked or not
bool enableCanNode();

//! \brief Disables the node
void disableCanNode();

//! \brief Queues a message to the can bus
//! \param message A pointer to the message that should be sent
//! \param freeMessageAfterwards If true the memory of the message will be freed after successfully queuing it
//! \param freeMessageDataAfterwards If true the memory of the data will be freed after successfully queuing it
//! \retval Boolean indicating if queuing worked or not
//! \note If the queuing fails the memory of the message WON'T be freed!
bool queueCanBusMessage(twai_frame_t* message, const bool freeMessageAfterwards, const bool freeMessageDataAfterwards);

//! \brief Registers a task that should be notified once a message was received
//! \param taskToNotify A pointer to the task handle that should be notified
//! \retval Boolean indicating if the registering was successfull
bool registerMessageReceivedCb(TaskHandle_t* taskToNotify);

//! \brief Returns a copy of the last message that was received
//! \retval A twai_frame_t
//! \note This should be called in the FreeRTOS tasks which get notified
twai_frame_t getLastReceivedMessage();
