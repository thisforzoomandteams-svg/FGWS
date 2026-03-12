#pragma once

#include <cstdint>
#include <span>
#include <memory>

#if defined(_WIN32)
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;
struct ID3D12RootSignature;
struct ID3D12PipelineState;
#endif

namespace framegen {

struct CameraCB {
    float viewProjCurr[16];  // row-major mat4
    float viewProjPrev[16];
};

class MotionVectorPass {
public:
    MotionVectorPass(ID3D12Device* device, ID3D12RootSignature* rootSig, ID3D12PipelineState* pso);
    ~MotionVectorPass();

    // Dispatch motion vector computation shader.
    // Inputs: depth buffer (SRV), camera matrices (CBV)
    // Output: motion vectors texture (UAV, RG16F)
    void Dispatch(
        ID3D12GraphicsCommandList* cmdList,
        const CameraCB& cameraCB,
        ID3D12Resource* depthBuffer,
        ID3D12Resource* motionVectorsOutput,
        uint32_t width, uint32_t height
    );

private:
    ID3D12Device* device_;
    ID3D12RootSignature* rootSig_;
    ID3D12PipelineState* pso_;
    ID3D12Resource* uploadHeap_;
};

} // namespace framegen
