#pragma once

// Project includes
#include "SystemContext.hpp"

// C++ includes
#include <array>
#include <string>
#include <functional>

class Wifi
{
public:
	/*
	 *	Public Enums
	 */
	enum class WIFI_TYPE
	{
		HOST,
		JOIN
	};

	/*
	 *	Public Functions
	 */
	Wifi(SystemContext* p_sysCon, WIFI_TYPE type);

	virtual ~Wifi() = default;

	virtual bool start() { return false; }

	virtual void stop()
	{
	}

	void callOnSuccess(const std::function<void()>& cb);

	void setSSID(std::string ssid);

	void setPassword(std::string password);

	std::string getSSID();

	std::string getPassword();

	std::array<uint8_t, 4> getIp() const;

protected:
	/*
	 *	Variables
	 */
	SystemContext* sysCon_ = nullptr;

	WIFI_TYPE type_;

	bool connected_ = false;

	std::function<void()> callOnSuccess_;

	std::string ssid_ = "";
	std::string password_ = "";

	std::array<uint8_t, 4> ip_ = {};
};
