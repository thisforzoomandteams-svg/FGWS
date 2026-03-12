// Interpolate.hlsl
// RootSig: SRV x4 (colorA, colorB, MVs, depth), UAV (output), CBV (params)
// Params CB: { float t; uint occlusionThreshold; float confidenceDecay; }
// Thread group: [16,16,1]

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID) {
    // Skeleton: bilinear warp + occlusion handling + confidence blend
}
