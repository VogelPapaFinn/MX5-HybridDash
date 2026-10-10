#include "State/Operation.hpp"

// Project includes
#include "DevelopmentStuff/DataSimulation.h"
#include "Driver/Display.hpp"
#include "Events.hpp"
#include "Sensor/FuelLevel.hpp"
#include "Sensor/LeftIndicator.hpp"
#include "Sensor/OilPressure.hpp"
#include "Sensor/RightIndicator.hpp"
#include "Sensor/Rpm.hpp"
#include "Sensor/Speed.hpp"
#include "Sensor/WaterTemperature.hpp"
#include "Wifi.hpp"
#include "WifiHost.hpp"
#include "WifiJoin.hpp"

// espidf includes
#include "CanGroupsAndFunctions.hpp"
#include "esp_event.h"
#include "esp_log.h"

/*
 *	constexpr
 */
constexpr auto TAG = "Operation";

constexpr auto FUEL_LEVEL_READ_INTERVAL_MS = 60000;
constexpr auto OIL_PRESSURE_READ_INTERVAL_MS = 10000;
constexpr auto WATER_TEMP_READ_INTERVAL_MS = 5000;

constexpr auto SENSOR_DATA_LOGGING_INTERVAL_MS = 1000;
constexpr auto SENSOR_DATA_SAVE_INTERVAL = 60;

constexpr auto BROADCAST_SENSOR_DATA_HZ = 100;

/*
 *	Private Static Functions
 */
static void staticBroadcastSensorData(void* p_sensorContext, esp_event_base_t, int32_t, void*)
{
	/*
	 *	Get the sensors vector
	 */
	if (p_sensorContext == nullptr) {
		return;
	}

	const auto sensorContext = static_cast<SensorContext*>(p_sensorContext);

	/*
	 *	Build basic CAN frame
	 */
	Can::Frame frame;
	frame.sender = CAN_MASTER_ID;
	frame.target = CAN_BROADCAST_ID;
	frame.group = CanFrameGroups::GROUP::SENSOR;
	frame.function = CanFrameGroups::SENSOR::BROADCAST_DATA;
	frame.dataLengthCode = 8;
	frame.answer = false;

	/*
	 *	Add sensor data
	 */
	const auto& sensors = sensorContext->sensors;

	// Fuel Level
	frame.data[0] = sensors->at(0)->get();

	// Oil Pressure
	frame.data[1] = sensors->at(1)->get();

	// Water Temperature
	frame.data[2] = sensors->at(2)->get() > 90 ? 90 : sensors->at(2)->get();

	// RPM
	frame.data[3] = sensors->at(3)->get() >> 8;
	frame.data[4] = sensors->at(3)->get() & 0xFF;

	// Speed
	frame.data[5] = sensors->at(4)->get();

	// Left Indicator
	frame.data[6] = sensors->at(5)->get();

	// Right Indicator
	frame.data[7] = sensors->at(6)->get();

	/*
	 *	Send the frame
	 */
	sensorContext->sysCon->can->queueFrame(frame);
}

/*
 *	Public Function implementations
 */
Operation::Operation(SystemContext* p_sysCon) : State(State::OPERATION)
{
	sysCon_ = p_sysCon;

	/*
	 *	Install ISR Service for the Active Sensors
	 */
	if (gpio_install_isr_service(ESP_INTR_FLAG_IRAM) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to install the ISR service");
	}

	/*
	 *	Get the current logging file index
	 */
	auto json = sysCon_->config->getJson();
	unsigned int sensorFileIndex = 0;

	// Check if an entry in the config exists
	if ((*json)["SensorDataFileIndex"].is<unsigned int>()) {
		// Yes, so pull it and increment it
		sensorFileIndex = (*json)["SensorDataFileIndex"].as<unsigned int>() + 1;
	}

	// Save the new index in the config
	(*json)["SensorDataFileIndex"] = sensorFileIndex;
	sysCon_->config->save();

	/*
	 *	Create and open the logging file
	 */
	const auto fileName = "SensorData_" + std::to_string(sensorFileIndex) + ".csv";

	// Create the file
	sysCon_->filesystem->createFile(fileName, Filesystem::SD_CARD);

	// Open it
	sensorDataCsv_ = sysCon_->filesystem->openFile(fileName, "w", Filesystem::SD_CARD);
	if (sensorDataCsv_ == nullptr) {
		ESP_LOGW(TAG, "Failed to create %s", fileName.c_str());
		return;
	}

	ESP_LOGI(TAG, "Created sensor log file %s", fileName.c_str());
}

Operation::~Operation()
{
	/*
	 *	Unregister from all events
	 */
	for (const auto& event : eventHandlers_) {
		const auto& base = std::get<0>(event);
		const auto& id = std::get<1>(event);
		const auto& handler = std::get<2>(event);

		esp_event_handler_instance_unregister(base, id, handler);
	}
	eventHandlers_.clear();

	/*
	 *	Stop all timers
	 */
	for (const auto& sensorEntry : passiveSensorTimers_) {
		xTimerStop(sensorEntry.second, portMAX_DELAY);
	}
	xTimerStop(sensorDataLoggingTimer_, portMAX_DELAY);

	/*
	 *	Delete all sensors
	 */
	for (auto& sensor : sensors_) {
		free(sensor);
	}

	/*
	 *	Remove ISR Service for the Active Sensors
	 */
	gpio_uninstall_isr_service();
}

void Operation::enter()
{
	/*
	 *	Register to all events
	 */
	registerToEvents();

	/*
	 *	Setup passive sensors
	 */
	const auto adc1 = sysCon_->adc1;
	sensors_.push_back(new FuelLevel(adc1));
	sensors_.push_back(new OilPressure(adc1));
	sensors_.push_back(new WaterTemperature(adc1));

	/*
	 *	Setup active sensors
	 */
	sensors_.push_back(new Rpm());
	sensors_.push_back(new Speed());
	sensors_.push_back(new LeftIndicator());
	sensors_.push_back(new RightIndicator());

	/*
	 *	Setup the sensor context
	 */
	senCon_.sysCon = sysCon_;
	senCon_.sensors = &sensors_;

	/*
	 *	Setup the periodic reading of each passive sensor
	 */
	setupPassiveSensorReadings();

	/*
	 *	Setup sensor data broadcasting
	 */
	setupSensorBroadcasting();

	/*
	 *	Setup the .csv logging of the sensor data
	 */
	setupSensorDataLogging();

	/*
	 *	Wifi
	 */
	setupWifi();
}

void Operation::handleCanFrame(const Can::Frame* p_frame) const
{
	if (p_frame->group != CanFrameGroups::GROUP::WIFI) {
		return;
	}

	/*
	 * Act depending on the function
	 */
	switch (p_frame->function) {
		/*
		 *	Display connected to the Wifi
		 */
		case CanFrameGroups::WIFI::JOIN_WIFI:
			{
				if (!p_frame->answer) {
					return;
				}

				static uint8_t s_counter = 0;
				esp_rom_printf("Display %d joined Wifi\n", ++s_counter);
			}
			break;

		/*
		 *	Display executed uupdate
		 */
		case CanFrameGroups::WIFI::EXECUTE_UPDATE:
			{
				if (!p_frame->answer) {
					return;
				}

				static uint8_t s_counter = 0;
				ESP_LOGI(TAG, "Display %d executed update successfully!", p_frame->sender);

				/*
				 * Restart all displays & ourselves when they are ready
				 */
				if (++s_counter >= 3) {
					// Create basic CAN frame
					Can::Frame txFrame;
					txFrame.sender = CAN_MASTER_ID;
					txFrame.target = CAN_BROADCAST_ID;
					txFrame.group = CanFrameGroups::GROUP::CONFIGURATION;
					txFrame.function = CanFrameGroups::CONFIGURATION::RESTART;

					// Send the frame
					sysCon_->can->queueFrame(txFrame);

					// Restart after 1 seconds
					vTaskDelay(pdMS_TO_TICKS(1000));
					esp_restart();
				}

				/*
				 *	Execute the update on the next display
				 */
				executeDisplayUpdate(sysCon_->displays.at(s_counter)->getCanId());
			}
			break;

		default:
			break;
	}
}

/*
 *	Private Function Implementations
 */
void Operation::registerToEvents()
{
	/*
	 *	CAN frame received
	 */
	eventHandlers_.push_back(std::make_tuple(SYSTEM_EVENT_BASE, CAN_FRAME_RECEIVED, esp_event_handler_instance_t()));
	esp_event_handler_instance_register(
		SYSTEM_EVENT_BASE, CAN_FRAME_RECEIVED,
		[](void* p_state, esp_event_base_t, int32_t, void* p_payload)
		{
			/*
			 *	Get the state ptr
			 */
			if (p_state == nullptr) {
				return;
			}

			// Convert it
			Operation* state = static_cast<Operation*>(p_state);

			/*
			 *	Get the payload
			 */
			if (p_payload == nullptr) {
				return;
			}

			Can::Frame* frame = static_cast<Can::Frame*>(p_payload);

			/*
			 *	Pass the register call
			 */
			state->handleCanFrame(frame);
		},
		this, &get<2>(eventHandlers_.back()));

	/*
	 *	Display update completed
	 */
	eventHandlers_.push_back(
		std::make_tuple(SYSTEM_EVENT_BASE, DISPLAY_UPDATE_DOWNLOADED, esp_event_handler_instance_t()));
	esp_event_handler_instance_register(
		SYSTEM_EVENT_BASE, DISPLAY_UPDATE_DOWNLOADED,
		[](void* p_state, esp_event_base_t, int32_t, void*)
		{
			/*
			 *	Get the state ptr
			 */
			if (p_state == nullptr) {
				return;
			}

			// Convert it
			Operation* state = static_cast<Operation*>(p_state);

			/*
			 *	Execute the update on the next display
			 */
			state->executeDisplayUpdate(state->sysCon_->displays.at(0)->getCanId());
		},
		this, &get<2>(eventHandlers_.back()));
}

void Operation::setupPassiveSensorReadings()
{
	// Fuel Level
	PassiveSensor* sensor = static_cast<PassiveSensor*>(sensors_.at(0));
	passiveSensorTimers_[sensor] =
		xTimerCreate("Periodic Fuel Level read timer", pdMS_TO_TICKS(FUEL_LEVEL_READ_INTERVAL_MS), pdTRUE, &sensors_,
					 [](const TimerHandle_t p_timerHandle)
					 {
						 /*
						  *	Get the sensors vector
						  */
						 const auto sensors = static_cast<std::vector<Sensor*>*>(pvTimerGetTimerID(p_timerHandle));

						 /*
						  *	Cast the sensor
						  */
						 const auto sensor = static_cast<PassiveSensor*>(sensors->at(0));

						 /*
						  *	Read the sensor
						  */
						 sensor->read();
					 });
	xTimerStart(passiveSensorTimers_[sensor], 0);

	// Oil Pressure
	sensor = static_cast<PassiveSensor*>(sensors_.at(1));
	passiveSensorTimers_[sensor] = xTimerCreate(
		"Periodic Oil Pressure read timer", pdMS_TO_TICKS(OIL_PRESSURE_READ_INTERVAL_MS), pdTRUE, &sensors_,
		[](const TimerHandle_t p_timerHandle)
		{
			/*
			 *	Get the sensors vector
			 */
			const auto sensors = static_cast<std::vector<Sensor*>*>(pvTimerGetTimerID(p_timerHandle));

			/*
			 *	Cast the sensor
			 */
			const auto sensor = static_cast<PassiveSensor*>(sensors->at(1));

			/*
			 *	Read the sensor
			 */
			sensor->read();
		});
	xTimerStart(passiveSensorTimers_[sensor], 0);

	// Water Temperature
	sensor = static_cast<PassiveSensor*>(sensors_.at(2));
	passiveSensorTimers_[sensor] = xTimerCreate(
		"Periodic Water Temperature read timer", pdMS_TO_TICKS(WATER_TEMP_READ_INTERVAL_MS), pdTRUE, &sensors_,
		[](const TimerHandle_t p_timerHandle)
		{
			/*
			 *	Get the sensors vector
			 */
			const auto sensors = static_cast<std::vector<Sensor*>*>(pvTimerGetTimerID(p_timerHandle));

			/*
			 *	Cast the sensor
			 */
			const auto sensor = static_cast<PassiveSensor*>(sensors->at(2));

			/*
			 *	Read the sensor
			 */
			sensor->read();
		});
	xTimerStart(passiveSensorTimers_[sensor], 0);

	/*
	 *	Initial reading of all passive sensors
	 */
	static_cast<PassiveSensor*>(sensors_.at(0))->read();
	static_cast<PassiveSensor*>(sensors_.at(1))->read();
	static_cast<PassiveSensor*>(sensors_.at(2))->read();
}

void Operation::setupSensorBroadcasting()
{
	/*
	 *	Fuel Level Changed
	 */
	eventHandlers_.push_back(std::make_tuple(SYSTEM_EVENT_BASE, FUEL_LEVEL_CHANGED, esp_event_handler_instance_t()));
	esp_event_handler_instance_register(SYSTEM_EVENT_BASE, FUEL_LEVEL_CHANGED, staticBroadcastSensorData, &senCon_,
										&get<2>(eventHandlers_.back()));

	/*
	 *	Oil Pressure Changed
	 */
	eventHandlers_.push_back(std::make_tuple(SYSTEM_EVENT_BASE, OIL_PRESSURE_CHANGED, esp_event_handler_instance_t()));
	esp_event_handler_instance_register(SYSTEM_EVENT_BASE, OIL_PRESSURE_CHANGED, staticBroadcastSensorData, &senCon_,
										&get<2>(eventHandlers_.back()));

	/*
	 *	Water Temperature Changed
	 */
	eventHandlers_.push_back(std::make_tuple(SYSTEM_EVENT_BASE, WATER_TEMP_CHANGED, esp_event_handler_instance_t()));
	esp_event_handler_instance_register(SYSTEM_EVENT_BASE, WATER_TEMP_CHANGED, staticBroadcastSensorData, &senCon_,
										&get<2>(eventHandlers_.back()));

	/*
	 *	RPM Changed
	 */
	eventHandlers_.push_back(std::make_tuple(SYSTEM_EVENT_BASE, RPM_CHANGED, esp_event_handler_instance_t()));
	esp_event_handler_instance_register(SYSTEM_EVENT_BASE, RPM_CHANGED, staticBroadcastSensorData, &senCon_,
										&get<2>(eventHandlers_.back()));

	/*
	 *	Speed Changed
	 */
	eventHandlers_.push_back(std::make_tuple(SYSTEM_EVENT_BASE, SPEED_CHANGED, esp_event_handler_instance_t()));
	esp_event_handler_instance_register(SYSTEM_EVENT_BASE, SPEED_CHANGED, staticBroadcastSensorData, &senCon_,
										&get<2>(eventHandlers_.back()));

	/*
	 *	Left Indicator Changed
	 */
	eventHandlers_.push_back(
		std::make_tuple(SYSTEM_EVENT_BASE, LEFT_INDICATOR_ACTIVE_CHANGED, esp_event_handler_instance_t()));
	esp_event_handler_instance_register(SYSTEM_EVENT_BASE, LEFT_INDICATOR_ACTIVE_CHANGED, staticBroadcastSensorData,
										&senCon_, &get<2>(eventHandlers_.back()));

	/*
	 *	Right Indicator Changed
	 */
	eventHandlers_.push_back(
		std::make_tuple(SYSTEM_EVENT_BASE, RIGHT_INDICATOR_ACTIVE_CHANGED, esp_event_handler_instance_t()));
	esp_event_handler_instance_register(SYSTEM_EVENT_BASE, RIGHT_INDICATOR_ACTIVE_CHANGED, staticBroadcastSensorData,
										&senCon_, &get<2>(eventHandlers_.back()));
}

void Operation::setupSensorDataLogging()
{
	/*
	 *	Setup data logging
	 */
	sensorDataLoggingTimer_ =
		xTimerCreate("Sensor data logging timer", pdMS_TO_TICKS(SENSOR_DATA_LOGGING_INTERVAL_MS), pdTRUE, this,
					 [](const TimerHandle_t p_timerHandle)
					 {
						 /*
						  *	Get the state instance
						  */
						 const auto state = static_cast<Operation*>(pvTimerGetTimerID(p_timerHandle));

						 /*
						  *	Trigger the logging function
						  */
						 state->logSensorData();
					 });
	xTimerStart(sensorDataLoggingTimer_, 0);

	/*
	 *	Initialize the header in the .csv file
	 */
	if (sensorDataCsv_ != nullptr) {
		fprintf(sensorDataCsv_, "FuelLevel R, FuelLevel %%, OilPressure R, WaterTemperature R, WaterTemperature °C, RPM, Speed, LIndicator, RIndicator, Coolant °C (ECU), Coolant V (ECU), Injection ms (ECU), RPM (ECU), Speed kmh (ECU)");
	}

	/*
	 *	Track specified ECU sensors and add them to the header
	 */
	// Append the sensors to the list of tracked sensors
	const auto sensorsToTrack = {ECU::COOLANT_C, ECU::COOLANT_V, ECU::INJECTION_MS, ECU::RPM, ECU::SPEED_KMH};
	for (auto& sensor : sensorsToTrack) {
		if (!ECU_SENSORS.contains(sensor)) {
			continue;
		}

		// Track sensor
		ecuSensors_.push_back(&ECU_SENSORS[sensor]);

		// Append to header
		fprintf(sensorDataCsv_, ", %s", ECU_SENSORS[sensor].name.c_str());
	}

	/*
	 *	End the header
	 */
	fprintf(sensorDataCsv_, ";\n");
	fflush(sensorDataCsv_);
	fsync(fileno(sensorDataCsv_));
}

void Operation::logSensorData() const
{
	/*
	 *	Error protection
	 */
	if (sensorDataCsv_ == nullptr) {
		xTimerStop(sensorDataLoggingTimer_, portMAX_DELAY);
		return;
	}

	/*
	 *	Request new data from the ECU
	 */
	for (const auto& sensor : ecuSensors_) {
		sysCon_->kline->readPid(sensor->id);
	}

	/*
	 *	Append active & passive hardware sensor data
	 */
	std::string row = "";
	for (auto& sensor : sensors_) {
		if (sensor->getType() == SENSOR::TYPE::FUEL_LEVEL || sensor->getType() == SENSOR::TYPE::OIL_PRESSURE || sensor->getType() == SENSOR::TYPE::WATER_TEMPERATURE) {
			row += std::to_string(static_cast<PassiveSensor*>(sensor)->getResistance());
			row += ", ";
			row += std::to_string(sensor->get());
		} else {
			row += std::to_string(sensor->get());
		}

		row += ", ";
	}

	/*
	 *	Append ECU sensor data
	 */
	for (const auto& ecuSensor : ecuSensors_) {
		row += std::to_string(ecuSensor->getConvertedValue());

		// Check if we need to append a comma at the end
		if (ecuSensor != ecuSensors_.back()) {
			row += ", ";
		}
	}

	// Finish the row
	row += ';';

	/*
	 *	Write data to the .csv file
	 */
	fprintf(sensorDataCsv_, "%s\n", row.c_str());

	/*
	 *	Flush to actual file every X seconds
	 */
	static unsigned int s_counter = 0;
	if (++s_counter % SENSOR_DATA_SAVE_INTERVAL == 0) {
		fflush(sensorDataCsv_);
		fsync(fileno(sensorDataCsv_));
	}
}

void Operation::setupWifi() const
{
	Wifi::WIFI_TYPE wifiType = Wifi::WIFI_TYPE::HOST;
	const auto json = sysCon_->config->getJson();

	/*
	 *	Delete old Wifi
	 */
	if (sysCon_->wifi != nullptr) {
		free(sysCon_->wifi);
		sysCon_->wifi = nullptr;
	}

	/*
	 *	Get Wifi mode
	 */
	if ((*json)["ForceWifiMode"].is<unsigned int>()) {
		wifiType = static_cast<Wifi::WIFI_TYPE>((*json)["ForceWifiMode"].as<int>());
	}

	/*
	 *	Setup Wifi Host
	 */
	if (wifiType == Wifi::WIFI_TYPE::HOST) {
		// Create Wifi
		sysCon_->wifi = new WifiHost(sysCon_);

		// Initialize Wifi
		sysCon_->wifi->setSsid((*json)["WifiHost"]["ssid"]);
		sysCon_->wifi->setPassword((*json)["WifiHost"]["password"]);
	}

	/*
	 *	Setup Wifi Join
	 */
	else if (wifiType == Wifi::WIFI_TYPE::JOIN) {
		// Create Wifi
		sysCon_->wifi = new WifiJoin(sysCon_);

		// Initialize Wifi
		sysCon_->wifi->setSsid((*json)["WifiJoin"]["ssid"]);
		sysCon_->wifi->setPassword((*json)["WifiJoin"]["password"]);
	}

	// Error
	else {
		ESP_LOGW(TAG, "Failed to initialize wifi. Continuing without it");
		return;
	}

	/*
	 *	Start Wifi
	 */
	sysCon_->wifi->callOnSuccess(
		[this]
		{
			// Create and start WebInterface
			sysCon_->webInterface = new WebInterface(sysCon_);

			// Initialize display Wifi
			this->connectDisplaysToWifi();
		});

	sysCon_->wifi->start();
}


void Operation::connectDisplaysToWifi() const
{
	ESP_LOGI(TAG, "Starting to transmit SSID and Password to the displays");

	/*
	 *	Transmit own IP
	 */
	// Build the basic CAN frame
	Can::Frame transmitMasterIpFrame;
	transmitMasterIpFrame.sender = CAN_MASTER_ID;
	transmitMasterIpFrame.target = CAN_BROADCAST_ID;
	transmitMasterIpFrame.group = CanFrameGroups::GROUP::WIFI;
	transmitMasterIpFrame.function = CanFrameGroups::WIFI::SET_MASTER_IP;
	transmitMasterIpFrame.dataLengthCode = 4;

	// Add the master IP address
	const auto& ip = sysCon_->wifi->getIp();
	transmitMasterIpFrame.data[0] = ip[0];
	transmitMasterIpFrame.data[1] = ip[1];
	transmitMasterIpFrame.data[2] = ip[2];
	transmitMasterIpFrame.data[3] = ip[3];

	// Send it
	sysCon_->can->queueFrame(transmitMasterIpFrame);

	/*
	 *	Split the SSID into transferable packages
	 */
	const auto& ssid = sysCon_->wifi->getSsid();
	std::vector<std::vector<char>> allSsidPackages;
	std::vector<char> ssidPackage;

	// Split the ssid up into packages with a size of max 8 bytes/chars
	for (const auto& c : ssid) {
		if (ssidPackage.size() >= 8) {
			allSsidPackages.push_back(ssidPackage);
			ssidPackage.clear();
		}

		ssidPackage.push_back(c);
	}

	// Add the last package too
	allSsidPackages.push_back(ssidPackage);

	/*
	 * Transmit the SSID
	 */
	for (const auto& package : allSsidPackages) {
		// Build the basic CAN frame
		Can::Frame ssidPackageFrame;
		ssidPackageFrame.sender = CAN_MASTER_ID;
		ssidPackageFrame.target = CAN_BROADCAST_ID;
		ssidPackageFrame.group = CanFrameGroups::GROUP::WIFI;
		ssidPackageFrame.function = CanFrameGroups::WIFI::SET_SSID;
		ssidPackageFrame.dataLengthCode = package.size();

		// Fill in the SSID package
		std::copy(package.begin(), package.end(), ssidPackageFrame.data);

		// Send the frame
		sysCon_->can->queueFrame(ssidPackageFrame);
	}

	/*
	 *	Split the Password into transferable packages
	 */
	const auto& password = sysCon_->wifi->getPassword();
	std::vector<std::vector<char>> allPsswdPackages;
	std::vector<char> psswdPackage;

	// Split the password up into packages with a size of max 8 bytes/chars
	for (const auto& c : password) {
		if (psswdPackage.size() >= 8) {
			allPsswdPackages.push_back(psswdPackage);
			psswdPackage.clear();
		}

		psswdPackage.push_back(c);
	}

	// Add the last package too
	allPsswdPackages.push_back(psswdPackage);

	/*
	 * Transmit the password
	 */
	for (const auto& package : allPsswdPackages) {
		// Build the basic CAN frame
		Can::Frame passwordPackageFrame;
		passwordPackageFrame.sender = CAN_MASTER_ID;
		passwordPackageFrame.target = CAN_BROADCAST_ID;
		passwordPackageFrame.group = CanFrameGroups::GROUP::WIFI;
		passwordPackageFrame.function = CanFrameGroups::WIFI::SET_PASSWORD;
		passwordPackageFrame.dataLengthCode = package.size();

		// Fill in the password package
		std::copy(package.begin(), package.end(), passwordPackageFrame.data);

		// Send the frame
		sysCon_->can->queueFrame(passwordPackageFrame);
	}

	/*
	 *	Instruct the displays to join the Wifi
	 */
	Can::Frame joinWifiFrame;
	joinWifiFrame.sender = CAN_MASTER_ID;
	joinWifiFrame.target = CAN_BROADCAST_ID;
	joinWifiFrame.group = CanFrameGroups::GROUP::WIFI;
	joinWifiFrame.function = CanFrameGroups::WIFI::JOIN_WIFI;
	joinWifiFrame.dataLengthCode = 0;

	sysCon_->can->queueFrame(joinWifiFrame);
}

void Operation::executeDisplayUpdate(const uint8_t displayId) const
{
	ESP_LOGI(TAG, "Executing update for display with ID %d!", displayId);

	// Build the basic CAN frame
	Can::Frame frame;
	frame.sender = CAN_MASTER_ID;
	frame.target = displayId;
	frame.group = CanFrameGroups::GROUP::WIFI;
	frame.function = CanFrameGroups::WIFI::EXECUTE_UPDATE;

	// Send the frame
	sysCon_->can->queueFrame(frame);
}
