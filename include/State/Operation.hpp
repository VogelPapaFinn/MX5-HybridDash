#pragma once

// C++ includes
#include "fstream"

// Project includes
#include "Can.hpp"
#include "Driver/KLine.hpp"
#include "Sensor/PassiveSensor.hpp"
#include "State/State.hpp"
#include "WebInterface/WebInterface.hpp"

/*
 *	Public struct
 */
struct SensorContext
{
	SystemContext* sysCon = nullptr;
	std::vector<Sensor*>* sensors = nullptr;
};

/*
 *	Class implementation
 */
class Operation : public State
{
public:
	Operation(SystemContext* p_sysCon);

	~Operation();

	void enter() override;

	void executeDisplayUpdate(const uint8_t displayId) const;

private:
	/*
	 *	Private Functions
	 */
	void registerToEvents();

	void handleCanFrame(const Can::Frame* p_frame) const;

	void setupPassiveSensorReadings();

	void setupSensorBroadcasting();

	void setupSensorDataLogging();

	void logSensorData() const;

	void setupWifi() const;

	void connectDisplaysToWifi() const;

	/*
	 *	Private Variables
	 */
	SensorContext senCon_;

	std::vector<Sensor*> sensors_;
	std::unordered_map<PassiveSensor*, TimerHandle_t> passiveSensorTimers_;

	TimerHandle_t sensorDataLoggingTimer_ = nullptr;

	std::vector<EcuSensor*> ecuSensors_;

	FILE* sensorDataCsv_ = nullptr;
};
