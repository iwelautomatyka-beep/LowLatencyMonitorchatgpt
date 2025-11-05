#pragma once
#include <atomic>
struct EngineParams {
    std::atomic<int> channels {1};
};
