#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <span>
#include <format>
#include <expected>
#include <concepts>
#include <atomic>
#include "DeviceCaps.hpp"

namespace framegen {

enum class Backend : uint8_t {
    DLSS_FG  = 0,
    FSR3_FG  = 1,
    XESS_FG  = 2,
    SOFTWARE = 3
};

enum class FrameMultiplier : uint8_t {
    X2 = 2,
    X3 = 3,
    X4 = 4
};

// Minimal glm::mat4 placeholder to avoid heavy dependencies in scaffold.
namespace glm { struct mat4 { float m[16] = {}; }; }

// Forward-declare D3D resource type to avoid including D3D headers in public API.
struct ID3D12Resource;

struct FrameGenConfig {
    Backend          backend          = Backend::FSR3_FG;
    FrameMultiplier  multiplier       = FrameMultiplier::X2;
    uint32_t         renderWidth      = 0;
    uint32_t         renderHeight     = 0;
    uint32_t         displayWidth     = 0;
    uint32_t         displayHeight    = 0;
    float            sharpness        = 0.5f;
    bool             enableHDR        = false;
    bool             enableOcclusion  = true;
    bool             enableHUDMask    = true;
    float            confidenceLambda = 1.4f;
};

struct FrameContext {
    ID3D12Resource*  colorCurrent     = nullptr;
    ID3D12Resource*  colorPrevious    = nullptr;
    ID3D12Resource*  motionVectors    = nullptr;
    ID3D12Resource*  depthBuffer      = nullptr;
    ID3D12Resource*  hudStencil       = nullptr;
    float            deltaTimeMs      = 0.0f;
    glm::mat4        viewProjCurr     = {};
    glm::mat4        viewProjPrev     = {};
    uint64_t         frameIndex       = 0;
};

class IFrameGenerator {
public:
    virtual ~IFrameGenerator() = default;
    virtual std::expected<void,std::string> Init(const FrameGenConfig& cfg) = 0;
    virtual void GenerateFrames(FrameContext& ctx) = 0;
    virtual void Shutdown() = 0;
    virtual Backend GetBackend() const = 0;
    virtual bool SupportsMultiplier(FrameMultiplier m) const = 0;
};

template<typename T>
concept FrameGeneratorImpl = requires(T t, FrameContext& ctx) {
    { t.GenerateFrames(ctx) } -> std::same_as<void>;
    { t.GetBackend() }        -> std::same_as<Backend>;
};

// Factory
std::expected<std::unique_ptr<IFrameGenerator>, std::string>
CreateFrameGenerator(Backend requested, const DeviceCaps& caps);

} // namespace framegen
