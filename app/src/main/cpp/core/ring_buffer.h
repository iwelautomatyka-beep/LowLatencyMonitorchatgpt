#pragma once
#include <atomic>
#include <vector>
#include <cstring>
#include <algorithm>

class RingBuffer {
public:
    explicit RingBuffer(size_t capacitySamples)
        : mCapacity(capacitySamples), mBuffer(capacitySamples), mWriteIndex(0), mReadIndex(0) {}
    size_t write(const float* src, size_t count) {
        size_t written = 0;
        while (written < count) {
            size_t w = mWriteIndex.load(std::memory_order_relaxed);
            size_t r = mReadIndex.load(std::memory_order_acquire);
            size_t freeSpace = mCapacity - (w - r);
            if (freeSpace == 0) break;
            size_t toWrite = std::min(freeSpace, count - written);
            size_t wi = w % mCapacity;
            size_t first = std::min(toWrite, mCapacity - wi);
            memcpy(&mBuffer[wi], src + written, first * sizeof(float));
            if (toWrite > first) memcpy(&mBuffer[0], src + written + first, (toWrite - first) * sizeof(float));
            mWriteIndex.store(w + toWrite, std::memory_order_release);
            written += toWrite;
        }
        return written;
    }
    size_t read(float* dst, size_t count) {
        size_t read = 0;
        while (read < count) {
            size_t r = mReadIndex.load(std::memory_order_relaxed);
            size_t w = mWriteIndex.load(std::memory_order_acquire);
            size_t available = w - r;
            if (available == 0) break;
            size_t toRead = std::min(available, count - read);
            size_t ri = r % mCapacity;
            size_t first = std::min(toRead, mCapacity - ri);
            memcpy(dst + read, &mBuffer[ri], first * sizeof(float));
            if (toRead > first) memcpy(dst + read + first, &mBuffer[0], (toRead - first) * sizeof(float));
            mReadIndex.store(r + toRead, std::memory_order_release);
            read += toRead;
        }
        for (size_t i = read; i < count; ++i) dst[i] = 0.0f;
        return read;
    }
    void clear() {
        mWriteIndex.store(0);
        mReadIndex.store(0);
        std::fill(mBuffer.begin(), mBuffer.end(), 0.0f);
    }
private:
    const size_t mCapacity;
    std::vector<float> mBuffer;
    std::atomic<size_t> mWriteIndex;
    std::atomic<size_t> mReadIndex;
};
