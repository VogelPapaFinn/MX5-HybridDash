# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(EXCLUDE_COMPONENTS "freemodbus" "mqtt" "coap" "libsodium" "wifi_provisioning" "protocomm" "esp_local_ctrl" "ulp" "unity" "cmock" "esp_gdbstub")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Deactivate the Webinterface
add_compile_definitions(DISABLE_WEBINTERFACE)

set(EXTRA_COMPONENT_DIRS src/ ${CMAKE_CURRENT_LIST_DIR}/../components)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(SensorBoard)

# Copy all binaries and the .elf (needed to decode core dumps) into SW/saved-bins after every build
add_custom_target(save_bins ALL
	COMMAND ${CMAKE_COMMAND}
		-DBUILD_DIR=${CMAKE_BINARY_DIR}
		-DOUT_DIR=${CMAKE_CURRENT_LIST_DIR}/../saved-bins
		-DAPP_NAME=${CMAKE_PROJECT_NAME}
		-P ${CMAKE_CURRENT_LIST_DIR}/save_bins.cmake
	VERBATIM)
add_dependencies(save_bins app bootloader partition_table_bin littlefs_data_bin littlefs_config_bin)
