#pragma once
#include "dsp_base.h"
#include <atomic>
#include <algorithm>

enum { GAIN_PARAM_GAIN = 0 };

class GainNode : public DspNode {
public:
    const char* name() const override { return "gain"; }
    void setParam(int id, float v) override {
        if (id == GAIN_PARAM_GAIN) gain_.store(std::clamp(v, 0.f, 3.f));
    }
    void process(float* interleaved, int32_t frames) override {
        const int ch = ctx_.channels;
        const float g = gain_.load();
        for (int i = 0; i < frames * ch; ++i) interleaved[i] *= g;
    }
private:
    std::atomic<float> gain_{1.0f};
};
