#include "../include/DeviceCaps.hpp"

#if defined(_WIN32)
#include <windows.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3d12")
#endif

#if defined(FRAMEGEN_HAVE_VULKAN)
#include <vulkan/vulkan.h>
#endif

#include <vector>
#include <string>
#include <locale>
#include <codecvt>

using namespace framegen;

DeviceCaps DetectDeviceCaps() {
    DeviceCaps caps;
    caps.vendorID = 0;
    caps.deviceID = 0;
    caps.supportsNGX = false;
    caps.supportsFSR3 = false;
    caps.supportsXeSS = false;
    caps.supportsWave64 = false;
    caps.driverVersion = "unknown";

#if defined(FRAMEGEN_HAVE_NGX)
    caps.supportsNGX = true;
#endif
#if defined(FRAMEGEN_HAVE_FSR3)
    caps.supportsFSR3 = true;
#endif
#if defined(FRAMEGEN_HAVE_XESS)
    caps.supportsXeSS = true;
#endif

    // Try DXGI / D3D12 first on Windows
#if defined(_WIN32)
    IDXGIFactory6* factory = nullptr;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        IDXGIAdapter1* adapter = nullptr;
        for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            // Skip software adapters
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                adapter->Release();
                continue;
            }

            caps.vendorID = desc.VendorId;
            caps.deviceID = desc.DeviceId;
            caps.driverVersion = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(desc.Description);

            // Basic heuristic for wave64: check for recent AMD/NVIDIA GPUs (placeholder)
            if (caps.vendorID == 0x1002 || caps.vendorID == 0x10DE) {
                caps.supportsWave64 = true;
            }

            adapter->Release();
            break;
        }
        factory->Release();
    }
#endif

    // If vendor not detected via DXGI and Vulkan is available, query Vulkan
#if defined(FRAMEGEN_HAVE_VULKAN)
    VkInstance instance = VK_NULL_HANDLE;
    VkInstanceCreateInfo ici{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    if (vkCreateInstance(&ici, nullptr, &instance) == VK_SUCCESS) {
        uint32_t gpuCount = 0;
        if (vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr) == VK_SUCCESS && gpuCount > 0) {
            std::vector<VkPhysicalDevice> gpus(gpuCount);
            vkEnumeratePhysicalDevices(instance, &gpuCount, gpus.data());
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(gpus[0], &props);
            caps.vendorID = props.vendorID;
            caps.deviceID = props.deviceID;
            caps.driverVersion = std::to_string(props.driverVersion);
        }
        vkDestroyInstance(instance, nullptr);
    }
#endif

    return caps;
}

