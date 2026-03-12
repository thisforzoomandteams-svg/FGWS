#include "FSR3_FrameGen.hpp"
#include <format>

using namespace framegen;

// Detect available header at compile time; do not require SDK for scaffold builds.
#if defined(FRAMEGEN_ENABLE_FSR3) && (defined(__has_include) && (__has_include("ffx_fsr3_api.h") || __has_include(<ffx_fsr3_api.h>) || __has_include("ffx_fsr3.h") || __has_include(<ffx_fsr3.h>)))
    #if __has_include("ffx_fsr3_api.h")
        #include "ffx_fsr3_api.h"
    #elif __has_include(<ffx_fsr3_api.h>)
        #include <ffx_fsr3_api.h>
    #elif __has_include("ffx_fsr3.h")
        #include "ffx_fsr3.h"
    #elif __has_include(<ffx_fsr3.h>)
        #include <ffx_fsr3.h>
    #endif
    #define FRAMEGEN_HAS_FSR3 1
#endif

class FSR3FrameGenerator : public IFrameGenerator {
public:
    FSR3FrameGenerator() = default;
    ~FSR3FrameGenerator() override = default;

    std::expected<void,std::string> Init(const FrameGenConfig& cfg) override {
#ifdef FRAMEGEN_HAS_FSR3
        // Example: acquire DX12/VK interface and create interpolation context.
        // Real usage depends on the platform and available API — this is a best-effort wiring.
        // The SDK provides ffxGetInterfaceDX12 / ffxGetInterfaceVK and ffxFrameInterpolationContextCreate.
        // We assume a device/context will be provided later via dedicated init paths.
        bool created = false;
        // TODO: Implement actual DX12/VK device binding when integrating with runtime device.
        if (!created) {
            return std::unexpected(std::format("FSR3 enabled but runtime device binding not implemented in scaffold"));
        }
#else
        return std::unexpected(std::format("FSR3 SDK not available at compile time. Set FRAMEGEN_ENABLE_FSR3 and provide FidelityFX SDK."));
#endif
    }

    void GenerateFrames(FrameContext& ctx) override {
        // Implementation will call FSR3 interpolation dispatches.
    }

    void Shutdown() override {}
    Backend GetBackend() const override { return Backend::FSR3_FG; }
    bool SupportsMultiplier(FrameMultiplier m) const override { return true; }
};

namespace framegen {

std::unique_ptr<IFrameGenerator> CreateFSR3FrameGenerator() {
#ifdef FRAMEGEN_HAS_FSR3
    return std::make_unique<FSR3FrameGenerator>();
#else
    return nullptr;
#endif
}

} // namespace framegen
