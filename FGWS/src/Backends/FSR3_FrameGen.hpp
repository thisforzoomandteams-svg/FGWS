#pragma once
#include "../include/FrameGen.hpp"
#include <memory>
#include <atomic>

namespace framegen {

class FSR3FrameGenerator final : public IFrameGenerator {
public:
     FSR3FrameGenerator() noexcept;
    ~FSR3FrameGenerator() noexcept override;

    FSR3FrameGenerator(const FSR3FrameGenerator&)            = delete;
    FSR3FrameGenerator& operator=(const FSR3FrameGenerator&) = delete;

    std::expected<void, std::string> Init(const FrameGenConfig& cfg) override;
    void GenerateFrames(FrameContext& ctx) override;
    void Shutdown() noexcept;

    Backend GetBackend()                          const override;
    bool    IsInitialized()                       const;
    bool    SupportsMultiplier(FrameMultiplier m) const override;

private:
    struct Impl;
    std::unique_ptr<Impl>    m_impl;
    std::atomic<bool>        m_initialized { false };
    FrameGenConfig           m_config      {};
};

std::unique_ptr<IFrameGenerator> CreateFSR3FrameGenerator();

} // namespace framegen
