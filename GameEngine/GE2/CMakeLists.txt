cmake_minimum_required(VERSION 3.16)

project(GE2
    VERSION 0.1.0
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GE2_BUILD_SANDBOX "Build the GE2 Sandbox editor application" ON)

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    if(CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
        foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
            string(TOUPPER "${CONFIG}" CONFIG_UPPER)
            set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/bin/${CONFIG}")
        endforeach()
    else()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}")
    endif()
endif()

include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36
)
set(GLAD_PROFILE "core" CACHE STRING "" FORCE)
set(GLAD_API "gl=3.3" CACHE STRING "" FORCE)
set(GLAD_GENERATOR "c" CACHE STRING "" FORCE)
FetchContent_MakeAvailable(glad)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.89.9
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
    lua
    URL https://www.lua.org/ftp/lua-5.4.7.tar.gz
)
FetchContent_Populate(lua)

set(LUA_SRC_DIR "${lua_SOURCE_DIR}/src")
set(LUA_SOURCES
    ${LUA_SRC_DIR}/lapi.c
    ${LUA_SRC_DIR}/lcode.c
    ${LUA_SRC_DIR}/lctype.c
    ${LUA_SRC_DIR}/ldebug.c
    ${LUA_SRC_DIR}/ldo.c
    ${LUA_SRC_DIR}/ldump.c
    ${LUA_SRC_DIR}/lfunc.c
    ${LUA_SRC_DIR}/lgc.c
    ${LUA_SRC_DIR}/llex.c
    ${LUA_SRC_DIR}/lmem.c
    ${LUA_SRC_DIR}/lobject.c
    ${LUA_SRC_DIR}/lopcodes.c
    ${LUA_SRC_DIR}/lparser.c
    ${LUA_SRC_DIR}/lstate.c
    ${LUA_SRC_DIR}/lstring.c
    ${LUA_SRC_DIR}/ltable.c
    ${LUA_SRC_DIR}/ltm.c
    ${LUA_SRC_DIR}/lundump.c
    ${LUA_SRC_DIR}/lvm.c
    ${LUA_SRC_DIR}/lzio.c
    ${LUA_SRC_DIR}/lauxlib.c
    ${LUA_SRC_DIR}/lbaselib.c
    ${LUA_SRC_DIR}/lcorolib.c
    ${LUA_SRC_DIR}/ldblib.c
    ${LUA_SRC_DIR}/liolib.c
    ${LUA_SRC_DIR}/lmathlib.c
    ${LUA_SRC_DIR}/loslib.c
    ${LUA_SRC_DIR}/lstrlib.c
    ${LUA_SRC_DIR}/ltablib.c
    ${LUA_SRC_DIR}/lutf8lib.c
    ${LUA_SRC_DIR}/loadlib.c
    ${LUA_SRC_DIR}/linit.c
)
add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua PUBLIC ${LUA_SRC_DIR})
if(MSVC)
    target_compile_definitions(lua PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

add_library(ge2_sol INTERFACE)
target_include_directories(ge2_sol INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

set(IMGUI_DIR ${imgui_SOURCE_DIR})
add_library(imgui_lib
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC glfw glad)

add_library(GE2Engine
    Engine/src/Core/Application.cpp
    Engine/src/Core/Window.cpp
    Engine/src/Core/Log.cpp
    Engine/src/Core/Input.cpp
    Engine/src/Rendering/Renderer.cpp
    Engine/src/Rendering/Camera.cpp
    Engine/src/Rendering/Framebuffer.cpp
    Engine/src/Rendering/Shader.cpp
    Engine/src/ECS/Scene.cpp
    Engine/src/ECS/Entity.cpp
    Engine/src/Scripting/Scripting.cpp
    Engine/src/Editor/ImGuiLayer.cpp
    Engine/src/ThirdParty/ImGuizmo/ImGuizmo.cpp
)

target_include_directories(GE2Engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/src/ThirdParty
)

target_link_libraries(GE2Engine
    PUBLIC
        glfw
        glad
        glm
        imgui_lib
        nlohmann_json::nlohmann_json
        lua
        ge2_sol
)

if(MSVC)
    target_compile_definitions(GE2Engine PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if(GE2_BUILD_SANDBOX)
    add_executable(Sandbox
        Sandbox/src/main.cpp
        Sandbox/src/ImGuiLayout.cpp
    )
    target_link_libraries(Sandbox PRIVATE GE2Engine)

    add_custom_command(TARGET Sandbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_SOURCE_DIR}/exe"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Sandbox>"
            "${CMAKE_SOURCE_DIR}/exe/$<TARGET_FILE_NAME:Sandbox>"
    )
endif()

enable_testing()

add_test(
    NAME SandboxSmokeAutoExit1
    COMMAND Sandbox --auto-exit=1
)

add_test(
    NAME SandboxSmokeAutoExit10
    COMMAND Sandbox --auto-exit=10
)
