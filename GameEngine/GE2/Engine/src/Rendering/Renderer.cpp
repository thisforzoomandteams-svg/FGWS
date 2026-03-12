#include "ge2/Rendering/Renderer.h"

#include "ge2/Rendering/Framebuffer.h"
#include "ge2/Rendering/Shader.h"
#include "ge2/ECS/Scene.h"
#include "ge2/ECS/Components.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

namespace ge2
{
    namespace
    {
        const char* s_VertexSrc = R"(#version 330 core
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Proj;

out vec3 v_FragPos;
out vec3 v_Normal;

void main()
{
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    v_FragPos = worldPos.xyz;
    v_Normal = mat3(transpose(inverse(u_Model))) * a_Normal;
    gl_Position = u_Proj * u_View * worldPos;
}
)";

        const char* s_FragmentSrc = R"(#version 330 core
struct Material {
    vec3 Albedo;
    vec3 Specular;
    float Shininess;
};

struct PointLight {
    vec3 Position;
    vec3 Color;
    float Intensity;
    float Radius;
};

uniform Material u_Material;
uniform vec3 u_ViewPos;
uniform PointLight u_PointLight;

in vec3 v_FragPos;
in vec3 v_Normal;

out vec4 FragColor;

void main()
{
    vec3 ambient = 0.1 * u_Material.Albedo;

    vec3 norm = normalize(v_Normal);
    vec3 lightDir = normalize(u_PointLight.Position - v_FragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * u_PointLight.Color * u_Material.Albedo;

    vec3 viewDir = normalize(u_ViewPos - v_FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Material.Shininess);
    vec3 specular = spec * u_PointLight.Color * u_Material.Specular;

    float dist = length(u_PointLight.Position - v_FragPos);
    float attenuation = clamp(1.0 - dist / u_PointLight.Radius, 0.0, 1.0);
    vec3 color = ambient + (diffuse + specular) * u_PointLight.Intensity * attenuation;

    FragColor = vec4(color, 1.0);
}
)";

        const char* s_WireVertex = R"(#version 330 core
layout (location = 0) in vec3 a_Position;
uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Proj;
void main()
{
    gl_Position = u_Proj * u_View * u_Model * vec4(a_Position, 1.0);
}
)";

        const char* s_WireFragment = R"(#version 330 core
uniform vec3 u_Color;
out vec4 FragColor;
void main()
{
    FragColor = vec4(u_Color, 1.0);
}
)";
    }

    Renderer::Renderer()
    {
        InitGLResources();
    }

    Renderer::~Renderer()
    {
        ShutdownGLResources();
    }

    void Renderer::InitGLResources()
    {
        m_Framebuffer = std::make_unique<Framebuffer>(1280, 720);

        float cubeVertices[] = {
            -0.5f,-0.5f,-0.5f,   0.0f, 0.0f,-1.0f,
             0.5f,-0.5f,-0.5f,   0.0f, 0.0f,-1.0f,
             0.5f, 0.5f,-0.5f,   0.0f, 0.0f,-1.0f,
            -0.5f, 0.5f,-0.5f,   0.0f, 0.0f,-1.0f,
            -0.5f,-0.5f, 0.5f,   0.0f, 0.0f, 1.0f,
             0.5f,-0.5f, 0.5f,   0.0f, 0.0f, 1.0f,
             0.5f, 0.5f, 0.5f,   0.0f, 0.0f, 1.0f,
            -0.5f, 0.5f, 0.5f,   0.0f, 0.0f, 1.0f,
        };

        unsigned int cubeIndices[] = {
            0,1,2, 2,3,0,
            4,5,6, 6,7,4,
            0,1,5, 5,4,0,
            2,3,7, 7,6,2,
            0,3,7, 7,4,0,
            1,2,6, 6,5,1
        };

        glGenVertexArrays(1, &m_CubeVAO);
        glGenBuffers(1, &m_CubeVBO);
        glGenBuffers(1, &m_CubeIBO);

        glBindVertexArray(m_CubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_CubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_CubeIBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (const void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (const void*)(3 * sizeof(float)));

        glBindVertexArray(0);

        float planeVertices[] = {
            -0.5f, 0.0f,-0.5f, 0.0f, 1.0f, 0.0f,
             0.5f, 0.0f,-0.5f, 0.0f, 1.0f, 0.0f,
             0.5f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f,
            -0.5f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f,
        };
        unsigned int planeIndices[] = {
            0,1,2,
            2,3,0
        };

        glGenVertexArrays(1, &m_PlaneVAO);
        glGenBuffers(1, &m_PlaneVBO);
        glGenBuffers(1, &m_PlaneIBO);

        glBindVertexArray(m_PlaneVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_PlaneVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_PlaneIBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (const void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (const void*)(3 * sizeof(float)));

        glBindVertexArray(0);

        m_ShaderProgram = Shader::CompileProgram(s_VertexSrc, s_FragmentSrc);
        m_WireframeShader = Shader::CompileProgram(s_WireVertex, s_WireFragment);
    }

    void Renderer::ShutdownGLResources()
    {
        if (m_CubeVAO)
        {
            glDeleteVertexArrays(1, &m_CubeVAO);
            glDeleteBuffers(1, &m_CubeVBO);
            glDeleteBuffers(1, &m_CubeIBO);
        }
        if (m_PlaneVAO)
        {
            glDeleteVertexArrays(1, &m_PlaneVAO);
            glDeleteBuffers(1, &m_PlaneVBO);
            glDeleteBuffers(1, &m_PlaneIBO);
        }
        if (m_ShaderProgram)
        {
            glDeleteProgram(m_ShaderProgram);
            m_ShaderProgram = 0;
        }
        if (m_WireframeShader)
        {
            glDeleteProgram(m_WireframeShader);
            m_WireframeShader = 0;
        }
        m_Framebuffer.reset();
    }

    void Renderer::Resize(unsigned int width, unsigned int height)
    {
        m_Framebuffer->Resize(width, height);
    }

    void Renderer::BeginFrame(const glm::mat4& view, const glm::mat4& proj)
    {
        m_View = view;
        m_Proj = proj;

        m_Framebuffer->Bind();
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.1f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Renderer::RenderScene(Scene& scene, int selectedEntityId)
    {
        auto entities = scene.GetEntities();

        glEnable(GL_DEPTH_TEST);

        for (auto id : entities)
        {
            auto* mr = scene.GetMeshRenderer(id);
            if (!mr)
                continue;

            auto& tc = scene.GetTransform(id);
            auto& mat = mr->Material;

            unsigned int vao = (mr->Mesh == MeshType::Cube) ? m_CubeVAO : m_PlaneVAO;
            unsigned int ibo = (mr->Mesh == MeshType::Cube) ? m_CubeIBO : m_PlaneIBO;
            int indexCount = (mr->Mesh == MeshType::Cube) ? 36 : 6;

            glm::mat4 model = scene.GetModelMatrix(id);

            glUseProgram(m_ShaderProgram);

            int locModel = glGetUniformLocation(m_ShaderProgram, "u_Model");
            int locView  = glGetUniformLocation(m_ShaderProgram, "u_View");
            int locProj  = glGetUniformLocation(m_ShaderProgram, "u_Proj");
            int locAlbedo   = glGetUniformLocation(m_ShaderProgram, "u_Material.Albedo");
            int locSpecular = glGetUniformLocation(m_ShaderProgram, "u_Material.Specular");
            int locShine    = glGetUniformLocation(m_ShaderProgram, "u_Material.Shininess");

            glUniformMatrix4fv(locModel, 1, GL_FALSE, &model[0][0]);
            glUniformMatrix4fv(locView, 1, GL_FALSE, &m_View[0][0]);
            glUniformMatrix4fv(locProj, 1, GL_FALSE, &m_Proj[0][0]);
            glUniform3fv(locAlbedo, 1, &mat.Albedo.x);
            glUniform3fv(locSpecular, 1, &mat.Specular.x);
            glUniform1f(locShine, mat.Shininess);

            glBindVertexArray(vao);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);

            if (selectedEntityId >= 0 && (unsigned int)selectedEntityId == id)
            {
                glEnable(GL_POLYGON_OFFSET_LINE);
                glPolygonOffset(-1.f, -1.f);
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glLineWidth(2.0f);

                glUseProgram(m_WireframeShader);
                int locWModel = glGetUniformLocation(m_WireframeShader, "u_Model");
                int locWView  = glGetUniformLocation(m_WireframeShader, "u_View");
                int locWProj  = glGetUniformLocation(m_WireframeShader, "u_Proj");
                int locColor  = glGetUniformLocation(m_WireframeShader, "u_Color");

                glm::mat4 wfModel = glm::scale(model, glm::vec3(1.01f));
                glUniformMatrix4fv(locWModel, 1, GL_FALSE, &wfModel[0][0]);
                glUniformMatrix4fv(locWView, 1, GL_FALSE, &m_View[0][0]);
                glUniformMatrix4fv(locWProj, 1, GL_FALSE, &m_Proj[0][0]);

                glm::vec3 color(1.0f, 1.0f, 0.0f);
                glUniform3fv(locColor, 1, &color.x);

                glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);

                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glDisable(GL_POLYGON_OFFSET_LINE);
                glLineWidth(1.0f);
            }

            glBindVertexArray(0);
        }

        glUseProgram(0);
    }

    void Renderer::EndFrame()
    {
        m_Framebuffer->Unbind();
    }
}

