//
// Created by Plutex on 2026-03-09.
//

#include "PluEngine/Physics/PhysicsPointRenderer.h"
#include <Jolt/Physics/Body/Body.h>
#include <glm/gtc/type_ptr.hpp>

using namespace Plu;

JoltPointRenderer::JoltPointRenderer()  { Init(); }
JoltPointRenderer::~JoltPointRenderer() { Cleanup(); }

void JoltPointRenderer::BeginFrame()
{
    m_points.Clear();
}

void JoltPointRenderer::AddBody(const JPH::Body& body, const glm::vec3& color)
{
    auto tris  = ExtractTriangles(body.GetShape());
    auto world = JoltToGlm(body.GetWorldTransform());

    for (int i = 0; i < tris.Size(); i++)
    {
        glm::vec3 p = glm::vec3(world * glm::vec4(tris[i], 1.0f));
        m_points.PushBack({ p, color });
    }
}

void JoltPointRenderer::Render(const glm::mat4& viewProj, float pointSize)
{
    if (m_points.IsEmpty()) return;

    DynamicArray<float> buf;
    buf.Reserve(m_points.Size() * 6);

    for (int i = 0; i < m_points.Size(); i++)
    {
        const Point& p = m_points[i];
        buf.PushBack(p.pos.x);   buf.PushBack(p.pos.y);   buf.PushBack(p.pos.z);
        buf.PushBack(p.color.r); buf.PushBack(p.color.g); buf.PushBack(p.color.b);
    }

    glPointSize(pointSize);
    glUseProgram(m_shader);
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "uViewProj"), 1, GL_FALSE, glm::value_ptr(viewProj));

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, buf.Size() * sizeof(float), buf.Data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_POINTS, 0, m_points.Size());
    glBindVertexArray(0);
    glPointSize(1.0f);
}

void JoltPointRenderer::Init()
{
    m_shader = BuildShader();

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void JoltPointRenderer::Cleanup()
{
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteProgram(m_shader);
}

GLuint JoltPointRenderer::BuildShader()
{
    const char* vert = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aColor;
        uniform mat4 uViewProj;
        out vec3 vColor;
        void main()
        {
            vColor = aColor;
            gl_Position = uViewProj * vec4(aPos, 1.0);
        }
    )glsl";

    const char* frag = R"glsl(
        #version 330 core
        in vec3 vColor;
        out vec4 FragColor;
        void main() { FragColor = vec4(vColor, 1.0); }
    )glsl";

    auto compile = [](GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        return s;
    };

    GLuint vs = compile(GL_VERTEX_SHADER, vert);
    GLuint fs = compile(GL_FRAGMENT_SHADER, frag);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}