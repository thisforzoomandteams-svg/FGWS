#include "../include/FrameGen.hpp"
#include <memory>
#include <vector>
#include <format>
#include <string>
#include <cstdio>

using namespace framegen;

// Forward declare software factory implemented in backend stub
namespace framegen { std::unique_ptr<IFrameGenerator> CreateSoftwareFrameGenerator(); }

static Backend ResolveBackend(Backend requested, const DeviceCaps& caps, std::string& outMessage) {
    // Resolution order helpers
    auto canUse = [&](Backend b)->bool {
        switch (b) {
            case Backend::DLSS_FG: return caps.supportsNGX;
            case Backend::FSR3_FG: return caps.supportsFSR3;
            case Backend::XESS_FG: return caps.supportsXeSS;
            case Backend::SOFTWARE: return true;
        }
        return false;
    };

    Backend orderDLSS[] = { Backend::DLSS_FG, Backend::FSR3_FG, Backend::XESS_FG, Backend::SOFTWARE };
    Backend orderFSR3[] = { Backend::FSR3_FG, Backend::XESS_FG, Backend::SOFTWARE };
    Backend orderXESS[] = { Backend::XESS_FG, Backend::FSR3_FG, Backend::SOFTWARE };

    Backend const* order = nullptr;
    size_t orderCount = 0;
    switch (requested) {
        case Backend::DLSS_FG: order = orderDLSS; orderCount = std::size(orderDLSS); break;
        case Backend::FSR3_FG: order = orderFSR3; orderCount = std::size(orderFSR3); break;
        case Backend::XESS_FG: order = orderXESS; orderCount = std::size(orderXESS); break;
        case Backend::SOFTWARE: order = orderDLSS; orderCount = 1; break;
    }

    Backend chosen = Backend::SOFTWARE;
    for (size_t i=0;i<orderCount;++i) {
        if (canUse(order[i])) { chosen = order[i]; break; }
    }

    if (chosen != requested) {
        outMessage = std::format("Requested backend {} not available; selected {} as fallback", static_cast<int>(requested), static_cast<int>(chosen));
    }
    return chosen;
}

std::expected<std::unique_ptr<IFrameGenerator>, std::string>
CreateFrameGenerator(Backend requested, const DeviceCaps& caps) {
    std::string msg;
    Backend chosen = ResolveBackend(requested, caps, msg);

    if (!msg.empty()) {
        auto log = std::format("[FrameGen] {}", msg);
        // Best-effort logging to stderr for scaffold. Replace with proper logging later.
        std::fprintf(stderr, "%s\n", log.c_str());
    }

    // Attempt to instantiate chosen vendor backend if available; fall back to software.
    switch (chosen) {
        case Backend::DLSS_FG:
#if defined(FRAMEGEN_HAS_DLSS)
        {
            auto impl = CreateDLSSFrameGenerator();
            if (impl) return std::expected<std::unique_ptr<IFrameGenerator>, std::string>(std::move(impl));
        }
#endif
            return std::expected<std::unique_ptr<IFrameGenerator>, std::string>(CreateSoftwareFrameGenerator());
        case Backend::FSR3_FG:
#if defined(FRAMEGEN_HAS_FSR3)
        {
            auto impl = CreateFSR3FrameGenerator();
            if (impl) return std::expected<std::unique_ptr<IFrameGenerator>, std::string>(std::move(impl));
        }
#endif
            return std::expected<std::unique_ptr<IFrameGenerator>, std::string>(CreateSoftwareFrameGenerator());
        case Backend::XESS_FG:
#if defined(FRAMEGEN_HAS_XESS)
        {
            auto impl = CreateXeSSFrameGenerator();
            if (impl) return std::expected<std::unique_ptr<IFrameGenerator>, std::string>(std::move(impl));
        }
#endif
            return std::expected<std::unique_ptr<IFrameGenerator>, std::string>(CreateSoftwareFrameGenerator());
        case Backend::SOFTWARE:
        default:
            return std::expected<std::unique_ptr<IFrameGenerator>, std::string>(CreateSoftwareFrameGenerator());
    }
}
