#include "Sensors/SwSensorReader.hpp"

// Project includes
#include "Sensors/SwSensor/BitfieldSensors.hpp"
#include "Sensors/SwSensor/NumericalSensors.hpp"

/*
 *   constexpr
 */
constexpr auto TAG = "SwSensorReader";

// constexpr auto SENSORS_TO_READ = { };

/*
 *   Public Function Implementations
 */
SwSensorReader::SwSensorReader(SensorValues* p_sensorValues) { sensorValues_ = p_sensorValues; }

void SwSensorReader::start() {}

void SwSensorReader::stop() {}

/*
 *   Private Function Implementations
 */
void SwSensorReader::startReadThread() {}

void SwSensorReader::stopReadThread() {}
