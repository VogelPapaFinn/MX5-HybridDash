#include "LogFileServer.hpp"

/*
 *   constexpr
 */
constexpr auto TAG = "LogFileServer";

constexpr unsigned int FILE_TRANSMISSION_CHUNK_SIZE_B = 1024;
constexpr unsigned int HTTPD_STACK_SIZE_B = 6144;

/*
 *   Public Function Implementations
 */
LogFileServer::LogFileServer(Filesystem* p_fs, Config* p_config, WifiHost* p_wifiHost)
{
	fs_ = p_fs;
	config_ = p_config;
	wifi_ = p_wifiHost;

	webserverConfig_.task_priority = 1;
	webserverConfig_.stack_size = HTTPD_STACK_SIZE_B;
	webserverConfig_.uri_match_fn = httpd_uri_match_wildcard;
}

bool LogFileServer::start() { return startAp() && startWebserver(); }

void LogFileServer::stop()
{
	stopWebserver();
	stopAp();
}

/*
 *   Private Function Implementations
 */
bool LogFileServer::startAp()
{
	if (wifi_ == nullptr) {
		return false;
	}

	const auto json = config_->getReadAccess();

	// Setup
	wifi_->setSsid(json["WifiHost"]["ssid"]);
	wifi_->setPassword(json["WifiHost"]["password"]);

	return wifi_->start();
}
void LogFileServer::stopAp() { wifi_->stop(); }

bool LogFileServer::startWebserver()
{
	if (webserver_ != nullptr) {
		return false;
	}

	// Start server
	if (httpd_start(&webserver_, &webserverConfig_) != ESP_OK) {
		ESP_LOGW(TAG, "Failed to start Webserver");
		return false;
	}

	// Register URI
	csvUri_ = {.uri = "/*", .method = HTTP_GET, .handler = csvDownloadHandler, .user_ctx = this, .is_websocket = false};

	return httpd_register_uri_handler(webserver_, &csvUri_) == ESP_OK;
}

void LogFileServer::stopWebserver()
{
	if (webserver_ == nullptr) {
		return;
	}

	httpd_unregister_uri(webserver_, csvUri_.uri);
	httpd_stop(webserver_);
	webserver_ = nullptr;
}

bool LogFileServer::transmitFile(httpd_req_t* p_req, FILE* p_file)
{
	if (p_file == nullptr) {
		return false;
	}

	char chunk[FILE_TRANSMISSION_CHUNK_SIZE_B] = {0x00};
	unsigned int chunkSize = 0;

	do {
		chunkSize = fread(chunk, 1, FILE_TRANSMISSION_CHUNK_SIZE_B, p_file);

		// Everything transmitted
		if (chunkSize == 0) {
			continue;
		}

		if (httpd_resp_send_chunk(p_req, chunk, chunkSize) != ESP_OK) {
			ESP_LOGW(TAG, "Failed to transmit requested file");
			return false;
		}
	}
	while (chunkSize != 0);

	httpd_resp_send_chunk(p_req, nullptr, 0); // Indicates that the transmission is finished

	return true;
}

esp_err_t LogFileServer::csvDownloadHandler(httpd_req_t* p_req)
{
	if (p_req == nullptr || p_req->user_ctx == nullptr) {
		return ESP_FAIL;
	}

	auto server = static_cast<LogFileServer*>(p_req->user_ctx);

	// Extract filepath
	std::string filepath = "";
	if (strcmp(p_req->uri, "") == 0) {
		return ESP_ERR_INVALID_ARG;
	}
	filepath = p_req->uri;
	filepath.erase(filepath.begin(), filepath.begin() + 1); // Remove the leading '/'

	// Check file type
	if (!filepath.ends_with(".csv")) {
		ESP_LOGW(TAG, "Requested file '%s' is not a .csv file!", filepath.c_str());
		httpd_resp_send_404(p_req);
		return ESP_FAIL;
	}
	httpd_resp_set_type(p_req, "text/csv");

	// Check if file exists
	if (!server->fs_->doesFileExist(filepath, Filesystem::SD_CARD)) {
		ESP_LOGW(TAG, "Couldn't find requested file: %s", filepath.c_str());
		httpd_resp_send_404(p_req);
		return ESP_FAIL;
	}

	// Open the file
	FILE* csvFile = server->fs_->openFile(filepath, "r", Filesystem::SD_CARD);
	if (csvFile == nullptr) {
		ESP_LOGW(TAG, "Couldn't open requested file: %s", filepath.c_str());
		httpd_resp_send_404(p_req);
		return ESP_FAIL;
	}

	// Transmit the file
	if (!server->transmitFile(p_req, csvFile)) {
		fclose(csvFile);
		ESP_LOGW(TAG, "Failed to transmit requested file: %s", filepath.c_str());
		return ESP_FAIL;
	}

	fclose(csvFile);

	return ESP_OK;
}
