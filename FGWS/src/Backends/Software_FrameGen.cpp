#include "../include/FrameGen.hpp"
#include "Software_FrameGen.hpp"
#include <memory>
#include <format>

using namespace framegen;

class SoftwareFrameGenerator : public IFrameGenerator {
public:
    SoftwareFrameGenerator() = default;
    ~SoftwareFrameGenerator() override = default;

    std::expected<void,std::string> Init(const FrameGenConfig& cfg) override {
        cfg_ = cfg;
        if (cfg_.multiplier != FrameMultiplier::X2) {
            // SOFTWARE supports X2 only in spec.
            auto msg = std::format("SOFTWARE backend supports X2 only; requested {}. Clamping.", (int)cfg_.multiplier);
            cfg_.multiplier = FrameMultiplier::X2;
            return std::unexpected(msg);
        }
        return {};
    }

    void GenerateFrames(FrameContext& ctx) override {
        // Placeholder: real optical flow + interpolation will be implemented.
    }

    void Shutdown() override {}
    Backend GetBackend() const override { return Backend::SOFTWARE; }
    bool SupportsMultiplier(FrameMultiplier m) const override {
        return m == FrameMultiplier::X2;
    }

private:
    FrameGenConfig cfg_;
};

namespace framegen {

std::unique_ptr<IFrameGenerator> CreateSoftwareFrameGenerator() {
    return std::make_unique<SoftwareFrameGenerator>();
}

} // namespace framegen
