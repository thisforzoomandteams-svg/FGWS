cmake_minimum_required(VERSION 3.20)
project(FrameGenEngine LANGUAGES CXX)

include(FetchContent)

option(FRAMEGEN_ENABLE_DLSS "Enable DLSS Frame Generation backend" OFF)
option(FRAMEGEN_ENABLE_FSR3 "Enable FSR3 Frame Generation backend" ON)
option(FRAMEGEN_ENABLE_XESS "Enable XeSS Frame Generation backend" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Make custom Find modules available
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Fetch GLM (header-only math library) if not found
find_package(glm QUIET)
if (NOT glm_FOUND)
    message(STATUS "FRAMEGEN: GLM not found, fetching from GitHub...")
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(glm)
endif()

# Fetch d3dx12.h (DirectX 12 helper headers) for Windows builds
if (WIN32)
    find_path(D3DX12_INCLUDE_DIR "d3dx12.h" HINTS "$ENV{DXSDK_DIR}" "$ENV{WindowsSDKDir}")
    if (NOT D3DX12_INCLUDE_DIR)
        message(STATUS "FRAMEGEN: d3dx12.h not found, fetching from DirectX-Headers...")
        FetchContent_Declare(
            DirectX-Headers
            GIT_REPOSITORY https://github.com/microsoft/DirectX-Headers.git
            GIT_TAG main
        )
        FetchContent_MakeAvailable(DirectX-Headers)
        set(D3DX12_INCLUDE_DIR "${directx-headers_SOURCE_DIR}/include")
    endif()
endif()

# FSR3 (open-source) FetchContent stub: fetch if enabled and not found locally
if (FRAMEGEN_ENABLE_FSR3)
    find_path(FFX_SDK_ROOT "FidelityFX.h" HINTS "$ENV{FFX_SDK_ROOT}")
    if (NOT FFX_SDK_ROOT)
        message(STATUS "FRAMEGEN: FSR3 enabled but SDK not found locally. FetchContent stub will attempt to download at configure time.")
        FetchContent_Declare(
            FidelityFX-SDK
            GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK.git
            GIT_TAG        main
        )
        FetchContent_MakeAvailable(FidelityFX-SDK)
        # User can override with -DFFX_SDK_ROOT=path
    else()
        message(STATUS "FRAMEGEN: Using local FSR3 SDK at ${FFX_SDK_ROOT}")
    endif()
endif()

# DLSS/NGX and XeSS: look for SDK roots from environment variables but do not download proprietary binaries.
if (FRAMEGEN_ENABLE_DLSS)
    find_path(NGX_SDK_ROOT_INCLUDE "ngx_dlss.h" HINTS "$ENV{NGX_SDK_ROOT}")
    if (NOT NGX_SDK_ROOT_INCLUDE)
        message(WARNING "FRAMEGEN: DLSS (NGX) enabled but NGX_SDK_ROOT not set or headers not found. Set NGX_SDK_ROOT to the SDK path to enable.")
    else()
        message(STATUS "FRAMEGEN: Found NGX SDK includes at ${NGX_SDK_ROOT_INCLUDE}")
    endif()
endif()

if (FRAMEGEN_ENABLE_XESS)
    find_path(XESS_SDK_ROOT_INCLUDE "xess.h" HINTS "$ENV{XESS_SDK_ROOT}")
    if (NOT XESS_SDK_ROOT_INCLUDE)
        message(WARNING "FRAMEGEN: XeSS enabled but XESS_SDK_ROOT not set or headers not found. Set XESS_SDK_ROOT to the SDK path to enable.")
    else()
        message(STATUS "FRAMEGEN: Found XeSS SDK includes at ${XESS_SDK_ROOT_INCLUDE}")
    endif()
endif()

# Core library
add_library(framegen_core STATIC
    src/FrameGen.cpp
    src/DeviceCaps.cpp
    src/PresentationQueue.cpp
    src/MotionVectorPass.cpp
    src/OcclusionMaskPass.cpp
    src/InterpolationPass.cpp
    src/SoftwareInterpolationPipeline.cpp
    Backends/Software_FrameGen.cpp
    Backends/FSR3_FrameGen.cpp
    Backends/DLSS_FrameGen.cpp
    Backends/XeSS_FrameGen.cpp
)

target_include_directories(framegen_core PUBLIC include)

if (WIN32 AND D3DX12_INCLUDE_DIR)
    target_include_directories(framegen_core PRIVATE ${D3DX12_INCLUDE_DIR})
endif()

target_link_libraries(framegen_core PUBLIC glm)

# If the user provided an Imported folder (from attachments), integrate it
if (EXISTS "${CMAKE_SOURCE_DIR}/Imported/CMakeLists.txt")
    message(STATUS "FRAMEGEN: Adding Imported/ subproject (attached files)")
    add_subdirectory(Imported)
    target_link_libraries(framegen_core PRIVATE framegen_imports)
endif()

if (WIN32)
    target_link_libraries(framegen_core PRIVATE d3d12 dxgi)
    target_compile_definitions(framegen_core PRIVATE FRAMEGEN_D3D12=1)
endif()

if (FRAMEGEN_ENABLE_FSR3)
    target_compile_definitions(framegen_core PRIVATE FRAMEGEN_HAVE_FSR3=1)
endif()

find_package(Vulkan QUIET)
if (Vulkan_FOUND)
    message(STATUS "FRAMEGEN: Vulkan SDK found; Vulkan support enabled")
    target_compile_definitions(framegen_core PRIVATE FRAMEGEN_HAVE_VULKAN=1)
    target_link_libraries(framegen_core PRIVATE Vulkan::Vulkan)
endif()

if (NGX_FOUND)
    target_compile_definitions(framegen_core PRIVATE FRAMEGEN_HAVE_NGX=1)
    target_include_directories(framegen_core PRIVATE ${NGX_INCLUDE_DIRS})
    target_link_libraries(framegen_core PRIVATE ${NGX_LIBRARIES})
endif()

if (XESS_FOUND)
    target_compile_definitions(framegen_core PRIVATE FRAMEGEN_HAVE_XESS=1)
    target_include_directories(framegen_core PRIVATE ${XESS_INCLUDE_DIRS})
    target_link_libraries(framegen_core PRIVATE ${XESS_LIBRARIES})
endif()

# TODO: Catch2 FetchContent failing due to network/Git issues; test compilation temporarily disabled
# add_executable(framegen_test
#     Tests/FrameGenTests.cpp
# Test executable (simple C++ tests without external framework)
add_executable(framegen_test
    Tests/FrameGenTests.cpp
)
target_include_directories(framegen_test PRIVATE include)
target_link_libraries(framegen_test PRIVATE framegen_core)

enable_testing()
add_test(NAME FrameGenTests COMMAND framegen_test)
