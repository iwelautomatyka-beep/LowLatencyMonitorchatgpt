#pragma once
#include "dsp_base.h"
#include <vector>
#include <atomic>
#include <algorithm>

enum { DELAY_PARAM_TIME_MS = 0, DELAY_PARAM_FEEDBACK = 1, DELAY_PARAM_MIX = 2 };

class DelayNode : public DspNode {
public:
    const char* name() const override { return "delay"; }

    void prepare(const DspContext& ctx) override {
        DspNode::prepare(ctx);
        setDelayMs(delayMs_.load());
        write_ = 0;
        std::fill(buf_.begin(), buf_.end(), 0.0f);
    }

    void setParam(int id, float v) override {
        if (id == DELAY_PARAM_TIME_MS) setDelayMs(std::clamp(v, 1.f, 2000.f));
        else if (id == DELAY_PARAM_FEEDBACK) feedback_.store(std::clamp(v, 0.f, 0.98f));
        else if (id == DELAY_PARAM_MIX) mix_.store(std::clamp(v, 0.f, 1.f));
    }

    void process(float* interleaved, int32_t frames) override {
        const int ch = ctx_.channels;
        const float fb = feedback_.load();
        const float mix = mix_.load();
        for (int f = 0; f < frames; ++f) {
            for (int c = 0; c < ch; ++c) {
                const int idx = f*ch + c;
                float in = interleaved[idx];
                float d = buf_[write_*ch + c];
                buf_[write_*ch + c] = in + d * fb;
                interleaved[idx] = in*(1.f - mix) + d*mix;
            }
            write_ = (write_ + 1) % delayFrames_;
        }
    }

private:
    void setDelayMs(float ms) {
        delayMs_.store(ms);
        delayFrames_ = std::max(1, int(ms * ctx_.sampleRate / 1000.f));
        buf_.assign(delayFrames_ * ctx_.channels, 0.0f);
    }
    std::vector<float> buf_;
    int delayFrames_ = 1;
    int write_ = 0;
    std::atomic<float> delayMs_{250.f};
    std::atomic<float> feedback_{0.4f};
    std::atomic<float> mix_{0.25f};
};
