#include "FileServer.hpp"

// Project includes
#include "Config/ConfigEntries.hpp"

// espidf includes
#include "esp_log.h"

/*
 *   constexpr
 */
constexpr auto TAG = "FileServer";

constexpr unsigned int FILE_TRANSMISSION_CHUNK_SIZE_B = 1024;
constexpr unsigned int HTTPD_STACK_SIZE_B = 6144;

/*
 *   Public Function Implementations
 */
FileServer::FileServer(Filesystem* p_fs, Config* p_config, WifiHost* p_wifiHost)
{
	fs_ = p_fs;
	config_ = p_config;
	wifi_ = p_wifiHost;

	webserverConfig_.task_priority = 1;
	webserverConfig_.stack_size = HTTPD_STACK_SIZE_B;
	webserverConfig_.uri_match_fn = httpd_uri_match_wildcard;
}

bool FileServer::start() { return startAp() && startWebserver(); }

void FileServer::stop()
{
	stopWebserver();
	stopAp();
}

/*
 *   Private Function Implementations
 */
bool FileServer::startAp()
{
	if (wifi_ == nullptr) {
		return false;
	}

	const auto json = config_->getReadAccess();

	// Setup
	wifi_->setSsid(json[CONFIG_WIFI_HOST][CONFIG_WIFI_SSID]);
	wifi_->setPassword(json[CONFIG_WIFI_HOST][CONFIG_WIFI_PSSWD]);

	return wifi_->start();
}
void FileServer::stopAp() { wifi_->stop(); }

bool FileServer::startWebserver()
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
	csvUri_ = {.uri = "/*", .method = HTTP_GET, .handler = downloadHandler, .user_ctx = this, .is_websocket = false};

	return httpd_register_uri_handler(webserver_, &csvUri_) == ESP_OK;
}

void FileServer::stopWebserver()
{
	if (webserver_ == nullptr) {
		return;
	}

	httpd_unregister_uri(webserver_, csvUri_.uri);
	httpd_stop(webserver_);
	webserver_ = nullptr;
}

bool FileServer::setMimeType(httpd_req_t* p_req, const std::string& filepath)
{
	// .csv
	if (filepath.ends_with(".csv")) {
		httpd_resp_set_type(p_req, "text/csv");
		return true;
	}

	// .json
	if (filepath.ends_with(".json")) {
		httpd_resp_set_type(p_req, "application/json");
		return true;
	}

	// .txt
	if (filepath.ends_with(".txt")) {
		httpd_resp_set_type(p_req, "text/plain");
		return true;
	}

	return false;
}

FsLocation FileServer::getFileLocation(const std::string& filepath)
{
	// .json
	if (filepath.ends_with(".json")) {
		return FsLocation::CONFIG_PARTITION;
	}

	return FsLocation::SD_CARD;
}

bool FileServer::transmitFile(httpd_req_t* p_req, FILE* p_file)
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

esp_err_t FileServer::downloadHandler(httpd_req_t* p_req)
{
	if (p_req == nullptr || p_req->user_ctx == nullptr) {
		return ESP_FAIL;
	}

	auto server = static_cast<FileServer*>(p_req->user_ctx);

	// Extract filepath
	std::string filepath = "";
	if (strcmp(p_req->uri, "") == 0) {
		return ESP_ERR_INVALID_ARG;
	}
	filepath = p_req->uri;
	filepath.erase(filepath.begin(), filepath.begin() + 1); // Remove the leading '/'

	if (!server->setMimeType(p_req, filepath)) {
		ESP_LOGW(TAG, "Requested file '%s' is not a vaild file type!", filepath.c_str());
		httpd_resp_send_err(p_req, HTTPD_400_BAD_REQUEST, "Requested file is not a vaild file type!");
		return ESP_FAIL;
	}

	// Check if file exists
	const auto partition = server->getFileLocation(filepath);
	if (!server->fs_->doesFileExist(filepath, partition)) {
		ESP_LOGW(TAG, "Couldn't find requested file: %s", filepath.c_str());
		httpd_resp_send_404(p_req);
		return ESP_FAIL;
	}

	// Open the file
	FILE* file = server->fs_->openFile(filepath, "r", partition);
	if (file == nullptr) {
		ESP_LOGW(TAG, "Couldn't open requested file: %s", filepath.c_str());
		httpd_resp_send_404(p_req);
		return ESP_FAIL;
	}

	// Transmit the file
	if (!server->transmitFile(p_req, file)) {
		fclose(file);
		ESP_LOGW(TAG, "Failed to transmit requested file: %s", filepath.c_str());
		return ESP_FAIL;
	}

	fclose(file);

	return ESP_OK;
}
