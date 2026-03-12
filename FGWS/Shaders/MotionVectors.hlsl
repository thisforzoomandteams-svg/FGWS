// MotionVectors.hlsl
// RootSig: CBV (camera matrices), SRV (depth), UAV (MV output)
// Thread group: [8,8,1]

[numthreads(8,8,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID) {
    // Skeleton: per-pixel reprojection NDC(prev) - NDC(curr) -> output RG16F
}
