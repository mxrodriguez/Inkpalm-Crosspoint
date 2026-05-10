#pragma once
#include <cstdint>

struct ESPType {
    uint32_t getFreeHeap() const { return 1024 * 1024; }
};

inline ESPType ESP;
