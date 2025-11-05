#include "audio_engine.h"
#include <thread>
#include <chrono>

AudioEngineImpl::AudioEngineImpl() {
    registry.registerFactory("gain", []{ return std::make_unique<GainNode>(); });
    registry.registerFactory("delay", []{ return std::make_unique<DelayNode>(); });
}

void AudioEngineImpl::restartAsync() {
    std::thread([this]{
        stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        start();
    }).detach();
}

bool AudioEngineImpl::start() {
    std::lock_guard<std::mutex> lk(startStopMutex);
    if (state.load() != EngineState::Stopped) return true;
    state.store(EngineState::Starting);

    inputCb = std::make_unique<InputCallback>(ring);

    inErrCb  = std::make_unique<StreamErrorCallback>([this]{ restartAsync(); });
    outErrCb = std::make_unique<StreamErrorCallback>([this]{ restartAsync(); });

    oboe::AudioStreamBuilder inB;
    inB.setDirection(oboe::Direction::Input);
    inB.setPerformanceMode(oboe::PerformanceMode::LowLatency);
    inB.setSharingMode(oboe::SharingMode::Exclusive);
    inB.setChannelCount(params.channels.load());
    inB.setFormat(oboe::AudioFormat::Float);
    inB.setInputPreset(oboe::InputPreset::VoiceRecognition);
    inB.setCallback(inputCb.get());
    inB.setErrorCallback(inErrCb.get());

    oboe::Result r = inB.openManagedStream(inputStream);
    if (r != oboe::Result::OK) { LOGE("Input open failed: %s", oboe::convertToText(r)); state.store(EngineState::Stopped); return false; }

    sampleRate = inputStream->getSampleRate();

    dctx.sampleRate = sampleRate;
    dctx.channels = params.channels.load();
    dctx.framesPerBlock = inputStream->getFramesPerBurst();

    if (chain.size() == 0) {
        chain.prepare(dctx);
        chain.add(registry.create("gain"));
    } else {
        chain.prepare(dctx);
    }

    outputCb = std::make_unique<OutputCallback>(ring, chain, dctx.channels);

    oboe::AudioStreamBuilder outB;
    outB.setDirection(oboe::Direction::Output);
    outB.setPerformanceMode(oboe::PerformanceMode::LowLatency);
    outB.setSharingMode(oboe::SharingMode::Exclusive);
    outB.setChannelCount(dctx.channels);
    outB.setFormat(oboe::AudioFormat::Float);
    outB.setSampleRate(sampleRate);
    outB.setUsage(oboe::Usage::Media);
    outB.setContentType(oboe::ContentType::Speech);
    outB.setCallback(outputCb.get());
    outB.setErrorCallback(outErrCb.get());

    r = outB.openManagedStream(outputStream);
    if (r != oboe::Result::OK) { LOGE("Output open failed: %s", oboe::convertToText(r)); inputStream->close(); state.store(EngineState::Stopped); return false; }

    (void)inputStream->swapErrorCallback(inErrCb.get());
    (void)outputStream->swapErrorCallback(outErrCb.get());

    r = inputStream->requestStart();
    if (r != oboe::Result::OK) { LOGE("Input start failed: %s", oboe::convertToText(r)); stop(); return false; }
    r = outputStream->requestStart();
    if (r != oboe::Result::OK) { LOGE("Output start failed: %s", oboe::convertToText(r)); stop(); return false; }

    state.store(EngineState::Running);
    LOGI("Started SR=%d, ch=%d, FPB=%d", sampleRate, dctx.channels, dctx.framesPerBlock);
    return true;
}

void AudioEngineImpl::stop() {
    std::lock_guard<std::mutex> lk(startStopMutex);
    if (state.load() == EngineState::Stopped || state.load() == EngineState::Stopping) return;
    state.store(EngineState::Stopping);

    if (outputStream) (void)outputStream->requestStop();
    if (inputStream)  (void)inputStream->requestStop();
    if (outputStream) outputStream->close();
    if (inputStream)  inputStream->close();

    inputCb.reset();
    outputCb.reset();
    inErrCb.reset();
    outErrCb.reset();
    ring.clear();

    state.store(EngineState::Stopped);
    LOGI("Stopped");
}

void AudioEngineImpl::clearChain() {
    chain.clear();
    chain.prepare(dctx);
}

int AudioEngineImpl::addNode(const char* type) {
    auto n = registry.create(type ? type : "");
    if (!n) return -1;
    int idx = (int)chain.size();
    chain.add(std::move(n));
    return idx;
}

void AudioEngineImpl::setParam(int index, int paramId, float value) {
    auto* n = chain.at((size_t)index);
    if (n) n->setParam(paramId, value);
}

// C linkage for JNI
static AudioEngineImpl g;
extern "C" {
    void engine_start() { (void)g.start(); }
    void engine_stop()  { g.stop(); }
    void engine_clear_chain() { g.clearChain(); }
    int  engine_add_node(const char* type) { return g.addNode(type); }
    void engine_set_param(int index, int paramId, float value) { g.setParam(index, paramId, value); }
}
