#ifndef FIRMWARE_INCLUDE_C_HEADER_TEMPLATE_H_FILEMANAGER
#define FIRMWARE_INCLUDE_C_HEADER_TEMPLATE_H_FILEMANAGER

/* --- Includes --- */
// C includes
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Project includes
#include "Logger/Logger.h"

// esp-idf includes
#include "driver/sdmmc_host.h"
#include "esp_spiffs.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

/* --- Defines & Macros --- */
#define FILEMANAGER_GPIO_CLK GPIO_NUM_16// The GPIO for the clock
#define FILEMANAGER_GPIO_CMD GPIO_NUM_17// The GPIO for the command
#define FILEMANAGER_GPIO_D0 GPIO_NUM_4  // The GPIO for the data0 line
#define FILEMANAGER_GPIO_D1 GPIO_NUM_5  // The GPIO for the data1 line
#define FILEMANAGER_GPIO_D2 GPIO_NUM_8  // The GPIO for the data2 line
#define FILEMANAGER_GPIO_D3 GPIO_NUM_18 // The GPIO for the data3 line

/* --- Variables, Typedefs etc. --- */
enum {
    LOCATION_INTERNAL = 0,
    LOCATION_SDCARD = 1,
};

/* --- Imported Variables, Typedefs etc. --- */

/* --- Global variables and function (headers) --- */
//! \brief Initializes the FileManager
//! \retval Boolean indicating if it was successful or not
bool fileManagerInit(void);

//! \brief Creates a new file at the specified location
//! \param path The path of the file including the file name and extension without "/sdcard/" or "/spiffs/"
//! \param location Where the file shall be created
//! \retval Boolean if the operation was successful
bool fileManagerCreateFile(const char *path, int location);

//! \brief Checks if a file at the specified path exists at the specified location
//! \param path The path to the file without "/sdcard/" or "/spiffs/"
//! \param location Where we should check for the file
//! \retval Boolean
bool fileManagerDoesFileExists(const char *path, const int location);

//! \brief Tries to open a file at the specified path and location
//! \param path The path to the file without "/sdcard/" or "/spiffs/"
//! \param mode The mode how the file should be opened
//! \param location Where the file is stored
//! \retval Returns a pointer to the FILE. Check for NULL!
FILE *fileManagerOpenFile(const char *path, const char *mode, const int location);

//! \brief Tries to delete a file at the specified path and location
//! \param path The path to the file without "/sdcard/" or "/spiffs/"
//! \param location Where the file is stored
//! \retval True if it was successful - False if not
bool fileManagerDeleteFile(const char *path, const int location);

//! \brief Checks if the specified directory exists on the SD Card
//! \param dir The path to the directory without "/sdcard/"
//! \retval True if the directory exists - False if it doesn't
//! \note The SPIFFS filesystem unfortunately does not support directories
bool fileManagerDoesDirectoryExist(const char *dir);

//! \brief Creates a directory at the specified path on the SD Card
//! \param path The path to the directory without "/sdcard/"
//! \retval True if it was successful - False if it wasn't
//! \note The SPIFFS filesystem unfortunately does not support directories
bool fileManagerCreateDir(const char *path);

//! \brief Deletes a directory at the specified path on the SD Card
//! \param path The path to the directory without "/sdcard/"
//! \retval True if it was successful - False if it wasn't
//! \note The SPIFFS filesystem unfortunately does not support directories
bool fileManagerDeleteDir(const char *path);

//! \brief Tests all functions of the FileManager system
void fileManager_test();

#endif// FIRMWARE_INCLUDE_C_HEADER_TEMPLATE_H_FILEMANAGER
