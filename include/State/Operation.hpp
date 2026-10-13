#pragma once

// C++ includes
#include "fstream"

// Project includes
#include "Can.hpp"
#include "Driver/Display.hpp"
#include "Driver/KLine.hpp"
#include "Sensor/PassiveSensor.hpp"
#include "State/State.hpp"
#include "WebInterface/WebInterface.hpp"


/*
 *	Public struct
 */
struct SensorContext
{
	Can* can = nullptr;
	std::vector<Sensor*>* sensors = nullptr;
};

/*
 *	Class implementation
 */
class Operation : public State
{
public:
	Operation(Filesystem* p_fs, adc_oneshot_unit_handle_t p_adc1, KLine* p_kline, Wifi* p_wifi, Config* p_config,
			  Can* p_can, std::vector<Display>* p_displays);

	~Operation();

	void enter() override;

	void executeDisplayUpdate(const uint8_t displayId) const;

private:
	/*
	 *	Private Functions
	 */
	void registerToEvents();

	void handleCanFrame(const Can::Frame_t* p_frame) const;

	void setupPassiveSensorReadings();

	void setupSensorBroadcasting();

	void setupSensorDataLogging();

	void logSensorData() const;

	void setupWifi();

	void connectDisplaysToWifi() const;

	/*
	 *	Private Variables
	 */
	Filesystem* fs_ = nullptr;
	adc_oneshot_unit_handle_t adc1_ = nullptr;
	Wifi* wifi_ = nullptr;
	KLine* kline_ = nullptr;
	Config* config_ = nullptr;
	Can* can_ = nullptr;
	std::vector<Display>* displays_ = nullptr;

	SensorContext senCon_;

	std::vector<Sensor*> sensors_;
	std::unordered_map<PassiveSensor*, TimerHandle_t> passiveSensorTimers_;

	TimerHandle_t sensorDataLoggingTimer_ = nullptr;

	std::vector<EcuSensor*> ecuSensors_;

	FILE* sensorDataCsv_ = nullptr;
};
