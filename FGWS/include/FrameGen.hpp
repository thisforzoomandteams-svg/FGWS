#include "FSR3_FrameGen.hpp"
#include "../include/FrameGen.hpp"
#include <format>
#include <array>
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
//  FSR3_FrameGen.cpp — AMD FidelityFX FSR 3 Frame Interpolation
//
//  SDK path (already fetched in your _deps):
//    build/_deps/fidelityfx-sdk-src/Kits/FidelityFX/
//
//  Real headers:
//    framegeneration/include/dx12/ffx_framegeneration.h
//    framegeneration/fsr3/include/gpu/frameinterpolation/
//    api/include/dx12/ffx_dx12.h
// ─────────────────────────────────────────────────────────────────────────────

#if defined(FRAMEGEN_HAS_FSR3)
    #include <FidelityFX/host/ffx_frameinterpolation.h>
    #include <FidelityFX/host/backends/dx12/ffx_dx12.h>
#endif

namespace framegen {

// ── PImpl — all FSR3 SDK types live here only ─────────────────────────────────
struct FSR3FrameGenerator::Impl {
#if defined(FRAMEGEN_HAS_FSR3)
    FfxInterface                         ffxInterface    = {};
    FfxFrameInterpolationContext         fiContext       = {};
    bool                                 contextCreated  = false;
    void*                                scratchBuffer   = nullptr;
    size_t                               scratchSize     = 0;
#endif
    // Device handles (void* to keep D3D12 headers out of public API)
    void*    d3d12Device   = nullptr;  // ID3D12Device*
    void*    d3d12CmdList  = nullptr;  // ID3D12GraphicsCommandList*
    uint32_t renderW       = 0;
    uint32_t renderH       = 0;
    uint32_t displayW      = 0;
    uint32_t displayH      = 0;

    // Output textures for synthetic frames (max 3 for X4)
    std::array<void*, 3> outputTextures = { nullptr, nullptr, nullptr };
};

// ─────────────────────────────────────────────────────────────────────────────

FSR3FrameGenerator::FSR3FrameGenerator() noexcept
    : m_impl(std::make_unique<Impl>()) {}

FSR3FrameGenerator::~FSR3FrameGenerator() noexcept {
    Shutdown();
}

// ─────────────────────────────────────────────────────────────────────────────

std::expected<void, std::string> FSR3FrameGenerator::Init(const FrameGenConfig& cfg) {
    if (cfg.renderWidth == 0 || cfg.renderHeight == 0)
        return std::unexpected("FSR3: renderWidth/renderHeight cannot be zero");
    if (cfg.displayWidth == 0 || cfg.displayHeight == 0)
        return std::unexpected("FSR3: displayWidth/displayHeight cannot be zero");

    m_config = cfg;
    m_impl->renderW  = cfg.renderWidth;
    m_impl->renderH  = cfg.renderHeight;
    m_impl->displayW = cfg.displayWidth;
    m_impl->displayH = cfg.displayHeight;

#if defined(FRAMEGEN_HAS_FSR3)
    // ── Step 1: Allocate scratch memory for the FidelityFX backend ───────────
    m_impl->scratchSize   = ffxGetScratchMemorySizeDX12(FFX_FRAME_INTERPOLATION_CONTEXT_COUNT);
    m_impl->scratchBuffer = malloc(m_impl->scratchSize);
    if (!m_impl->scratchBuffer)
        return std::unexpected("FSR3: failed to allocate scratch memory");

    // ── Step 2: Create FidelityFX DX12 backend interface ─────────────────────
    FfxErrorCode err = ffxGetInterfaceDX12(
        &m_impl->ffxInterface,
        static_cast<ID3D12Device*>(m_impl->d3d12Device),
        m_impl->scratchBuffer,
        m_impl->scratchSize,
        FFX_FRAME_INTERPOLATION_CONTEXT_COUNT);

    if (err != FFX_OK)
        return std::unexpected(std::format("FSR3: ffxGetInterfaceDX12 failed ({})", static_cast<int>(err)));

    // ── Step 3: Create frame interpolation context ────────────────────────────
    FfxFrameInterpolationContextDescription desc{};
    desc.backendInterface    = m_impl->ffxInterface;
    desc.maxRenderSize       = { cfg.renderWidth,  cfg.renderHeight  };
    desc.displaySize         = { cfg.displayWidth, cfg.displayHeight };
    desc.backBufferFormat    = cfg.enableHDR
                                ? FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT
                                : FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
    desc.flags               = FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INVERTED
                             | FFX_FRAMEINTERPOLATION_ENABLE_HIGH_DYNAMIC_RANGE;

    err = ffxFrameInterpolationContextCreate(&m_impl->fiContext, &desc);
    if (err != FFX_OK)
        return std::unexpected(std::format("FSR3: ffxFrameInterpolationContextCreate failed ({})", static_cast<int>(err)));

    m_impl->contextCreated = true;

#endif // FRAMEGEN_HAS_FSR3

    m_initialized.store(true, std::memory_order_release);
    return {};
}

// ─────────────────────────────────────────────────────────────────────────────

void FSR3FrameGenerator::GenerateFrames(FrameContext& ctx) {
    if (!m_initialized.load(std::memory_order_acquire)) return;

    const uint32_t syntheticCount = static_cast<uint32_t>(m_config.multiplier) - 1u;

    // Interpolation time values per multiplier
    // X2→{0.5}, X3→{0.333,0.667}, X4→{0.25,0.5,0.75}
    constexpr float kTimes[3][3] = {
        { 0.500f, 0.000f, 0.000f },
        { 0.333f, 0.667f, 0.000f },
        { 0.250f, 0.500f, 0.750f }
    };
    const float* times = kTimes[syntheticCount - 1];

#if defined(FRAMEGEN_HAS_FSR3)
    auto* cmdList = static_cast<ID3D12GraphicsCommandList*>(m_impl->d3d12CmdList);

    for (uint32_t i = 0; i < syntheticCount; ++i) {
        FfxFrameInterpolationDispatchDescription dispatchDesc{};

        dispatchDesc.commandList = ffxGetCommandListDX12(cmdList);

        // Input resources
        dispatchDesc.displaySize         = { m_impl->displayW, m_impl->displayH };
        dispatchDesc.renderSize          = { m_impl->renderW,  m_impl->renderH  };

        dispatchDesc.currentBackBuffer   = ffxGetResourceDX12(
            ctx.colorCurrent,
            ffxGetResourceDescriptionDX12(ctx.colorCurrent),
            nullptr,
            FFX_RESOURCE_STATE_COMPUTE_READ);

        dispatchDesc.currentBackBuffer_HUDLess = ctx.hudStencil
            ? ffxGetResourceDX12(ctx.hudStencil,
                ffxGetResourceDescriptionDX12(ctx.hudStencil),
                nullptr, FFX_RESOURCE_STATE_COMPUTE_READ)
            : dispatchDesc.currentBackBuffer;

        dispatchDesc.output = ffxGetResourceDX12(
            static_cast<ID3D12Resource*>(m_impl->outputTextures[i]),
            ffxGetResourceDescriptionDX12(
                static_cast<ID3D12Resource*>(m_impl->outputTextures[i])),
            nullptr,
            FFX_RESOURCE_STATE_UNORDERED_ACCESS);

        dispatchDesc.opticalFlowVector = ffxGetResourceDX12(
            ctx.motionVectors,
            ffxGetResourceDescriptionDX12(ctx.motionVectors),
            nullptr,
            FFX_RESOURCE_STATE_COMPUTE_READ);

        dispatchDesc.depth = ffxGetResourceDX12(
            ctx.depthBuffer,
            ffxGetResourceDescriptionDX12(ctx.depthBuffer),
            nullptr,
            FFX_RESOURCE_STATE_COMPUTE_READ);

        dispatchDesc.interpolationRect   = { 0, 0, m_impl->renderW, m_impl->renderH };
        dispatchDesc.frameTimeDelta      = ctx.deltaTimeMs;
        dispatchDesc.cameraNear          = 0.1f;
        dispatchDesc.cameraFar           = 10000.0f;
        dispatchDesc.viewSpaceToMetersFactor = 1.0f;
        dispatchDesc.reset               = (ctx.frameIndex == 0);

        // Confidence decay weight for X3/X4 frames
        const float t = times[i];
        const float d = (t < 0.5f) ? t : (1.0f - t);
        dispatchDesc.frameTimeDelta *= std::exp(-m_config.confidenceLambda * (0.5f - d) * 2.0f);

        ffxFrameInterpolationDispatch(&m_impl->fiContext, &dispatchDesc);
    }

#else
    // Scaffold: no-op when FSR3 SDK not present
    (void)ctx; (void)times; (void)syntheticCount;
#endif
}

// ─────────────────────────────────────────────────────────────────────────────

void FSR3FrameGenerator::Shutdown() noexcept {
    if (!m_initialized.exchange(false, std::memory_order_acq_rel)) return;

#if defined(FRAMEGEN_HAS_FSR3)
    if (m_impl->contextCreated) {
        ffxFrameInterpolationContextDestroy(&m_impl->fiContext);
        m_impl->contextCreated = false;
    }
    if (m_impl->scratchBuffer) {
        free(m_impl->scratchBuffer);
        m_impl->scratchBuffer = nullptr;
    }
#endif
}

// ─────────────────────────────────────────────────────────────────────────────

Backend         FSR3FrameGenerator::GetBackend()    const { return Backend::FSR3_FG; }
bool            FSR3FrameGenerator::IsInitialized() const { return m_initialized.load(std::memory_order_acquire); }
bool            FSR3FrameGenerator::SupportsMultiplier(FrameMultiplier) const { return true; }

// ─────────────────────────────────────────────────────────────────────────────
//  Factory function (matches FSR3_FrameGen.hpp declaration)
// ─────────────────────────────────────────────────────────────────────────────

std::unique_ptr<IFrameGenerator> CreateFSR3FrameGenerator() {
    return std::make_unique<FSR3FrameGenerator>();
}

} // namespace framegen
