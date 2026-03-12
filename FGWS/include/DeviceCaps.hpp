#pragma once

#include <cstdint>
#include <string>

namespace framegen {

struct DeviceCaps {
    uint32_t         vendorID         = 0;
    uint32_t         deviceID         = 0;
    bool             supportsNGX      = false;
    bool             supportsFSR3     = false;
    bool             supportsXeSS     = false;
    bool             supportsWave64   = false;
    std::string      driverVersion;
};

// Probe device capabilities (DX12/Vulkan) - scaffold stub.
DeviceCaps DetectDeviceCaps();

} // namespace framegen
