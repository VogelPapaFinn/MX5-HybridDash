/* --- Includes --- */
#include "FileManager/FileManager.h"

/* --- Private Defines & Macros --- */

/* --- Private Variables, Typedefs etc. --- */

sdmmc_card_t *sdmmcCard_ = NULL;
sdmmc_host_t sdmmcHost_ = SDMMC_HOST_DEFAULT();
sdmmc_slot_config_t slotConfig_ = SDMMC_SLOT_CONFIG_DEFAULT();
esp_vfs_fat_sdmmc_mount_config_t mountConfig_;

esp_vfs_spiffs_conf_t spiffsConfig_;

// Is the SD Card mounted?
bool sdCardMounted_ = false;
// Is the internal spiffs partition mounted?
bool spiffsMounted_ = false;

//! \brief Builds the whole path depending on the location
//! \param path The path that should be checked
//! \param location The location e.g. SD Card or Internal
//! \retval char* which contains the full path. Check for NULL!
char *buildFullPath(const char *path, const int location) {
    // Contains the full path to the file
    char *fullPath = malloc(strlen("/sdcard/") + strlen(path) + 1);
    if (fullPath == NULL) return false;

    // Check the location
    if (location == LOCATION_INTERNAL) {
        // Is the spiffs partition mounted?
        if (!spiffsMounted_) {
            // Logging
            loggerError("Spiffs partition not mounted");

            // No so stop here!
            free(fullPath);
            return false;
        }

        // Yes, so build the full path
        sprintf(fullPath, "%s%s", "/spiffs/", path);
    } else if (location == LOCATION_SDCARD) {
        // Is the SD Card mounted?
        if (!sdCardMounted_) {
            // Logging
            loggerError("SD Card not mounted");

            // No so stop here!
            free(fullPath);
            return false;
        }

        // Yes, so build the full path
        sprintf(fullPath, "%s%s", "/sdcard/", path);
    }

    return fullPath;
}

/* --- Function implementations --- */
bool fileManagerInit(void) {
    /* Initializing the micro SDCard slot */

    // Initialize the SDMMC host
    sdmmcHost_.max_freq_khz = SDMMC_FREQ_HIGHSPEED;// 40 MHz
    sdmmcHost_.slot = SDMMC_HOST_SLOT_0;

    // Initialize the slot
    slotConfig_.width = 4;
    slotConfig_.clk = FILEMANAGER_GPIO_CLK;
    slotConfig_.cmd = FILEMANAGER_GPIO_CMD;
    slotConfig_.d0 = FILEMANAGER_GPIO_D0;
    slotConfig_.d1 = FILEMANAGER_GPIO_D1;
    slotConfig_.d2 = FILEMANAGER_GPIO_D2;
    slotConfig_.d3 = FILEMANAGER_GPIO_D3;

    // Initialize the mount config
    mountConfig_.format_if_mount_failed = false;
    mountConfig_.max_files = 5;// Max. amount of simultaneously opened files

    // Try to mount the SDCard
    const esp_err_t mountCardResult = esp_vfs_fat_sdmmc_mount("/sdcard", &sdmmcHost_, &slotConfig_, &mountConfig_, &sdmmcCard_);

    // Was the mount successful?
    if (mountCardResult == ESP_OK) {
        // The SDCard was mounted successfully
        sdCardMounted_ = true;

        // Logging
        loggerInfo("Mounted SD card successfully");
    } else {
        // Logging
        loggerError("Mounting failed with error");
    }

    /* Initialize spiffs partition */

    // Initialize spiffs config
    esp_vfs_spiffs_conf_t spiffsConfig;
    spiffsConfig.base_path = "/spiffs";
    spiffsConfig.partition_label = NULL;
    spiffsConfig.max_files = 5;
    spiffsConfig.format_if_mount_failed = true;

    // Register spiffs partition
    const esp_err_t mountSpiffsResult = esp_vfs_spiffs_register(&spiffsConfig);

    // Was the mount successful?
    if (mountSpiffsResult == ESP_OK) {
        // The spiffs partition was mounted successfully
        spiffsMounted_ = true;

        // Logging
        loggerInfo("Mounted spiffs partition successfully");

        //esp_spiffs_format(NULL);
    } else {
        // Logging
        loggerError("Mounting spiffs partition failed");
    }

    // Return success
    return spiffsMounted_
            || sdCardMounted_;
}

bool fileManagerCreateFile(const char *path, const int location) {
    // Check if the location is mounted
    if (location == LOCATION_INTERNAL && !spiffsMounted_) {
        return false;
    } else if (location == LOCATION_SDCARD && !sdCardMounted_) {
        return false;
    }

    // Contains the full path to the file
    char *fullPath = buildFullPath(path, location);

    // For whatever reason fopen crashes with test.txt. I have absolutely no clue why but it costed me quite a few
    // hours until I found the crashes are caused by this :C
    if (strcmp(path, "test.txt") == 0) return false;

    // If the file does not yet exist
    if (!fileManagerDoesFileExists(path, location)) {
        // Try to create a file
        FILE *file = fopen(fullPath, "a+");

        // Was it successful?
        if (file == NULL) {
            // Logging
            loggerError("Failed creating file %s", fullPath);

            // Free the fullPath
            free(fullPath);

            // Then return NULL
            return false;
        }

        // Yes it was, now close it
        fclose(file);
    }

    // Yes, free the path and return true
    free(fullPath);
    return true;
}

bool fileManagerDoesFileExists(const char *path, const int location) {
    // Check if the location is mounted
    if (location == LOCATION_INTERNAL && !spiffsMounted_) {
        return false;
    } else if (location == LOCATION_SDCARD && !sdCardMounted_) {
        return false;
    }

    // Contains the full path to the file
    char *fullPath = buildFullPath(path, location);

    // Try to access that path
    const bool result = access(fullPath, F_OK);// 0 -> success ; 1 -> error

    // Free the full path
    free(fullPath);

    return !result;
}

FILE *fileManagerOpenFile(const char *path, const char *mode, const int location) {
    // Check if the location is mounted
    if (location == LOCATION_INTERNAL && !spiffsMounted_) {
        return NULL;
    } else if (location == LOCATION_SDCARD && !sdCardMounted_) {
        return NULL;
    }

    // Contains the full path to the file
    char *fullPath = buildFullPath(path, location);

    // Try to create a file
    FILE *file = fopen(fullPath, mode);

    // Was it successful?
    if (file == NULL) {
        // Logging
        loggerError("Failed to open file. Path: %s ; Mode: %s", fullPath, mode);

        // Free the fullPath
        free(fullPath);

        // Then return NULL
        return NULL;
    }

    // Yes, free the path and return the file
    free(fullPath);
    return file;
}

bool fileManagerDeleteFile(const char *path, const int location) {
    // Check if the location is mounted
    if (location == LOCATION_INTERNAL && !spiffsMounted_) {
        return false;
    } else if (location == LOCATION_SDCARD && !sdCardMounted_) {
        return false;
    }

    // Contains the full path to the file
    char *fullPath = buildFullPath(path, location);

    // Try to delete the file
    const bool result = !remove(fullPath);// 0 -> success ;  1 -> error
    if (result) {
        // Logging
        loggerInfo("Deleted file: %s", fullPath);
    } else {
        // Logging
        loggerWarn("Failed deleting file: %s", fullPath);
    }

    // Free the path
    free(fullPath);

    return result;
}

bool fileManagerDoesDirectoryExist(const char *dir) {
    // Check if the sdcard is mounted
    if (!sdCardMounted_) {
        return false;
    }

    // Contains the full path to the file
    char *fullPath = buildFullPath(dir, LOCATION_SDCARD);

    // Does the path exist?
    struct stat stats;
    stat(fullPath, &stats);
    const bool result = S_ISDIR(stats.st_mode);

    // Free fullPath
    free(fullPath);

    // Return result
    return result;
}

bool fileManagerCreateDir(const char *path) {
    // Check if the sdcard is mounted
    if (!sdCardMounted_) {
        return false;
    }

    // Contains the full path to the file
    char *fullPath = buildFullPath(path, LOCATION_SDCARD);

    // Create the path
    const bool result = mkdir(fullPath, S_IRWXU) == 0 ? true : false;// 0 -> success ; -1 -> error

    // Did it fail?
    if (!result) {
        // Logging
        loggerWarn("Failed to create directory: %s", fullPath);
    }

    // Free fullPath
    free(fullPath);

    return result;
}

bool fileManagerDeleteDir(const char *path) {
    // Check if the sdcard is mounted
    if (!sdCardMounted_) {
        return false;
    }

    // Contains the full path to the file
    char *fullPath = buildFullPath(path, LOCATION_SDCARD);

    // Create the path
    const bool result = rmdir(fullPath) == 0 ? true : false;// 0 -> success ; -1 -> error

    // Did it fail?
    if (!result) {
        // Logging
        loggerWarn("Failed to delete directory: %s", fullPath);
    }

    // Free fullPath
    free(fullPath);

    return result;
}

void fileManager_test() {
    // Slots for two files
    FILE *file1 = NULL;
    FILE *file2 = NULL;

    // Create two files, one on the internal partition ...
    if (fileManagerCreateFile("hello.txt", LOCATION_INTERNAL)) {
        file1 = fileManagerOpenFile("hello.txt", "a+", LOCATION_INTERNAL);
    }
    // ... and one on the SD Card
    if (fileManagerCreateFile("hello.txt", LOCATION_SDCARD)) {
        file2 = fileManagerOpenFile("hello.txt", "a+", LOCATION_SDCARD);
    }

    /* TESTING */

    // Close the files
    fclose(file1);
    fclose(file2);

    // Delete the files
    fileManagerDeleteFile("hello.txt", LOCATION_INTERNAL);
    fileManagerDeleteFile("hello.txt", LOCATION_SDCARD);

    // Create a directory "location" on the SD Card
    if (fileManagerCreateDir("location"))
        loggerInfo("The location on the SD Card was created successfully!");

    // Check if these locations exists on the SD Card
    if (fileManagerDoesDirectoryExist("location"))
        loggerInfo("The location exists on the SD Card!");
    else
        loggerWarn("The location does not exist on the SD Card!");

    // Repoint both slots to NULL
    file1 = NULL;
    file2 = NULL;

    // Create new file in the newly created directory
    if (fileManagerCreateFile("location/hello.txt", LOCATION_SDCARD)) {
        file1 = fileManagerOpenFile("hello.txt", "a+", LOCATION_SDCARD);
    }

    // Close the file again
    fclose(file1);

    // Then delete it
    fileManagerDeleteFile("location/hello.txt", LOCATION_SDCARD);

    // Then delete the directory
    if (fileManagerDeleteDir("location"))
        loggerInfo("Successfully deleted directory on SD Card!");
    else
        loggerWarn("Failed to delete the directory on the SD Card");
}