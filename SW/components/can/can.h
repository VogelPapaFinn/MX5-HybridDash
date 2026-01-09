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
extern uint8_t g_ownCanComId;

/*
 *	Public typedefs
 */
typedef struct
{
  uint8_t buffer[CAN_FRAME_MAX_BUFFER_LENGTH_B];
  twai_frame_t espidfFrame;

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
void canInitiateFrame(TwaiFrame_t* frame, uint8_t frameId, uint32_t senderId, uint8_t bufferLen);

bool canQueueFrame(const TwaiFrame_t* frame);

//! \brief Creates and initializes a can bus node handle and returns it
//! \param txGpio The GPIO used to send messages
//! \param rxGpio The GPIO used for receiving messages
//! \retval A ptr to a twai_node_handle_t on success. NULL on fail
twai_node_handle_t* canInitializeNode(uint8_t txGpio, uint8_t rxGpio);

//! \brief Destroys the can bus node handle and frees all of its memory
void canDestroyNode();

//! \brief Enables the node so it can be used for transmitting/receiving
//! \retval Boolean indicating if it worked or not
bool canEnableNode();

//! \brief Disables the node
void canDisableNode();

esp_err_t canRecoverDriver();

//! \brief Registers a task that should be notified once a message was received
//! \param queueHandle A pointer to a queue where to append a QUEUE_EVENT_T
//! \retval Boolean indicating if the registering was successful
bool canRegisterRxCbQueue(QueueHandle_t* queueHandle);
