#pragma once
#include <cstdint>
class HalTiltSensor {
public:
    static HalTiltSensor& getInstance() { static HalTiltSensor inst; return inst; }
    void init() {}
    bool isLandscape() { return false; }
    int getRotation() { return 0; }
};

#define Tilt HalTiltSensor::getInstance()