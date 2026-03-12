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

struct OcclusionCB {
    uint32_t depthThreshold;  // Sobel threshold for occlusion detection
    float padding[3];
};

class OcclusionMaskPass {
public:
    OcclusionMaskPass(ID3D12Device* device, ID3D12RootSignature* rootSig, ID3D12PipelineState* pso);
    ~OcclusionMaskPass();

    // Dispatch occlusion mask computation (Sobel on depth, output confidence map R8_UNORM)
    void Dispatch(
        ID3D12GraphicsCommandList* cmdList,
        const OcclusionCB& occlusionCB,
        ID3D12Resource* depthBuffer,
        ID3D12Resource* confidenceOutput,
        uint32_t width, uint32_t height
    );

private:
    ID3D12Device* device_;
    ID3D12RootSignature* rootSig_;
    ID3D12PipelineState* pso_;
    ID3D12Resource* uploadHeap_;
};

} // namespace framegen
