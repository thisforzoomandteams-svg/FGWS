#pragma once

#include <memory>
#include <string>
#include <chrono>

namespace ge2
{
    class Window;
    class ImGuiLayer;
    class Scene;
    class Renderer;
    class ScriptingSystem;

    struct ApplicationCommandLine
    {
        bool windowed = false;
        bool debugLogs = false;
        int autoExitSeconds = -1;
    };

    class Application
    {
    public:
        Application(const std::string& name, const ApplicationCommandLine& cmd);
        virtual ~Application();

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        int Run();

        Window& GetWindow() { return *m_Window; }
        Scene& GetScene() { return *m_Scene; }
        Renderer& GetRenderer() { return *m_Renderer; }
        ScriptingSystem& GetScripting() { return *m_Scripting; }
        ImGuiLayer& GetImGuiLayer();

        const ApplicationCommandLine& GetCommandLine() const { return m_CommandLine; }

    protected:
        virtual void OnInit();
        virtual void OnShutdown();
        virtual void OnUpdate(float dt);
        virtual void OnRender();
        virtual void OnImGuiRender();

    private:
        void ProcessAutoExit(float dt);

    private:
        ApplicationCommandLine m_CommandLine{};
        std::unique_ptr<Window> m_Window;
        std::unique_ptr<ImGuiLayer> m_ImGuiLayer;
        std::unique_ptr<Scene> m_Scene;
        std::unique_ptr<Renderer> m_Renderer;
        std::unique_ptr<ScriptingSystem> m_Scripting;

        bool m_Running = true;
        bool m_Minimized = false;
        float m_AccumulatedTime = 0.0f;

        using Clock = std::chrono::high_resolution_clock;
        Clock::time_point m_LastFrameTime;
    };

    Application* CreateApplication(const ApplicationCommandLine& cmd);
}

