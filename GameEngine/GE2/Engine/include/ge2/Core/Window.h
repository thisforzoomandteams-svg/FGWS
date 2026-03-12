#pragma once

#include <string>
#include <functional>
#include <stdint.h>

struct GLFWwindow;

namespace ge2
{
    enum class EventType
    {
        None = 0,
        WindowClose,
        WindowResize
    };

    struct WindowResizeEvent
    {
        int width = 0;
        int height = 0;
    };

    struct Event
    {
        EventType type = EventType::None;
        WindowResizeEvent resize;
    };

    using EventCallbackFn = std::function<void(const Event&)>;

    struct WindowProps
    {
        std::string Title = \"GE2\";
        uint32_t Width = 1280;
        uint32_t Height = 720;
        bool Fullscreen = true;
    };

    class Window
    {
    public:
        Window(const WindowProps& props, bool windowedFallback);
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        void OnUpdate();

        uint32_t GetWidth() const { return m_Data.Width; }
        uint32_t GetHeight() const { return m_Data.Height; }

        GLFWwindow* GetNativeWindow() const { return m_Window; }

        void SetEventCallback(const EventCallbackFn& cb) { m_EventCallback = cb; }

        bool IsFullscreen() const { return m_Data.Fullscreen; }

    private:
        void Init(const WindowProps& props, bool windowedFallback);
        void Shutdown();

    private:
        GLFWwindow* m_Window = nullptr;

        struct WindowData
        {
            std::string Title;
            uint32_t Width;
            uint32_t Height;
            bool Fullscreen;
        } m_Data;

        EventCallbackFn m_EventCallback;
    };
}

