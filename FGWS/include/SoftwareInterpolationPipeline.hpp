#pragma once

#include "FrameGen.hpp"
#include <memory>
#include <vector>

namespace framegen {

class MotionVectorPass;
class OcclusionMaskPass;
class InterpolationPass;

// Pre-computed interpolation times per multiplier
static constexpr float InterpolationTimes(FrameMultiplier m) {
    switch (m) {
        case FrameMultiplier::X2: return 0.5f;
        case FrameMultiplier::X3: return 0.333f;  // Note: first of two frames; second at 0.667f
        case FrameMultiplier::X4: return 0.25f;   // Note: first of three frames
        default: return 0.5f;
    }
}

struct InterpolationFrameTimes {
    std::vector<float> times;
};

// Get all interpolation times for a multiplier
InterpolationFrameTimes GetInterpolationTimes(FrameMultiplier m);

class SoftwareInterpolationPipeline {
public:
    SoftwareInterpolationPipeline(void* device, uint32_t renderWidth, uint32_t renderHeight);
    ~SoftwareInterpolationPipeline();

    // Execute the full interpolation pipeline:
    // 1. OcclusionMask: Sobel on depth → confidence map
    // 2. MotionVectors (if no engine MV provided): reproject depth → screen-space MV
    // 3. Interpolation × N: for each t in InterpolationTimes(multiplier), generate synthetic frame
    // Returns vector of (multiplier - 1) synthetic frame resources (as void pointers).
    std::vector<void*> Execute(
        void* cmdListPtr,  // ID3D12GraphicsCommandList*
        const FrameContext& ctx,
        FrameMultiplier multiplier,
        void* motionVectorsOverride = nullptr  // ID3D12Resource* 
    );

    void Shutdown();

private:
    void* device_;  // ID3D12Device* (opaque)
    uint32_t width_, height_;
    std::unique_ptr<OcclusionMaskPass> occlusionPass_;
    std::unique_ptr<MotionVectorPass> motionVectorPass_;
    std::unique_ptr<InterpolationPass> interpolationPass_;
    std::vector<void*> syntheticsCache_;  // ID3D12Resource* (opaque)
};

} // namespace framegen
