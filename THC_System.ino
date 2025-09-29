#include "THC_System.h"

bool THC_System::enabled = true;
bool THC_System::active = false;
float THC_System::targetVoltage = THC::DEFAULT_VOLTAGE;

void THC_System::init() {
    Serial.println("✅ THC system initialized");
}

void THC_System::update() {
    // Эмуляция работы THC
    active = enabled && PlasmaControl::isActive() && PlasmaControl::isArcOK();
}

bool THC_System::isEnabled() { return enabled; }
bool THC_System::isActive() { return active; }
float THC_System::getTargetVoltage() { return targetVoltage; }