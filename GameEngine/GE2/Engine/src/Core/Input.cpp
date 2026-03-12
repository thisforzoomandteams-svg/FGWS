#include "ge2/Core/Input.h"

#include <GLFW/glfw3.h>

namespace ge2
{
    void Input::Init(GLFWwindow* window)
    {
        s_Window = window;
    }

    void Input::Shutdown()
    {
        s_Window = nullptr;
        s_KeyDown.clear();
        s_KeyPressed.clear();
        s_MouseDown.clear();
    }

    void Input::BeginFrame()
    {
        s_KeyPressed.clear();

        if (!s_Window)
            return;

        for (auto& kv : s_KeyDown)
        {
            int key = kv.first;
            int state = glfwGetKey(s_Window, key);
            bool downNow = state == GLFW_PRESS || state == GLFW_REPEAT;
            if (downNow && !kv.second)
            {
                s_KeyPressed[key] = true;
            }
            kv.second = downNow;
        }

        for (auto& kv : s_MouseDown)
        {
            int button = kv.first;
            int state = glfwGetMouseButton(s_Window, button);
            kv.second = (state == GLFW_PRESS);
        }
    }

    void Input::EndFrame()
    {
    }

    bool Input::IsKeyDown(int key)
    {
        if (!s_Window)
            return false;

        int state = glfwGetKey(s_Window, key);
        if (state == GLFW_PRESS || state == GLFW_REPEAT)
        {
            s_KeyDown[key] = true;
            return true;
        }
        return false;
    }

    bool Input::IsKeyPressed(int key)
    {
        auto it = s_KeyPressed.find(key);
        return it != s_KeyPressed.end() && it->second;
    }

    bool Input::IsMouseButtonDown(int button)
    {
        if (!s_Window)
            return false;

        int state = glfwGetMouseButton(s_Window, button);
        s_MouseDown[button] = (state == GLFW_PRESS);
        return s_MouseDown[button];
    }

    glm::vec2 Input::GetMousePosition()
    {
        if (!s_Window)
            return {0.0f, 0.0f};

        double x, y;
        glfwGetCursorPos(s_Window, &x, &y);
        return {(float)x, (float)y};
    }

    static int KeyFromName(const std::string& name)
    {
        if (name.size() == 1)
        {
            char c = name[0];
            if (c >= 'A' && c <= 'Z')
                return GLFW_KEY_A + (c - 'A');
            if (c >= 'a' && c <= 'z')
                return GLFW_KEY_A + (c - 'a');
        }

        if (name == "Space") return GLFW_KEY_SPACE;
        if (name == "Shift") return GLFW_KEY_LEFT_SHIFT;
        if (name == "Ctrl") return GLFW_KEY_LEFT_CONTROL;

        return GLFW_KEY_UNKNOWN;
    }

    bool Input::IsKeyDownName(const std::string& name)
    {
        int key = KeyFromName(name);
        if (key == GLFW_KEY_UNKNOWN)
            return false;
        return IsKeyDown(key);
    }

    bool Input::IsKeyPressedName(const std::string& name)
    {
        int key = KeyFromName(name);
        if (key == GLFW_KEY_UNKNOWN)
            return false;
        return IsKeyPressed(key);
    }
}

