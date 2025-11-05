#pragma once
#include "dsp_base.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <algorithm>

class DspRegistry {
public:
    using Factory = std::function<std::unique_ptr<DspNode>()>;
    void registerFactory(const std::string& type, Factory f) { factories_[type] = std::move(f); }
    std::unique_ptr<DspNode> create(const std::string& type) const {
        auto it = factories_.find(type);
        if (it == factories_.end()) return nullptr;
        return it->second();
    }
private:
    std::unordered_map<std::string, Factory> factories_;
};

class DspChain {
public:
    void prepare(const DspContext& ctx) {
        std::lock_guard<std::mutex> lk(mutex_);
        ctx_ = ctx;
        for (auto& n : nodes_) n->prepare(ctx_);
    }
    void clear() {
        std::lock_guard<std::mutex> lk(mutex_);
        nodes_.clear();
    }
    void add(std::unique_ptr<DspNode> n) {
        if (!n) return;
        std::lock_guard<std::mutex> lk(mutex_);
        n->prepare(ctx_);
        nodes_.push_back(std::move(n));
    }
    DspNode* at(size_t i) {
        std::lock_guard<std::mutex> lk(mutex_);
        return i < nodes_.size() ? nodes_[i].get() : nullptr;
    }
    size_t size() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return nodes_.size();
    }
    void process(float* interleaved, int32_t frames) {
        std::lock_guard<std::mutex> lk(mutex_);
        for (auto& n : nodes_) n->process(interleaved, frames);
    }
private:
    DspContext ctx_;
    std::vector<std::unique_ptr<DspNode>> nodes_;
    mutable std::mutex mutex_;
};
