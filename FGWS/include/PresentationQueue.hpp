#pragma once

#include <thread>
#include <atomic>
#include <chrono>
#include <memory>
#include <functional>

namespace framegen {

class PresentationQueue {
public:
    PresentationQueue();
    ~PresentationQueue();

    // Start the presentation worker. Worker will join on destruction.
    void Start();
    void Stop();

    // Submit a frame sequence callback to be invoked in presentation order.
    void Submit(std::function<void()> frameProducer);

private:
    std::unique_ptr<std::thread> worker_;
    std::atomic<bool> running_ { false };
    std::atomic<uint64_t> frameIndex_{0};
};

} // namespace framegen
