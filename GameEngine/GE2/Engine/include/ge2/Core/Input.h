#pragma once

#include <glm/vec2.hpp>
#include <unordered_map>
#include <string>

struct GLFWwindow;

namespace ge2
{
    class Input
    {
    public:
        static void Init(GLFWwindow* window);
        static void Shutdown();

        static void BeginFrame();
        static void EndFrame();

        static bool IsKeyDown(int key);
        static bool IsKeyPressed(int key);
        static bool IsMouseButtonDown(int button);

        static glm::vec2 GetMousePosition();

        static bool IsKeyDownName(const std::string& name);
        static bool IsKeyPressedName(const std::string& name);

    private:
        static inline GLFWwindow* s_Window = nullptr;
        static inline std::unordered_map<int, bool> s_KeyDown;
        static inline std::unordered_map<int, bool> s_KeyPressed;
        static inline std::unordered_map<int, bool> s_MouseDown;
    };
}

