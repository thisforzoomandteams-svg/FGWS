#include "XeSS_FrameGen.hpp"
#include <format>

using namespace framegen;

// Probe for XeSS headers at compile time so builds without SDKs remain clean.
#if defined(FRAMEGEN_ENABLE_XESS) && (defined(__has_include) && (__has_include("xess.h") || __has_include(<xess.h>)))
    #if __has_include("xess.h")
        #include "xess.h"
    #else
        #include <xess.h>
    #endif
    #define FRAMEGEN_HAS_XESS 1
#endif

class XeSSFrameGenerator : public IFrameGenerator {
public:
    XeSSFrameGenerator() = default;
    ~XeSSFrameGenerator() override = default;

    std::expected<void,std::string> Init(const FrameGenConfig& cfg) override {
#ifdef FRAMEGEN_HAS_XESS
        // Real XeSS initialization would occur here (XeSS SDK + DirectML). For scaffold, return not-implemented.
        return std::unexpected(std::format("XeSS integration not implemented in scaffold. Provide XeSS SDK and implement device binding."));
#else
        return std::unexpected(std::format("XeSS SDK not available at compile time. Set FRAMEGEN_ENABLE_XESS and provide XESS_SDK_ROOT."));
#endif
    }

    void GenerateFrames(FrameContext& ctx) override {}
    void Shutdown() override {}
    Backend GetBackend() const override { return Backend::XESS_FG; }
    bool SupportsMultiplier(FrameMultiplier m) const override { return true; }
};

namespace framegen {

std::unique_ptr<IFrameGenerator> CreateXeSSFrameGenerator() {
#ifdef FRAMEGEN_HAS_XESS
    return std::make_unique<XeSSFrameGenerator>();
#else
    return nullptr;
#endif
}

} // namespace framegen
