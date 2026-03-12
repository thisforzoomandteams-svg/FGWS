#include "../include/SoftwareInterpolationPipeline.hpp"
#include "../include/MotionVectorPass.hpp"
#include "../include/OcclusionMaskPass.hpp"
#include "../include/InterpolationPass.hpp"
#include <cmath>
#include <algorithm>

#if defined(_WIN32)
#include <d3d12.h>
#endif

using namespace framegen;

InterpolationFrameTimes GetInterpolationTimes(FrameMultiplier m) {
    InterpolationFrameTimes result;
    switch (m) {
        case FrameMultiplier::X2:
            result.times = { 0.5f };
            break;
        case FrameMultiplier::X3:
            result.times = { 0.333f, 0.667f };
            break;
        case FrameMultiplier::X4:
            result.times = { 0.25f, 0.5f, 0.75f };
            break;
    }
    return result;
}

SoftwareInterpolationPipeline::SoftwareInterpolationPipeline(void* device, uint32_t renderWidth, uint32_t renderHeight)
    : device_(device), width_(renderWidth), height_(renderHeight) {
    // Initialize compute pass instances with placeholder PSOs/root sigs.
    // Real initialization would bind actual HLSL compute PSOs and root signatures.
    // For scaffold, create minimal stubs.
    auto deviceD3D = static_cast<ID3D12Device*>(device);
    occlusionPass_ = std::make_unique<OcclusionMaskPass>(deviceD3D, nullptr, nullptr);
    motionVectorPass_ = std::make_unique<MotionVectorPass>(deviceD3D, nullptr, nullptr);
    interpolationPass_ = std::make_unique<InterpolationPass>(deviceD3D, nullptr, nullptr);
}

SoftwareInterpolationPipeline::~SoftwareInterpolationPipeline() {
    Shutdown();
}

inline void InsertUAVBarrier(void* cmdListPtr) {
#if defined(_WIN32) && defined(FRAMEGEN_D3D12)
    auto cmdList = static_cast<ID3D12GraphicsCommandList*>(cmdListPtr);
    if (!cmdList) return;
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.UAV.pResource = nullptr;
    cmdList->ResourceBarrier(1, &barrier);
#endif
}

std::vector<void*> SoftwareInterpolationPipeline::Execute(
    void* cmdListPtr,
    const FrameContext& ctx,
    FrameMultiplier multiplier,
    void* motionVectorsOverride
) {
    std::vector<void*> syntheticFrames;

#if defined(_WIN32) && defined(FRAMEGEN_D3D12)
    auto cmdList = static_cast<ID3D12GraphicsCommandList*>(cmdListPtr);
    auto motionVectorsOverrideD3D = static_cast<ID3D12Resource*>(motionVectorsOverride);

    if (!cmdList) return syntheticFrames;

    // Step 1: Occlusion mask pass
    // Compute depth discontinuities via Sobel to generate confidence map
    OcclusionCB occluCB{ 10, {0.f, 0.f, 0.f} };  // Example threshold
    if (occlusionPass_) {
        occlusionPass_->Dispatch(cmdList, occluCB, static_cast<ID3D12Resource*>(ctx.depthBuffer), 
                                 nullptr,  // TODO: bind confidence output resource
                                 width_, height_);
    }
    InsertUAVBarrier(cmdListPtr);

    // Step 2: Motion vectors pass (if engine didn't provide them)
    if (!motionVectorsOverrideD3D) {
        CameraCB camCB;
        memcpy(camCB.viewProjCurr, &ctx.viewProjCurr, sizeof(glm::mat4));
        memcpy(camCB.viewProjPrev, &ctx.viewProjPrev, sizeof(glm::mat4));
        if (motionVectorPass_) {
            motionVectorPass_->Dispatch(cmdList, camCB, ctx.depthBuffer, 
                                        ctx.motionVectors,  // TODO: ensure non-null
                                        width_, height_);
        }
    }
    InsertUAVBarrier(cmdListPtr);

    // Step 3: Interpolation passes (one per synthetic frame)
    auto interpTimes = GetInterpolationTimes(multiplier);
    for (size_t i = 0; i < interpTimes.times.size(); ++i) {
        InterpolationCB interpCB{
            interpTimes.times[i],
            10,          // occlusion threshold
            1.4f,        // confidence decay lambda per spec
            0.f
        };
        // TODO: Create synthetic frame resources and bind to interpolation pass
        // For now, just dispatch with placeholder resources.
        if (interpolationPass_) {
            interpolationPass_->Dispatch(
                cmdList,
                interpCB,
                ctx.colorCurrent, ctx.colorPrevious,
                ctx.motionVectors, nullptr,          // confidence map
                nullptr,                              // TODO: output synthetic frame
                width_, height_
            );
        }
        InsertUAVBarrier(cmdListPtr);

        // TODO: Push generated synthetic frame to syntheticFrames vector
    }

#endif

    return syntheticFrames;
}

void SoftwareInterpolationPipeline::Shutdown() {
    occlusionPass_.reset();
    motionVectorPass_.reset();
    interpolationPass_.reset();
    syntheticsCache_.clear();
}
