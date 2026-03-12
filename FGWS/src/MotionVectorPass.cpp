#include "../include/MotionVectorPass.hpp"

#if defined(_WIN32)
#include <d3d12.h>
#include <cmath>
#include <cstring>
#endif

using namespace framegen;

MotionVectorPass::MotionVectorPass(ID3D12Device* device, ID3D12RootSignature* rootSig, ID3D12PipelineState* pso)
    : device_(device), rootSig_(rootSig), pso_(pso), uploadHeap_(nullptr) {
}

MotionVectorPass::~MotionVectorPass() {
    if (uploadHeap_) uploadHeap_->Release();
    uploadHeap_ = nullptr;
}

void MotionVectorPass::Dispatch(
    ID3D12GraphicsCommandList* cmdList,
    const CameraCB& cameraCB,
    ID3D12Resource* depthBuffer,
    ID3D12Resource* motionVectorsOutput,
    uint32_t width, uint32_t height
) {
#if defined(_WIN32) && defined(FRAMEGEN_D3D12)
    if (!cmdList || !depthBuffer || !motionVectorsOutput) return;

    // Upload constant buffer (camera matrices) via upload heap
    if (!uploadHeap_) {
        D3D12_HEAP_PROPERTIES heapProps;
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC uploadDesc;
        uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        uploadDesc.Alignment = 0;
        uploadDesc.Width = sizeof(CameraCB);
        uploadDesc.Height = 1;
        uploadDesc.DepthOrArraySize = 1;
        uploadDesc.MipLevels = 1;
        uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
        uploadDesc.SampleDesc.Count = 1;
        uploadDesc.SampleDesc.Quality = 0;
        uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        device_->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &uploadDesc, 
                                          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadHeap_));
    }

    // Map and copy camera CB data
    void* mappedPtr = nullptr;
    uploadHeap_->Map(0, nullptr, &mappedPtr);
    memcpy(mappedPtr, &cameraCB, sizeof(CameraCB));
    uploadHeap_->Unmap(0, nullptr);

    // Transition depth to SRV and MV output to UAV
    D3D12_RESOURCE_BARRIER depthBarrier;
    depthBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    depthBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    depthBarrier.Transition.pResource = depthBuffer;
    depthBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    depthBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    depthBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    D3D12_RESOURCE_BARRIER mvBarrier;
    mvBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    mvBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    mvBarrier.Transition.pResource = motionVectorsOutput;
    mvBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    mvBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    mvBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    cmdList->ResourceBarrier(2, &depthBarrier);
    cmdList->ResourceBarrier(1, &mvBarrier);

    // Set PSO, root signature, and descriptor tables
    cmdList->SetPipelineState(pso_);
    cmdList->SetComputeRootSignature(rootSig_);
    cmdList->SetComputeRootConstantBufferView(0, uploadHeap_->GetGPUVirtualAddress());
    // TODO: Set descriptor table for depth SRV and MV UAV

    // Dispatch: ceil(width/8) x ceil(height/8)
    uint32_t dispatchX = (width + 7) / 8;
    uint32_t dispatchY = (height + 7) / 8;
    cmdList->Dispatch(dispatchX, dispatchY, 1);

    // Transition MV to SRV for next pass
    D3D12_RESOURCE_BARRIER mvToSrvBarrier;
    mvToSrvBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    mvToSrvBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    mvToSrvBarrier.Transition.pResource = motionVectorsOutput;
    mvToSrvBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    mvToSrvBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    mvToSrvBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    cmdList->ResourceBarrier(1, &mvToSrvBarrier);
#endif
}
