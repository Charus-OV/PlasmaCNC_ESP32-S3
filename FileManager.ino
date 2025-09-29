#include "FileManager.h"
#include <SD_MMC.h>

bool FileManager::sdCardReady = false;

bool FileManager::init() {
    sdCardReady = SD_MMC.begin();
    if (sdCardReady) {
        Serial.println("✅ SD Card initialized");
        SD_MMC.mkdir("/gcode");
        return true;
    } else {
        Serial.println("❌ SD Card initialization failed");
        return false;
    }
}

std::vector<String> FileManager::listFiles() {
    std::vector<String> files;
    
    if (!sdCardReady) {
        // Эмуляция файлов для теста
        files.push_back("test1.nc");
        files.push_back("test2.gcode");
        files.push_back("circle.nc");
        return files;
    }
    
    // Реальная работа с SD картой
    // File root = SD_MMC.open("/gcode");
    // File file = root.openNextFile();
    // while (file) {
    //     if (!file.isDirectory()) {
    //         files.push_back(String(file.name()));
    //     }
    //     file = root.openNextFile();
    // }
    
    return files;
}