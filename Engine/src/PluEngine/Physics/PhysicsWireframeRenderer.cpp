//
// Created by Plutex on 2026-03-09.
//

#include "PluEngine/Physics/PhysicsWireframeRenderer.h"
#include <Jolt/Physics/Body/Body.h>
#include <glm/gtc/type_ptr.hpp>

using namespace Plu;

JoltWireframeRenderer::JoltWireframeRenderer()  { Init(); }
JoltWireframeRenderer::~JoltWireframeRenderer() { Cleanup(); }

void JoltWireframeRenderer::BeginFrame()
{
    m_lines.Clear();
}

void JoltWireframeRenderer::AddBody(const JPH::Body& body, const glm::vec3& color)
{
    auto tris  = ExtractTriangles(body.GetShape());
    auto world = JoltToGlm(body.GetWorldTransform());

    for (int i = 0; i < tris.Size(); i += 3)
    {
        glm::vec3 a = glm::vec3(world * glm::vec4(tris[i+0], 1.0f));
        glm::vec3 b = glm::vec3(world * glm::vec4(tris[i+1], 1.0f));
        glm::vec3 c = glm::vec3(world * glm::vec4(tris[i+2], 1.0f));

        m_lines.PushBack({ a, b, color });
        m_lines.PushBack({ b, c, color });
        m_lines.PushBack({ c, a, color });
    }
}

void JoltWireframeRenderer::Render(const glm::mat4& viewProj)
{
    if (m_lines.IsEmpty()) return;

    // Spakuj do flat bufora: pos(3) + color(3) na wierzchołek
    DynamicArray<float> buf;
    buf.Reserve(m_lines.Size() * 2 * 6);

    for (int i = 0; i < m_lines.Size(); i++)
    {
        const Line& l = m_lines[i];
        buf.PushBack(l.a.x); buf.PushBack(l.a.y); buf.PushBack(l.a.z);
        buf.PushBack(l.color.r); buf.PushBack(l.color.g); buf.PushBack(l.color.b);
        buf.PushBack(l.b.x); buf.PushBack(l.b.y); buf.PushBack(l.b.z);
        buf.PushBack(l.color.r); buf.PushBack(l.color.g); buf.PushBack(l.color.b);
    }

    glUseProgram(m_shader);
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "uViewProj"), 1, GL_FALSE, glm::value_ptr(viewProj));

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, buf.Size() * sizeof(float), buf.Data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_LINES, 0, m_lines.Size() * 2);
    glBindVertexArray(0);
}

void JoltWireframeRenderer::Init()
{
    m_shader = BuildShader();

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // aPos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    // aColor
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void JoltWireframeRenderer::Cleanup()
{
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteProgram(m_shader);
}

GLuint JoltWireframeRenderer::BuildShader()
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