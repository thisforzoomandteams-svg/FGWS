#include "ge2/Core/Application.h"

#include "ge2/Core/Window.h"
#include "ge2/Core/Log.h"
#include "ge2/Core/Input.h"
#include "ge2/Rendering/Renderer.h"
#include "ge2/ECS/Scene.h"
#include "ge2/Scripting/Scripting.h"
#include "ge2/Editor/ImGuiLayer.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <iostream>

namespace ge2
{
    Application::Application(const std::string& name, const ApplicationCommandLine& cmd)
        : m_CommandLine(cmd)
    {
        Log::Init(cmd.debugLogs);

        if (!glfwInit())
        {
            Log::Error("Failed to initialize GLFW.");
            throw std::runtime_error("GLFW init failed");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

        WindowProps props;
        props.Title = name;
        props.Fullscreen = !cmd.windowed;

        m_Window = std::make_unique<Window>(props, /*windowedFallback*/ true);
        glfwMakeContextCurrent(m_Window->GetNativeWindow());

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            Log::Error("Failed to initialize GLAD.");
            throw std::runtime_error("GLAD init failed");
        }

        glfwSwapInterval(1);

        Input::Init(m_Window->GetNativeWindow());

        m_Scene = std::make_unique<Scene>();
        m_Renderer = std::make_unique<Renderer>();
        m_Scripting = std::make_unique<ScriptingSystem>(m_Scene.get());
        m_ImGuiLayer = std::make_unique<ImGuiLayer>(*this);

        m_Window->SetEventCallback([this](const Event& e)
        {
            if (e.type == EventType::WindowResize)
            {
                if (e.resize.width == 0 || e.resize.height == 0)
                {
                    m_Minimized = true;
                    return;
                }
                m_Minimized = false;
                m_Renderer->Resize(e.resize.width, e.resize.height);
                glViewport(0, 0, e.resize.width, e.resize.height);
            }
            else if (e.type == EventType::WindowClose)
            {
                m_Running = false;
            }
        });

        m_LastFrameTime = Clock::now();

        OnInit();
    }

    Application::~Application()
    {
        OnShutdown();
        m_ImGuiLayer.reset();
        m_Scripting.reset();
        m_Renderer.reset();
        m_Scene.reset();

        Input::Shutdown();
        glfwTerminate();
        Log::Shutdown();
    }

    int Application::Run()
    {
        while (m_Running)
        {
            auto now = Clock::now();
            std::chrono::duration<float> dtDur = now - m_LastFrameTime;
            m_LastFrameTime = now;
            float dt = dtDur.count();

            ProcessAutoExit(dt);

            Input::BeginFrame();
            m_Window->OnUpdate();

            if (!m_Minimized)
            {
                m_ImGuiLayer->Begin();

                OnUpdate(dt);
                OnRender();

                OnImGuiRender();

                m_ImGuiLayer->End();
            }

            Input::EndFrame();
        }

        return 0;
    }

    void Application::ProcessAutoExit(float dt)
    {
        if (m_CommandLine.autoExitSeconds > 0)
        {
            m_AccumulatedTime += dt;
            if (m_AccumulatedTime >= static_cast<float>(m_CommandLine.autoExitSeconds))
            {
                Log::Info("Auto-exit reached. Shutting down.");
                m_Running = false;
            }
        }
    }

    ImGuiLayer& Application::GetImGuiLayer()
    {
        return *m_ImGuiLayer;
    }

    void Application::OnInit()
    {
        m_Renderer->Resize(m_Window->GetWidth(), m_Window->GetHeight());
    }

    void Application::OnShutdown()
    {
    }

    void Application::OnUpdate(float)
    {
    }

    void Application::OnRender()
    {
    }

    void Application::OnImGuiRender()
    {
        m_ImGuiLayer->RenderDockspace();
        m_ImGuiLayer->RenderPanels();
    }
}

