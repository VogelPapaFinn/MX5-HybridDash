#include "../Wifi.hpp"

/*
 *	constexpr
 */
constexpr auto TAG = "Wifi";

/*
 *	Public Function Implementations
 */
Wifi::Wifi(SystemContext* p_sysCon, WIFI_TYPE type) : type_(type)
{
	sysCon_ = p_sysCon;
}

void Wifi::callOnSuccess(const std::function<void()>& cb)
{
	callOnSuccess_ = cb;
}

void Wifi::setSSID(const std::string ssid)
{
	ssid_ = ssid;
}

void Wifi::setPassword(const std::string password)
{
	password_ = password;
}

std::string Wifi::getSSID()
{
	return ssid_;
}

std::string Wifi::getPassword()
{
	return password_;
}

std::array<uint8_t, 4> Wifi::getIp() const
{
	return ip_;
}
