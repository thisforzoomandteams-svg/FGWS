#pragma once

#include <cstdint>

#if defined(_WIN32)
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;
struct ID3D12RootSignature;
struct ID3D12PipelineState;
#endif

namespace framegen {

struct InterpolationCB {
    float t;                  // Interpolation parameter [0, 1]
    uint32_t occlusionThreshold;
    float confidenceDecay;    // Exponential decay factor for multi-frame confidence
    float padding;
};

class InterpolationPass {
public:
    InterpolationPass(ID3D12Device* device, ID3D12RootSignature* rootSig, ID3D12PipelineState* pso);
    ~InterpolationPass();

    // Dispatch interpolation shader to generate a synthetic frame at parameter t.
    // This runs (multiplier - 1) times from the pipeline executor.
    void Dispatch(
        ID3D12GraphicsCommandList* cmdList,
        const InterpolationCB& interpCB,
        ID3D12Resource* colorA,
        ID3D12Resource* colorB,
        ID3D12Resource* motionVectors,
        ID3D12Resource* confidenceMap,
        ID3D12Resource* outputFrame,
        uint32_t width, uint32_t height
    );

private:
    ID3D12Device* device_;
    ID3D12RootSignature* rootSig_;
    ID3D12PipelineState* pso_;
    ID3D12Resource* uploadHeap_;
};

} // namespace framegen
