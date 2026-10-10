# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(EXCLUDE_COMPONENTS "freemodbus" "mqtt" "coap" "libsodium" "wifi_provisioning" "protocomm" "esp_local_ctrl" "ulp" "unity" "cmock" "esp_gdbstub")

set(EXTRA_COMPONENT_DIRS src/ ${CMAKE_CURRENT_LIST_DIR}/../components)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(DisplayBoard)
