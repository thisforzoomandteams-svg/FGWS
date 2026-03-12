add_library(AetherEngine STATIC)
add_library(Aether::Engine ALIAS AetherEngine)

target_sources(AetherEngine PRIVATE
  src/Aether.cpp
  src/Core/Application.cpp
  src/Core/Window.cpp
  src/Editor/ImGuiLayout.cpp
  src/Editor/ImGuiLayer.cpp
  src/Editor/ViewportPanel.cpp
  src/ECS/Entity.cpp
  src/ECS/Prefab.cpp
  src/ECS/Scene.cpp
  src/ECS/Components/ScriptComponent.cpp
  src/Scripting/ScriptEngine.cpp
  src/Scripting/ScriptRegistry.cpp
  src/Serialization/SceneSerializer.cpp
  src/Rendering/Framebuffer.cpp
  src/Rendering/Camera.cpp
  src/Rendering/IndexBuffer.cpp
  src/Rendering/Renderer.cpp
  src/Rendering/Shader.cpp
  src/Rendering/ShadowMap.cpp
  src/Rendering/VertexArray.cpp
  src/Rendering/VertexBuffer.cpp
  src/ThirdParty/ImGuizmo/ImGuizmo.cpp
)

target_include_directories(AetherEngine PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/ThirdParty/ImGuizmo"
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(AetherEngine PUBLIC
  glfw
  glad_gl_core_33
  ${AETHER_GLM_TARGET}
  ${AETHER_NLOHMANN_JSON_TARGET}
  imgui
  OpenGL::GL
)

target_link_libraries(AetherEngine PUBLIC
  ${AETHER_LUA_TARGET}
)

target_include_directories(AetherEngine PUBLIC
  "${AETHER_SOL2_INCLUDE_DIR}"
)

target_compile_definitions(AetherEngine PUBLIC GLFW_INCLUDE_NONE)

if(MSVC)
  target_compile_definitions(AetherEngine PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Required by ImGuizmo (includes imgui_internal.h).
target_compile_definitions(AetherEngine PRIVATE IMGUI_DEFINE_MATH_OPERATORS)
