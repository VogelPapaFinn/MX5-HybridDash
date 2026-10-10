#pragma once

/*
 *	Public enum
 */
namespace SENSOR
{
	enum class TYPE
	{
		FUEL_LEVEL,
		WATER_TEMPERATURE,
		OIL_PRESSURE,
		SPEED,
		RPM,
		LEFT_INDICATOR,
		RIGHT_INDICATOR,
	};
}

class Sensor
{
public:
	/*
	 *	Public functions
	 */
	Sensor(const SENSOR::TYPE type) { type_ = type; }

	virtual ~Sensor() = default;

	virtual int get() { return 0; }

	SENSOR::TYPE& getType() { return type_; }

protected:
	virtual void notifyAboutNewValue() {}

	SENSOR::TYPE type_;
};
