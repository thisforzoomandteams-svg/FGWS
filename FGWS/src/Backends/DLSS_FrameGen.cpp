#include "DLSS_FrameGen.hpp"
#include <format>

using namespace framegen;

// Probe for NGX headers at compile time so builds without SDKs remain clean.
#if defined(FRAMEGEN_ENABLE_DLSS) && (defined(__has_include) && (__has_include("ngx_dlss.h") || __has_include(<ngx_dlss.h>)))
    #if __has_include("ngx_dlss.h")
        #include "ngx_dlss.h"
    #else
        #include <ngx_dlss.h>
    #endif
    #define FRAMEGEN_HAS_DLSS 1
#endif

class DLSSFrameGenerator : public IFrameGenerator {
public:
    DLSSFrameGenerator() = default;
    ~DLSSFrameGenerator() override = default;

    std::expected<void,std::string> Init(const FrameGenConfig& cfg) override {
#ifdef FRAMEGEN_HAS_DLSS
        // Real NGX initialization would occur here (NGX/DLSS SDK). For scaffold, return not-implemented.
        return std::unexpected(std::format("DLSS/NGX integration not implemented in scaffold. Provide NGX SDK and implement device binding."));
#else
        return std::unexpected(std::format("DLSS SDK not available at compile time. Set FRAMEGEN_ENABLE_DLSS and provide NGX SDK via NGX_SDK_ROOT."));
#endif
    }

    void GenerateFrames(FrameContext& ctx) override {}
    void Shutdown() override {}
    Backend GetBackend() const override { return Backend::DLSS_FG; }
    bool SupportsMultiplier(FrameMultiplier m) const override { return true; }
};

namespace framegen {

std::unique_ptr<IFrameGenerator> CreateDLSSFrameGenerator() {
#ifdef FRAMEGEN_HAS_DLSS
    return std::make_unique<DLSSFrameGenerator>();
#else
    return nullptr;
#endif
}

} // namespace framegen
