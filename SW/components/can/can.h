#pragma once

// Project includes
#include "EventQueues.h"
#include "can_messages.h"

// espidf includes
#include "esp_twai.h"
#include "esp_twai_onchip.h"

// FreeRTOS includes
#include "freertos/FreeRTOS.h"

/*
 *	Defines
 */
#define CAN_MESSAGE_ID_OFFSET 21
#define CAN_FRAME_MAX_BUFFER_LENGTH_B 8

/*
 *	Extern Variables
 */
//! \brief The id of the sender which should be used when transmitting frames
extern uint8_t g_ownCanComId;

/*
 *	Public typedefs
 */
//! \brief Struct used to keep track of frames which should be sent
typedef struct
{
	//! \brief The data buffer
	uint8_t buffer[CAN_FRAME_MAX_BUFFER_LENGTH_B];

	//! \brief Instance of the espidf twai_frame_t struct needed to actually transmit frames
	twai_frame_t espidfFrame;

	//! \brief Bool indicating if this frame is currently waiting to be send
	bool transmitting;
} TwaiFrame_t;

//! \brief An enum which represents the GUI's the display can show
typedef enum
{
	SCREEN_TEMPERATURE,
	SCREEN_SPEED,
	SCREEN_RPM,
	SCREEN_UNKNOWN = 255
} Screen_t;

/*
 *  Functions
*/
//! \brief Creates and initializes a can bus node handle and returns it
//! \param txGpio The GPIO used to send messages
//! \param rxGpio The GPIO used for receiving messages
//! \retval A ptr to a twai_node_handle_t on success. NULL on fail
twai_node_handle_t* canInitializeNode(uint8_t txGpio, uint8_t rxGpio);

//! \brief Enables the node so it can be used for transmitting/receiving
//! \retval Boolean indicating if it worked or not
bool canEnableNode();

//! \brief Function used to recover a crashed CAN driver
//! \brief esp_err_t indicating the success of the recovery
esp_err_t canRecoverDriver();

//! \brief Registers a task that should be notified once a message was received
//! \param queueHandle A pointer to a queue where to append a QUEUE_EVENT_T
//! \retval Boolean indicating if the registering was successful
bool canRegisterRxCbQueue(QueueHandle_t* queueHandle);

//! \brief Used to initiate a CAN frame
//! \param p_frame A pointer to the frame instance that should be initialized
//! \param frameId The frame ID that should be put into the header
//! \param bufferLen The length of the data buffer
void canInitiateFrame(TwaiFrame_t* p_frame, uint8_t frameId, uint8_t bufferLen);

//! \brief Used to transmit a CAN frame
//! \param p_frame A pointer to the frame instance that should be sent. The instance
//! is internaly copied and does NOT need to be kept alive
bool canQueueFrame(const TwaiFrame_t* p_frame);
