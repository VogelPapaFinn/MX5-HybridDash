#pragma once

// C++ includes
#include "fstream"

// Project includes
#include "Can.hpp"
#include "Driver/Display.hpp"
#include "Driver/KLine.hpp"
#include "Sensor/PassiveSensor.hpp"
#include "State/OldState.hpp"
#include "WebInterface/WebInterface.hpp"

/*
 *	Class implementation
 */
class Operation : public OldState
{
public:
	Operation(Filesystem* p_fs, adc_oneshot_unit_handle_t p_adc1, KLine* p_kline, Wifi* p_wifi, Config* p_config,
			  Can* p_can, std::vector<Display>* p_displays);

	void enter() override;

	void executeDisplayUpdate(const uint8_t displayId) const;

private:
	/*
	 *	Private Functions
	 */
	void handleCanFrame(const Can::Frame_t* p_frame) const;

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

	TimerHandle_t sensorDataLoggingTimer_ = nullptr;

	std::vector<EcuSensor*> ecuSensors_;

	FILE* sensorDataCsv_ = nullptr;
};
