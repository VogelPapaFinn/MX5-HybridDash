#pragma once

// Project includes
#include "Driver/KLine.hpp"
#include "WifiHost.hpp"
#include "WifiJoin.hpp"


// espidf includes
#include "esp_http_server.h"

class WebInterface
{
public:
	WebInterface(Filesystem* p_fs);

	void send(int clientFD, const std::string& data) const;

	std::unordered_map<int, std::vector<uint16_t>>& getTrackedSensors();

	SemaphoreHandle_t& getSensorsMutex();

	/*
	 *	Private ISRs
	 */
	esp_err_t websocketHandler(httpd_req_t* p_reqst);

	esp_err_t displayUpdateUploadHandler(httpd_req_t* p_reqst);

	esp_err_t displayUpdateDownloadHandler(httpd_req_t* p_reqst);

	void websocketCrashed(const int fd);

private:
	/*
	 *	Private Functions
	 */
	void sendAllSensors(int clientFD) const;

	/*
	 *	Private Variables
	 */
	Filesystem* fs_ = nullptr;

	bool initialized_ = false;

	httpd_config_t httpdConfig_ = HTTPD_DEFAULT_CONFIG();
	httpd_handle_t httpdHandle_;

	SemaphoreHandle_t sensorsMutex_;
	std::unordered_map<int, std::vector<uint16_t>> trackedSensors_;
	TaskHandle_t updateSensorsDataTask_ = nullptr;

	FILE* displayUpdateFile_ = nullptr;
};
