#pragma once
#include <cstdint>
class HalPowerManager {
public:
    static HalPowerManager& getInstance() { static HalPowerManager inst; return inst; }
    void init() {}
    uint8_t getBatteryLevel() { return 100; }
    bool isCharging() { return false; }
    void enableCharging() {}
    void disableCharging() {}
};

#define PowerManager HalPowerManager::getInstance()