#pragma once
#include <cstdint>

struct DspContext {
    int sampleRate = 48000;
    int channels   = 1;
    int framesPerBlock = 192;
};

class DspNode {
public:
    virtual ~DspNode() = default;
    virtual const char* name() const = 0;
    virtual void prepare(const DspContext& ctx) { ctx_ = ctx; }
    virtual void process(float* interleaved, int32_t frames) = 0;
    virtual void setParam(int id, float value) {}
protected:
    DspContext ctx_;
};
