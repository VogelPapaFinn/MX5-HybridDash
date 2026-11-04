#pragma once

// Project includes
#include "Config.hpp"
#include "Filesystem.hpp"
#include "WifiHost.hpp"

// espidf includes
#include <esp_http_server.h>

class LogFileServer
{
public:
	LogFileServer(Filesystem* p_fs, Config* p_config, WifiHost* p_wifiHost);

	bool start();

	void stop();

private:
	/*
	 *   Private Functions
	 */
	bool startAp();
	void stopAp();

	bool startWebserver();
	void stopWebserver();

	bool transmitFile(httpd_req_t* p_req, FILE* p_file);

	static esp_err_t csvDownloadHandler(httpd_req_t* p_req);

	/*
	 *   Private Variables
	 */
	Filesystem* fs_ = nullptr;
	Config* config_ = nullptr;
	WifiHost* wifi_ = nullptr;

	httpd_handle_t webserver_ = nullptr;
	httpd_config_t webserverConfig_ = HTTPD_DEFAULT_CONFIG();
	httpd_uri_t csvUri_ = {};
};
