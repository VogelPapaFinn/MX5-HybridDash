#include "Sensors/SwSensorReader.hpp"

// Project includes
#include "Sensors/SwSensor/BitfieldSensors.hpp"
#include "Sensors/SwSensor/NumericalSensors.hpp"
#include "Sensors/Utils/MutexGrabber.hpp"

// espidf includes
#include "esp_log.h"
#include "esp_timer.h"

/*
 *   constexpr
 */
constexpr auto TAG = "SwSensorReader";

constexpr uint8_t STOP_TASK = 0b0001;
constexpr uint8_t RECEIVED_KLINE_RESPONSE = 0b0010;
constexpr uint8_t TASK_BITMASK = STOP_TASK | RECEIVED_KLINE_RESPONSE;

constexpr unsigned int WAIT_FOR_KLINE_RESPONSE_MS = 200;

/*
 *   Public Function Implementations
 */
SwSensorReader::SwSensorReader(KLine* p_kline, SensorValues* p_sensorValues)
{
	kline_ = p_kline;
	sensorValues_ = p_sensorValues;

	sensorsToReadMutex_ = xSemaphoreCreateMutex();
}

SwSensorReader::~SwSensorReader() { stop(); }

void SwSensorReader::start() { startReadThread(); }

void SwSensorReader::stop() { stopReadThread(); }

void SwSensorReader::addSensor(SENSOR sensor, int64_t intervalMs)
{
	MutexGrabber mutexGrabber(sensorsToReadMutex_);

	// Ignore if already being read
	for (const auto& entry : sensorsToRead_) {
		if (entry.type != sensor || entry.intervalMs != intervalMs) {
			continue;
		}

		return;
	}

	sensorsToRead_.push_back({sensor, intervalMs, esp_timer_get_time()});
}

void SwSensorReader::addSensors(const std::vector<SENSOR>& sensors, int64_t intervalMs)
{
	for (const auto& sensor : sensors) {
		addSensor(sensor, intervalMs);
	}
}

void SwSensorReader::removeSensor(SENSOR sensor, int64_t intervalMs)
{
	MutexGrabber mutexGrabber(sensorsToReadMutex_);

	for (unsigned int i = 0; i < sensorsToRead_.size(); i++) {
		if (sensorsToRead_.at(i).type != sensor || sensorsToRead_.at(i).intervalMs != intervalMs) {
			continue;
		}

		sensorsToRead_.erase(sensorsToRead_.begin() + i);
		break;
	}
}

void SwSensorReader::removeAllSensors()
{
	MutexGrabber mutexGrabber(sensorsToReadMutex_);

	sensorsToRead_.clear();
}

/*
 *   Private Function Implementations
 */
void SwSensorReader::startReadThread()
{
	if (readSensorsTask_ != nullptr) {
		return;
	}

	if (xTaskCreate(readSensors, "ReadEcuSensors", 4096, this, 2, &readSensorsTask_) == pdPASS) {
		// Success
		return;
	}

	// Fail
	readSensorsTask_ = nullptr;
	ESP_LOGW(TAG, "Failed to start task which reads some ECU sensors");
	return;
}

void SwSensorReader::stopReadThread()
{
	if (readSensorsTask_ == nullptr) {
		return;
	}

	xTaskNotify(readSensorsTask_, STOP_TASK, eSetBits);

	// Wait until the task has really finished
	while (sensorsReadTaskRunning_) {
		vTaskDelay(pdMS_TO_TICKS(50));
	}
	readSensorsTask_ = nullptr;
}


bool SwSensorReader::doesPidMatch(const KLineMessage& message, const uint16_t& pid)
{
	auto payload = message.getPayload();

	if (payload.size() <= 2) {
		return false;
	}

	// Upper Byte
	if (payload.at(0) != ((pid >> 8) & 0xFF)) {
		return false;
	}

	// Lower Byte
	if (payload.at(1) != (pid & 0xFF)) {
		return false;
	}

	return true;
}

int SwSensorReader::extractSensorValue(KLineMessage& message, uint16_t expectedByteCount)
{
	auto payload = message.getPayload();

	if (payload.size() != 2 + expectedByteCount) { // 2 is for the PID, which has two bytes
		return -1;
	}

	// Erase PID
	payload.erase(payload.begin(), payload.begin() + 2);

	// Extract sensor value
	int output = 0;
	uint8_t i = payload.size();
	for (const auto& b : payload) {
		output += (b << ((i - 1) * 8));

		i--;
	}

	return output;
}

void SwSensorReader::handleSensorResponse(const NumericalSensor sensor, KLineMessage& response)
{
	// ERROR
	if (response.getSid() == 0x7F) {
		return;
	}

	// Check PID
	if (!doesPidMatch(response, sensor.pid)) {
		return;
	}

	// Get Raw Sensor Value
	const int rawValue = extractSensorValue(response, sensor.byteCount);
	if (rawValue == -1) {
		return;
	}

	// Get logical value
	const float value = static_cast<float>(sensor.convertFunc(rawValue));

	// Save value
	auto sensorValues = sensorValues_->getWriteAccess();
	(*sensorValues)[sensor.id] = {value, esp_timer_get_time()};
	sensorValues_->returnWriteAccess();
}

std::optional<SwSensorReader::Sensor> SwSensorReader::scheduleNextSensor()
{
	MutexGrabber mutexGrabber(sensorsToReadMutex_);

	// Return nothing if there are no sensors
	if (sensorsToRead_.empty()) {
		return {};
	}

	// Check which sensor is next
	unsigned int next = 0;
	for (unsigned int i = 0; i < sensorsToRead_.size(); i++) {
		// Compare the due time. Take the sensor with the lower one
		if (sensorsToRead_.at(i).nextDueMcS >= sensorsToRead_.at(next).nextDueMcS) {
			continue;
		}

		next = i;
	}

	return sensorsToRead_[next];
}

bool SwSensorReader::waitForStopNotification(TickType_t duration)
{
	uint32_t bits = 0;
	xTaskNotifyWait(0, TASK_BITMASK, &bits, duration);
	return bits & STOP_TASK;
}

void SwSensorReader::updateDueTime(SwSensorReader::Sensor& sensor)
{
	MutexGrabber mutexGrabber(sensorsToReadMutex_);

	// Iterate through all sensors
	for (auto& s : sensorsToRead_) {
		// Check if type and interval match
		if (s.type != sensor.type || s.intervalMs != sensor.intervalMs) {
			continue;
		}

		// Update due time if the case
		s.nextDueMcS = esp_timer_get_time() + (s.intervalMs * 1000);
		return;
	}
}

bool SwSensorReader::sendRequest(const SwSensorReader::Sensor& sensor)
{
	// Get the ECU Sensor instance
	const auto ecuSensor = getNumericalSensor(sensor.type);

	// Build KLine Message
	KLineMessage message(
		0x22, {static_cast<uint8_t>((ecuSensor.pid >> 8) & 0xFF), static_cast<uint8_t>((ecuSensor.pid >> 0) & 0xFF)});

	waitingForResponse_++;
	const bool result = kline_->send(message,
									 [this, ecuSensor](KLineMessage& response)
									 {
										 handleSensorResponse(ecuSensor, response);

										 // Notify the task before decrementing, so the task can't finish in between
										 xTaskNotify(readSensorsTask_, RECEIVED_KLINE_RESPONSE, eSetBits);
										 waitingForResponse_--;
									 });

	// We dont need to wait for a response if the send failed
	if (!result) {
		waitingForResponse_--;
	}

	return result;
}

/*
 *   Private Function Implementations
 */
void SwSensorReader::readSensors(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	auto reader = static_cast<SwSensorReader*>(p_param);

	{
		reader->sensorsReadTaskRunning_ = true;

		uint32_t bits = 0;
		while (true) {
			// Schedule next sensor
			auto nextSensor = reader->scheduleNextSensor();

			// Skip if the vector is empty (= nextSensor has no value)
			if (!nextSensor.has_value()) {
				// Wait for short duration and check if task should stop
				if (reader->waitForStopNotification(pdMS_TO_TICKS(1000))) {
					break;
				}
				continue;
			}

			// Wait if the due time is in the future
			const int64_t nextDueMcs = nextSensor.value().nextDueMcS - esp_timer_get_time();
			if (nextDueMcs > 0) {
				// Wait for stop notification
				if (reader->waitForStopNotification(pdMS_TO_TICKS(nextDueMcs / 1000))) {
					break;
				}
			}

			// Update due time
			reader->updateDueTime(nextSensor.value());

			// Skip waiting if the request couldn't be queued
			if (!reader->sendRequest(nextSensor.value())) {
				continue;
			}

			// Wait for the response
			xTaskNotifyWait(RECEIVED_KLINE_RESPONSE, RECEIVED_KLINE_RESPONSE, &bits,
							pdMS_TO_TICKS(WAIT_FOR_KLINE_RESPONSE_MS));
			if (bits & STOP_TASK) {
				break;
			}
		}

		// Wait until all pending KLine callbacks are done, as they access this task
		unsigned int failCounter = 0;
		while (reader->waitingForResponse_ > 0) {

			xTaskNotifyWait(0, TASK_BITMASK, nullptr, pdMS_TO_TICKS(WAIT_FOR_KLINE_RESPONSE_MS));

			if (++failCounter >= 10) {
				ESP_LOGW(TAG, "Didn't receive an answer to at least 1 request.");
				reader->waitingForResponse_ = 0;
				break;
			}
		}
	}

	// Cleanup
	reader->sensorsReadTaskRunning_ = false;
	vTaskDelete(nullptr);
}
