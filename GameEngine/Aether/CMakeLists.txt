cmake_minimum_required(VERSION 3.16)

project(Aether LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep build outputs in predictable locations.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
foreach(config Debug Release RelWithDebInfo MinSizeRel)
  string(TOUPPER "${config}" config_uc)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_uc} "${CMAKE_BINARY_DIR}/bin/${config}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config_uc} "${CMAKE_BINARY_DIR}/lib/${config}")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config_uc} "${CMAKE_BINARY_DIR}/lib/${config}")
endforeach()

include(FetchContent)

# ---- Dependencies ----

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.9
)

# GLM
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.3
)

# GLAD (glad2)
FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG v2.0.8
)

# ImGui
FetchContent_Declare(
  imgui_src
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  # Docking APIs are required for the editor layout in this iteration.
  GIT_TAG docking
)

# nlohmann/json (scene serialization)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)

# sol2 (Lua bindings) - header-only
FetchContent_Declare(
  sol2
  GIT_REPOSITORY https://github.com/ThePhD/sol2.git
  GIT_TAG v3.3.0
)

# Lua 5.4.6 - official repo (no CMake), we build a static library ourselves.
FetchContent_Declare(
  lua_src
  GIT_REPOSITORY https://github.com/lua/lua.git
  GIT_TAG v5.4.6
)

FetchContent_MakeAvailable(glfw glm glad imgui_src nlohmann_json sol2)

FetchContent_GetProperties(lua_src)
if(NOT lua_src_POPULATED)
  FetchContent_Populate(lua_src)
endif()

# Build Lua as a static library (the official repo ships makefiles but no CMake project).
if(NOT TARGET lua)
  add_library(lua STATIC
    "${lua_src_SOURCE_DIR}/lapi.c"
    "${lua_src_SOURCE_DIR}/lauxlib.c"
    "${lua_src_SOURCE_DIR}/lbaselib.c"
    "${lua_src_SOURCE_DIR}/lcode.c"
    "${lua_src_SOURCE_DIR}/lcorolib.c"
    "${lua_src_SOURCE_DIR}/lctype.c"
    "${lua_src_SOURCE_DIR}/ldblib.c"
    "${lua_src_SOURCE_DIR}/ldebug.c"
    "${lua_src_SOURCE_DIR}/ldo.c"
    "${lua_src_SOURCE_DIR}/ldump.c"
    "${lua_src_SOURCE_DIR}/lfunc.c"
    "${lua_src_SOURCE_DIR}/lgc.c"
    "${lua_src_SOURCE_DIR}/linit.c"
    "${lua_src_SOURCE_DIR}/liolib.c"
    "${lua_src_SOURCE_DIR}/llex.c"
    "${lua_src_SOURCE_DIR}/lmathlib.c"
    "${lua_src_SOURCE_DIR}/lmem.c"
    "${lua_src_SOURCE_DIR}/loadlib.c"
    "${lua_src_SOURCE_DIR}/lobject.c"
    "${lua_src_SOURCE_DIR}/lopcodes.c"
    "${lua_src_SOURCE_DIR}/loslib.c"
    "${lua_src_SOURCE_DIR}/lparser.c"
    "${lua_src_SOURCE_DIR}/lstate.c"
    "${lua_src_SOURCE_DIR}/lstring.c"
    "${lua_src_SOURCE_DIR}/lstrlib.c"
    "${lua_src_SOURCE_DIR}/ltable.c"
    "${lua_src_SOURCE_DIR}/ltablib.c"
    "${lua_src_SOURCE_DIR}/ltm.c"
    "${lua_src_SOURCE_DIR}/lundump.c"
    "${lua_src_SOURCE_DIR}/lutf8lib.c"
    "${lua_src_SOURCE_DIR}/lvm.c"
    "${lua_src_SOURCE_DIR}/lzio.c"
  )
  target_include_directories(lua PUBLIC "${lua_src_SOURCE_DIR}")
  if(MSVC)
    target_compile_definitions(lua PRIVATE _CRT_SECURE_NO_WARNINGS)
  endif()
endif()

set(AETHER_LUA_TARGET lua)
set(AETHER_SOL2_INCLUDE_DIR "${sol2_SOURCE_DIR}/include")

if(TARGET glm::glm)
  set(AETHER_GLM_TARGET glm::glm)
elseif(TARGET glm)
  set(AETHER_GLM_TARGET glm)
else()
  message(FATAL_ERROR "GLM target not found after FetchContent_MakeAvailable(glm)")
endif()

find_package(OpenGL REQUIRED)

# Build glad via its CMake helper (generates sources at configure time).
add_subdirectory("${glad_SOURCE_DIR}/cmake" glad_cmake)
glad_add_library(glad_gl_core_33 REPRODUCIBLE API gl:core=3.3)

# Build ImGui as a local static library from the fetched sources.
add_library(imgui STATIC)
target_sources(imgui PRIVATE
  "${imgui_src_SOURCE_DIR}/imgui.cpp"
  "${imgui_src_SOURCE_DIR}/imgui_demo.cpp"
  "${imgui_src_SOURCE_DIR}/imgui_draw.cpp"
  "${imgui_src_SOURCE_DIR}/imgui_tables.cpp"
  "${imgui_src_SOURCE_DIR}/imgui_widgets.cpp"
  "${imgui_src_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
  "${imgui_src_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
)
target_include_directories(imgui PUBLIC
  "${imgui_src_SOURCE_DIR}"
  "${imgui_src_SOURCE_DIR}/backends"
)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)
target_compile_definitions(imgui PUBLIC
  IMGUI_IMPL_OPENGL_LOADER_GLAD
  GLFW_INCLUDE_NONE
)

if(TARGET nlohmann_json::nlohmann_json)
  set(AETHER_NLOHMANN_JSON_TARGET nlohmann_json::nlohmann_json)
elseif(TARGET nlohmann_json)
  set(AETHER_NLOHMANN_JSON_TARGET nlohmann_json)
else()
  message(FATAL_ERROR "nlohmann/json target not found after FetchContent_MakeAvailable(nlohmann_json)")
endif()

add_subdirectory(Engine)
add_subdirectory(Sandbox)
