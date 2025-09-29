#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <Arduino.h>
#include <vector>

class FileManager {
public:
    static bool init();
    static std::vector<String> listFiles();
    
private:
    static bool sdCardReady;
};

#endif