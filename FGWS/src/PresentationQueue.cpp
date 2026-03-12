#include "../include/PresentationQueue.hpp"
#include <iostream>

using namespace framegen;

PresentationQueue::PresentationQueue() = default;

PresentationQueue::~PresentationQueue() {
    Stop();
    if (worker_ && worker_->joinable()) {
        worker_->join();
    }
}

void PresentationQueue::Start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) return;

    worker_ = std::make_unique<std::thread>([this](){
        using namespace std::chrono_literals;
        while (running_) {
            // Simple pacing tick for scaffold; real implementation will manage
            // presentation ordering and jitter-free distribution.
            std::this_thread::sleep_for(1ms);
        }
    });
}

void PresentationQueue::Stop() {
    running_.store(false);
    // jthread destructor will join automatically
}

void PresentationQueue::Submit(std::function<void()> frameProducer) {
    // For scaffold, invoke immediately. Real implementation queues and paces.
    if (frameProducer) frameProducer();
}
