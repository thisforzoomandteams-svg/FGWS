cmake_minimum_required(VERSION 3.20)
project(FrameGenEngine LANGUAGES CXX)

include(FetchContent)

option(FRAMEGEN_ENABLE_DLSS "Enable DLSS Frame Generation backend" OFF)
option(FRAMEGEN_ENABLE_FSR3 "Enable FSR3 Frame Generation backend" ON)
option(FRAMEGEN_ENABLE_XESS "Enable XeSS Frame Generation backend" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# ── GLM ───────────────────────────────────────────────────────────────────────
find_package(glm QUIET)
if (NOT glm_FOUND)
    FetchContent_Declare(glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        master)
    FetchContent_MakeAvailable(glm)
endif()

# ── DirectX Headers ───────────────────────────────────────────────────────────
if (WIN32)
    find_path(D3DX12_INCLUDE_DIR "d3dx12.h"
        HINTS "$ENV{DXSDK_DIR}" "$ENV{WindowsSDKDir}")
    if (NOT D3DX12_INCLUDE_DIR)
        FetchContent_Declare(DirectX-Headers
            GIT_REPOSITORY https://github.com/microsoft/DirectX-Headers.git
            GIT_TAG        main)
        FetchContent_MakeAvailable(DirectX-Headers)
        set(D3DX12_INCLUDE_DIR "${directx-headers_SOURCE_DIR}/include")
    endif()
endif()

# ── FSR3 (open-source, auto-fetch) ───────────────────────────────────────────
if (FRAMEGEN_ENABLE_FSR3)
    find_path(FFX_SDK_ROOT "FidelityFX.h" HINTS "$ENV{FFX_SDK_ROOT}")
    if (NOT FFX_SDK_ROOT)
        FetchContent_Declare(FidelityFX-SDK
            GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK.git
            GIT_TAG        main)
        FetchContent_MakeAvailable(FidelityFX-SDK)
        set(FFX_SDK_ROOT "${fidelityfx-sdk_SOURCE_DIR}")
    endif()
    message(STATUS "FRAMEGEN: FSR3 SDK at ${FFX_SDK_ROOT}")
endif()

# ── DLSS / XeSS (env var only, no auto-download) ─────────────────────────────
if (FRAMEGEN_ENABLE_DLSS)
    find_path(NGX_SDK_ROOT_INCLUDE "ngx_dlss.h" HINTS "$ENV{NGX_SDK_ROOT}")
    if (NOT NGX_SDK_ROOT_INCLUDE)
        message(WARNING "FRAMEGEN: DLSS enabled but NGX_SDK_ROOT not set.")
    endif()
endif()

if (FRAMEGEN_ENABLE_XESS)
    find_path(XESS_SDK_ROOT_INCLUDE "xess.h" HINTS "$ENV{XESS_SDK_ROOT}")
    if (NOT XESS_SDK_ROOT_INCLUDE)
        message(WARNING "FRAMEGEN: XeSS enabled but XESS_SDK_ROOT not set.")
    endif()
endif()

# ── Core library ──────────────────────────────────────────────────────────────
add_library(framegen_core STATIC
    src/FrameGen.cpp
    src/DeviceCaps.cpp
    src/Backends/Software_FrameGen.cpp
    src/Backends/FSR3_FrameGen.cpp
    src/Backends/DLSS_FrameGen.cpp
    src/Backends/XeSS_FrameGen.cpp
)

target_include_directories(framegen_core PUBLIC include)

if (WIN32 AND D3DX12_INCLUDE_DIR)
    target_include_directories(framegen_core PRIVATE ${D3DX12_INCLUDE_DIR})
endif()

target_link_libraries(framegen_core PUBLIC glm)

# ── FSR3 include paths (point at fetched SDK) ─────────────────────────────────
if (FRAMEGEN_ENABLE_FSR3 AND FFX_SDK_ROOT)
    target_include_directories(framegen_core PRIVATE
        # Main FidelityFX host API
        "${FFX_SDK_ROOT}/Kits/FidelityFX/api/include"
        # Frame interpolation headers
        "${FFX_SDK_ROOT}/Kits/FidelityFX/framegeneration/include"
        "${FFX_SDK_ROOT}/Kits/FidelityFX/framegeneration/fsr3/include"
    )
    target_compile_definitions(framegen_core PRIVATE
        FRAMEGEN_ENABLE_FSR3=1
        FRAMEGEN_HAS_FSR3=1)
endif()

# ── Imported subproject (VS Code attached files) ──────────────────────────────
if (EXISTS "${CMAKE_SOURCE_DIR}/Imported/CMakeLists.txt")
    add_subdirectory(Imported)
    target_link_libraries(framegen_core PRIVATE framegen_imports)
endif()

# ── Platform links ────────────────────────────────────────────────────────────
if (WIN32)
    target_link_libraries(framegen_core PRIVATE d3d12 dxgi)
    target_compile_definitions(framegen_core PRIVATE FRAMEGEN_D3D12=1)
endif()

find_package(Vulkan QUIET)
if (Vulkan_FOUND)
    target_compile_definitions(framegen_core PRIVATE FRAMEGEN_HAVE_VULKAN=1)
    target_link_libraries(framegen_core PRIVATE Vulkan::Vulkan)
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────
add_executable(framegen_test Tests/FrameGenTests.cpp)
target_include_directories(framegen_test PRIVATE include)
target_link_libraries(framegen_test PRIVATE framegen_core)

enable_testing()
add_test(NAME FrameGenTests COMMAND framegen_test)
