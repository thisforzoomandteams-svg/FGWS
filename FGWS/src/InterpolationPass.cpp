#include "../include/InterpolationPass.hpp"

#if defined(_WIN32)
#include <d3d12.h>
#include <cmath>
#include <cstring>
#endif

using namespace framegen;

InterpolationPass::InterpolationPass(ID3D12Device* device, ID3D12RootSignature* rootSig, ID3D12PipelineState* pso)
    : device_(device), rootSig_(rootSig), pso_(pso), uploadHeap_(nullptr) {
}

InterpolationPass::~InterpolationPass() {
    if (uploadHeap_) uploadHeap_->Release();
    uploadHeap_ = nullptr;
}

void InterpolationPass::Dispatch(
    ID3D12GraphicsCommandList* cmdList,
    const InterpolationCB& interpCB,
    ID3D12Resource* colorA,
    ID3D12Resource* colorB,
    ID3D12Resource* motionVectors,
    ID3D12Resource* confidenceMap,
    ID3D12Resource* outputFrame,
    uint32_t width, uint32_t height
) {
#if defined(_WIN32) && defined(FRAMEGEN_D3D12)
    if (!cmdList || !colorA || !colorB || !motionVectors || !outputFrame) return;

    // Upload interpolation CB
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
        uploadDesc.Width = sizeof(InterpolationCB);
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

    void* mappedPtr = nullptr;
    uploadHeap_->Map(0, nullptr, &mappedPtr);
    memcpy(mappedPtr, &interpCB, sizeof(InterpolationCB));
    uploadHeap_->Unmap(0, nullptr);

    // Transition inputs to SRV, output to UAV
    D3D12_RESOURCE_BARRIER barriers[4]{};
    for (int i = 0; i < 4; ++i) {
        barriers[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[i].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    }

    barriers[0].Transition.pResource = colorA;
    barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    barriers[1].Transition.pResource = colorB;
    barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    barriers[2].Transition.pResource = motionVectors;
    barriers[2].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barriers[2].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barriers[2].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    barriers[3].Transition.pResource = outputFrame;
    barriers[3].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barriers[3].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barriers[3].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    cmdList->ResourceBarrier(4, barriers);

    // Set PSO, root signature, and CBV
    cmdList->SetPipelineState(pso_);
    cmdList->SetComputeRootSignature(rootSig_);
    cmdList->SetComputeRootConstantBufferView(0, uploadHeap_->GetGPUVirtualAddress());
    // TODO: Set descriptor table for SRVs (colorA, colorB, MV, confidence) and UAV (output)

    // Dispatch: ceil(width/16) x ceil(height/16)
    uint32_t dispatchX = (width + 15) / 16;
    uint32_t dispatchY = (height + 15) / 16;
    cmdList->Dispatch(dispatchX, dispatchY, 1);

    // Transition output to COPY_SOURCE for later retrieval (or keep as UAV for next iteration)
    D3D12_RESOURCE_BARRIER outputBarrier;
    outputBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    outputBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    outputBarrier.Transition.pResource = outputFrame;
    outputBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    outputBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    outputBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    cmdList->ResourceBarrier(1, &outputBarrier);
#endif
}
