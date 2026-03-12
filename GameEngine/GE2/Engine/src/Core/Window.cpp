#include "ge2/Core/Window.h"
#include "ge2/Core/Log.h"

#include <GLFW/glfw3.h>

namespace ge2
{
    static void GLFWErrorCallback(int error, const char* description)
    {
        Log::Error("GLFW Error (" + std::to_string(error) + "): " + description);
    }

    Window::Window(const WindowProps& props, bool windowedFallback)
    {
        Init(props, windowedFallback);
    }

    Window::~Window()
    {
        Shutdown();
    }

    void Window::Init(const WindowProps& props, bool windowedFallback)
    {
        glfwSetErrorCallback(GLFWErrorCallback);

        m_Data.Title = props.Title;
        m_Data.Width = props.Width;
        m_Data.Height = props.Height;
        m_Data.Fullscreen = props.Fullscreen;

        GLFWmonitor* monitor = nullptr;
        const GLFWvidmode* mode = nullptr;

        if (props.Fullscreen)
        {
            monitor = glfwGetPrimaryMonitor();
            if (monitor)
            {
                mode = glfwGetVideoMode(monitor);
                m_Data.Width = mode->width;
                m_Data.Height = mode->height;
                m_Window = glfwCreateWindow((int)m_Data.Width, (int)m_Data.Height, m_Data.Title.c_str(), monitor, nullptr);
            }

            if (!m_Window && windowedFallback)
            {
                Log::Warn("Fullscreen window creation failed. Falling back to windowed mode.");
                m_Data.Fullscreen = false;
            }
        }

        if (!m_Window)
        {
            m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, props.Title.c_str(), nullptr, nullptr);
        }

        if (!m_Window)
        {
            Log::Error("Failed to create GLFW window.");
            throw std::runtime_error("GLFW window creation failed");
        }

        glfwMakeContextCurrent(m_Window);

        glfwSetWindowUserPointer(m_Window, this);

        glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
        {
            Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
            win->m_Data.Width = static_cast<uint32_t>(width);
            win->m_Data.Height = static_cast<uint32_t>(height);

            if (win->m_EventCallback)
            {
                Event e;
                e.type = EventType::WindowResize;
                e.resize.width = width;
                e.resize.height = height;
                win->m_EventCallback(e);
            }
        });

        glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
        {
            Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
            if (win->m_EventCallback)
            {
                Event e;
                e.type = EventType::WindowClose;
                win->m_EventCallback(e);
            }
        });
    }

    void Window::Shutdown()
    {
        if (m_Window)
        {
            glfwDestroyWindow(m_Window);
            m_Window = nullptr;
        }
    }

    void Window::OnUpdate()
    {
        glfwPollEvents();
        glfwSwapBuffers(m_Window);
    }
}

