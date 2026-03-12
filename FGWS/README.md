# FrameGenEngine (scaffold)

This repository contains a scaffold for a Frame Generation Engine targeting DirectX12/Vulkan with multiple backend options (DLSS/FSR3/XeSS/SOFTWARE).

This initial commit provides core public headers, a basic factory, a SOFTWARE backend stub, shader stubs, and a presentation queue skeleton. Implementation and SDK integrations remain to be completed.

Build: use CMake to configure the project. You will need vendor SDKs and DX12/Vulkan SDKs to enable non-software backends.
