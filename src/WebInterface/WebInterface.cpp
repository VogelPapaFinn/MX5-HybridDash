#include "WebInterface/WebInterface.hpp"

// Project includes
#include "Driver/EcuSensors.hpp"
#include "Events.hpp"

// C++ includes
#include <sstream>

/*
 *	Prototypes
 */
static esp_err_t fileHandler(httpd_req_t* p_reqst);
static esp_err_t staticWebsocketHandler(httpd_req_t* p_reqst);

/*
 *	constexpr
 */
constexpr auto TAG = "WebInterface";

constexpr auto JSON_WIFI_MODE = "OverwriteWifiMode";
constexpr auto JSON_WIFI_HOST = "WifiHost";
constexpr auto JSON_WIFI_JOIN = "WifiJoin";

constexpr uint16_t FILE_CHUNK_SIZE_B = 1024;

constexpr uint16_t WEBSOCKET_RECV_BUFFER_B = 256;

/*
 *	Private Static ISR
 */
static std::string getMimeType(const std::string& filepath)
{
	// Get file ending
	const size_t pos = filepath.find_last_of(".");
	if (pos == std::string::npos) {
		return "text/plain";
	}

	const std::string lastDot = filepath.substr(pos + 1);
	if (lastDot == "html") {
		return "text/html";
	}
	if (lastDot == "css") {
		return "text/css";
	}
	if (lastDot == "js") {
		return "application/javascript";
	}
	if (lastDot == "png") {
		return "image/png";
	}
	if (lastDot == "jpg") {
		return "image/jpeg";
	}
	if (lastDot == "ico") {
		return "image/x-icon";
	}
	if (lastDot == "csv") {
		return "text/csv";
	}

	return "text/plain";
}

static esp_err_t fileHandler(httpd_req_t* p_reqst)
{
	if (p_reqst == nullptr) {
		return ESP_FAIL;
	}

	auto fs = static_cast<Filesystem*>(p_reqst->user_ctx);

	std::string filepath;
	Filesystem::Location location = Filesystem::DATA_PARTITION;

	if (strcmp(p_reqst->uri, "") == 0) {
		return ESP_ERR_INVALID_ARG;
	}

	std::string uri = p_reqst->uri;

	// Prüfen, ob die URL mit "/sd/" beginnt (SD-Karten Zugriff)
	if (uri.find("/sd/") == 0) {
		// Schneide "/sd/" ab, um den relativen Pfad für die SD-Karte zu erhalten
		filepath = uri.substr(4);
		location = Filesystem::SD_CARD;
	}
	// Andernfalls normales Webinterface von der Daten-Partition laden
	else {
		if (uri == "/") {
			filepath = "webinterface/index.html";
		}
		else {
			filepath = "webinterface" + uri;
		}
	}

	// Datei mit der dynamisch ermittelten Location öffnen
	FILE* reqFile = fs->openFile(filepath, "rb", location);
	if (reqFile == nullptr) {
		esp_rom_printf("[%s] Couldn't open file: %s on location: %d\n", TAG, filepath.c_str(), location);
		httpd_resp_send_404(p_reqst);
		return ESP_FAIL;
	}

	if (filepath.find(".csv") != std::string::npos) {
		httpd_resp_set_hdr(p_reqst, "Content-Disposition", "attachment");
	}

	httpd_resp_set_type(p_reqst, getMimeType(filepath).c_str());

	/*
	 * Send the file in chunks
	 */
	char chunk[FILE_CHUNK_SIZE_B] = {0x00};
	ssize_t chunkSize;
	do {
		chunkSize = fread(chunk, 1, FILE_CHUNK_SIZE_B, reqFile);

		if (chunkSize <= 0) {
			continue;
		}

		if (httpd_resp_send_chunk(p_reqst, chunk, chunkSize) != ESP_OK) {
			fclose(reqFile);
			return ESP_FAIL;
		}
	}
	while (chunkSize != 0);

	fclose(reqFile);

	// Send one last empty chunk to signalize the end of the transmission
	httpd_resp_send_chunk(p_reqst, nullptr, 0);

	return ESP_OK;
}

struct WsContext
{
	WebInterface* web = nullptr;
	int clientFd = 0;
};

static void staticWebsocketCrashedHandler(void* ctx)
{
	if (ctx == nullptr) {
		return;
	}

	const WsContext* wsContext = static_cast<WsContext*>(ctx);
	wsContext->web->websocketCrashed(wsContext->clientFd);

	delete wsContext;
}

static esp_err_t staticWebsocketHandler(httpd_req_t* p_reqst)
{
	if (p_reqst->user_ctx == nullptr) {
		return ESP_FAIL;
	}

	const auto web = static_cast<WebInterface*>(p_reqst->user_ctx);

	if (p_reqst->method == HTTP_GET) {
		ESP_LOGI(TAG, "Websocket handshake successful");

		WsContext* ctx = new WsContext();
		ctx->web = web;
		ctx->clientFd = httpd_req_to_sockfd(p_reqst);
		;

		p_reqst->sess_ctx = (void*)ctx;
		p_reqst->free_ctx = staticWebsocketCrashedHandler;
		return ESP_OK;
	}

	return web->websocketHandler(p_reqst);
}

static esp_err_t staticDisplayUpdateUploadHandler(httpd_req_t* p_reqst)
{
	if (p_reqst->user_ctx == nullptr) {
		return ESP_FAIL;
	}

	const auto web = static_cast<WebInterface*>(p_reqst->user_ctx);
	return web->displayUpdateUploadHandler(p_reqst);
}

static esp_err_t staticDisplayUpdateDownloadHandler(httpd_req_t* p_reqst)
{
	if (p_reqst->user_ctx == nullptr) {
		return ESP_FAIL;
	}

	const auto web = static_cast<WebInterface*>(p_reqst->user_ctx);
	return web->displayUpdateDownloadHandler(p_reqst);
}

static void updateSensorsTask(void* p_param)
{
	if (p_param == nullptr) {
		ESP_LOGE("updateSensorsTask", "Killed itself");
		vTaskDelete(nullptr);
	}


	while (true) {
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

/*
 *	Public Function Implementations
 */
WebInterface::WebInterface(Filesystem* p_fs)
{
	fs_ = p_fs;
	sensorsMutex_ = xSemaphoreCreateMutex();

	httpdConfig_ = HTTPD_DEFAULT_CONFIG();
	httpdConfig_.uri_match_fn = httpd_uri_match_wildcard;
	httpdConfig_.stack_size = 8192;
	httpdConfig_.lru_purge_enable = true;

	if (httpd_start(&httpdHandle_, &httpdConfig_) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to start webserver");
		return;
	}

	/*
	 *	Setup URI's
	 */
	// For web requests
	const httpd_uri_t websocketUri = {
		.uri = "/ws", .method = HTTP_GET, .handler = staticWebsocketHandler, .user_ctx = this, .is_websocket = true};
	if (httpd_register_uri_handler(httpdHandle_, &websocketUri) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to register Websocket URI");
		return;
	}

	// Uploading of display update file
	const httpd_uri_t updateFileUploadUri = {.uri = "/display-update-upload",
											 .method = HTTP_POST,
											 .handler = staticDisplayUpdateUploadHandler,
											 .user_ctx = this};
	if (httpd_register_uri_handler(httpdHandle_, &updateFileUploadUri) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to register display update upload URI");
		return;
	}

	// Downloading of display update file
	const httpd_uri_t updateFileDownloadUri = {.uri = "/display-update.bin",
											   .method = HTTP_GET,
											   .handler = staticDisplayUpdateDownloadHandler,
											   .user_ctx = this};
	if (httpd_register_uri_handler(httpdHandle_, &updateFileDownloadUri) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to register display update download URI");
		return;
	}

	const httpd_uri_t fileUri = {.uri = "/*", .method = HTTP_GET, .handler = fileHandler, .user_ctx = fs_};
	if (httpd_register_uri_handler(httpdHandle_, &fileUri) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to register File URI");
		return;
	}

	ESP_LOGI(TAG, "Initialized");
	initialized_ = true;
}

void WebInterface::send(const int clientFD, const std::string& data) const
{
	if (!initialized_) {
		return;
	}

	if (httpd_ws_get_fd_info(httpdHandle_, clientFD) != HTTPD_WS_CLIENT_WEBSOCKET) {
		ESP_LOGW(TAG, "Cant send to Frontend. Server Handle or ClientFD is invalid");
		return;
	}

	httpd_ws_frame_t frame = {};
	frame.payload = (uint8_t*)data.c_str();
	frame.len = data.length();
	frame.type = HTTPD_WS_TYPE_TEXT;
	frame.final = true;

	httpd_ws_send_frame_async(httpdHandle_, clientFD, &frame);
}

std::unordered_map<int, std::vector<uint16_t>>& WebInterface::getTrackedSensors() { return trackedSensors_; }

SemaphoreHandle_t& WebInterface::getSensorsMutex() { return sensorsMutex_; }

/*
 *	Private ISRs
 */
esp_err_t WebInterface::websocketHandler(httpd_req_t* p_reqst)
{
	const int clientFD = httpd_req_to_sockfd(p_reqst);

	// Prepare rx frame slot
	httpd_ws_frame_t frame = {};
	frame.type = HTTPD_WS_TYPE_TEXT;
	char data[WEBSOCKET_RECV_BUFFER_B] = {};
	frame.payload = reinterpret_cast<uint8_t*>(data);

	// Get frame data
	if (httpd_ws_recv_frame(p_reqst, &frame, WEBSOCKET_RECV_BUFFER_B) != ESP_OK) {
		ESP_LOGW(TAG, "Failed to fetch frame");
		return ESP_FAIL;
	}
	std::string dataStr = data;

	// DEBUG LOGGING
	ESP_LOGI(TAG, "Received string from Frontend: %s", data);

	/*
	 * Act depending on the frame data
	 */
	if (dataStr == "fetch-sensors") {
		sendAllSensors(clientFD);
		return ESP_OK;
	}

	if (dataStr.contains("add-sensor")) {
		const std::string sensorId = dataStr.substr(dataStr.find(':') + 1);

		xSemaphoreTakeRecursive(sensorsMutex_, portMAX_DELAY);
		trackedSensors_[clientFD].push_back(std::stoi(sensorId));
		xSemaphoreGiveRecursive(sensorsMutex_);

		if (updateSensorsDataTask_ != nullptr) {
			return ESP_OK;
		}

		if (xTaskCreate(updateSensorsTask, "WebInterfaceUpdateSensorsTask", 4096, fs_, 2, &updateSensorsDataTask_) !=
			pdPASS) {
			updateSensorsDataTask_ = nullptr;
			ESP_LOGW(TAG, "Failed to start task which updates the sensor values");
			return ESP_OK;
		}

		return ESP_OK;
	}

	if (dataStr.contains("remove-sensor")) {
		const std::string sensorIdStr = dataStr.substr(dataStr.find(':') + 1);
		const int sensorId = std::stoi(sensorIdStr);

		auto& sensorVector = trackedSensors_[clientFD];
		for (int i = 0; i < sensorVector.size(); i++) {
			if (sensorVector.at(i) == sensorId) {
				xSemaphoreTakeRecursive(sensorsMutex_, portMAX_DELAY);
				sensorVector.erase(sensorVector.begin() + i);
				xSemaphoreGiveRecursive(sensorsMutex_);
				break;
			}
		}

		if (trackedSensors_.empty()) {
			if (updateSensorsDataTask_ == nullptr) {
				return ESP_OK;
			}

			vTaskDelete(updateSensorsDataTask_);
			updateSensorsDataTask_ = nullptr;
		}

		return ESP_OK;
	}

	if (dataStr.contains("prepare-display-update")) {
		auto filesystem = static_cast<Filesystem*>(p_reqst->user_ctx);

		// Create file if necessary
		if (!filesystem->doesFileExist("display_update.bin", Filesystem::Location::SD_CARD)) {
			filesystem->createFile("display_update.bin", Filesystem::Location::SD_CARD);
		}

		if (displayUpdateFile_ != nullptr) {
			fclose(displayUpdateFile_);
		}

		displayUpdateFile_ = filesystem->openFile("display_update.bin", "wb+", Filesystem::Location::SD_CARD);

		return ESP_OK;
	}

	return ESP_OK;
}

esp_err_t WebInterface::displayUpdateUploadHandler(httpd_req_t* p_reqst)
{
	if (displayUpdateFile_ == nullptr) {
		ESP_LOGE(TAG, "Can't download the display update file. The destination file is a nullptr!");
		return ESP_FAIL;
	}

	ESP_LOGI(TAG, "Uploading display update file...");

	/*
	 *	Download file
	 */
	size_t remaining = p_reqst->content_len;
	int received = 0;
	char buf[1024];

	while (remaining > 0) {
		// Read file chunk
		received = httpd_req_recv(p_reqst, buf, MIN(remaining, sizeof(buf)));

		if (received <= 0) {
			// Retry on timeout
			if (received == HTTPD_SOCK_ERR_TIMEOUT) {
				continue;
			}

			// Fault
			return ESP_FAIL;
		}

		// Write into the file on the filesystem
		if (fwrite(buf, 1, received, displayUpdateFile_) != received) {
			ESP_LOGE(TAG, "Failed writing into display update file!");

			fclose(displayUpdateFile_);
			return ESP_FAIL;
		}

		remaining -= received;
	}

	// Close the file
	fclose(displayUpdateFile_);
	displayUpdateFile_ = nullptr;

	ESP_LOGI(TAG, "Upload of display update file successful!");

	/*
	 *	Notify backend that the update is ready
	 */
	esp_event_post(SYSTEM_EVENT_BASE, DISPLAY_UPDATE_DOWNLOADED, nullptr, 0, portMAX_DELAY);

	return ESP_OK;
}

esp_err_t WebInterface::displayUpdateDownloadHandler(httpd_req_t* p_reqst)
{
	/*
	 *	Open the file
	 */
	const auto filesystem = fs_;
	if (!filesystem->doesFileExist("display_update.bin", Filesystem::SD_CARD)) {
		ESP_LOGE(TAG, "Display update file does not exist!");
		return ESP_FAIL;
	}

	FILE* updateFile = filesystem->openFile("display_update.bin", "r", Filesystem::SD_CARD);

	if (updateFile == nullptr) {
		ESP_LOGE(TAG, "Couldn't open display update file!");
		return ESP_FAIL;
	}

	/*
	 *	Upload file
	 */
	// Set to binary stream
	httpd_resp_set_type(p_reqst, "application/octet-stream");

	char chunk[1024];
	size_t readBytes;
	while ((readBytes = fread(chunk, 1, sizeof(chunk), updateFile)) > 0) {
		if (httpd_resp_send_chunk(p_reqst, chunk, readBytes) != ESP_OK) {
			return ESP_FAIL;
		}
	}

	// Signalize end of stream
	httpd_resp_send_chunk(p_reqst, NULL, 0);

	ESP_LOGI(TAG, "Download of display update file successful!");

	fclose(updateFile);
	updateFile = nullptr;

	return ESP_OK;
}

void WebInterface::websocketCrashed(const int fd)
{
	if (!trackedSensors_.contains(fd)) {
		return;
	}

	trackedSensors_.erase(fd);

	if (!trackedSensors_.empty()) {
		return;
	}

	if (updateSensorsDataTask_ == nullptr) {
		return;
	}

	vTaskDelete(updateSensorsDataTask_);
	updateSensorsDataTask_ = nullptr;
}

/*
 *	Private Functions Implementations
 */
void WebInterface::sendAllSensors(const int clientFD) const
{
	std::stringstream output;
	output << "{";
	output << "\"type\":\"fetch-sensors\",";
	output << "\"sensors\":" << "[";

	// Parse all sensors to JSON
	for (auto it = ECU_SENSORS.begin(); it != ECU_SENSORS.end(); ++it) {
		output << it->second.toJson();

		if (std::next(it) != ECU_SENSORS.end()) {
			output << ",";
		}
	}

	output << "]";
	output << "}";

	send(clientFD, output.str());
}
