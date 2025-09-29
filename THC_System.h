#ifndef THC_SYSTEM_H
#define THC_SYSTEM_H

#include <Arduino.h>
#include "Config.h"

class THC_System {
public:
    static void init();
    static void update();
    
    static bool isEnabled();
    static bool isActive();
    static float getTargetVoltage();
    
private:
    static bool enabled;
    static bool active;
    static float targetVoltage;
};

#endif