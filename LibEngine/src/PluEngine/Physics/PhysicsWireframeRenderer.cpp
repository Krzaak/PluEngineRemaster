//
// Created by Plutex on 2026-03-09.
//

#include "PluEngine/Physics/PhysicsWireframeRenderer.h"
#include <Jolt/Physics/Body/Body.h>
#include <glm/gtc/type_ptr.hpp>

#include "EngineAssets.h"
#include "PluEngine/Managers/ShadersManager.h"

using namespace Plu;

JoltWireframeRenderer::JoltWireframeRenderer(const TUsePointer<IShaderManager> &shaderManager)
{
    mShaderManager = shaderManager;
    Init();
}

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

    if (buf.IsEmpty()) return;

    if (!mShader) {
        mShader = mShaderManager->GetShaderProgram(EngineAssets::DebugLine);
    }

    if (!mShader->IsLoaded()) {
        mShaderManager->LoadShader(mShader->Uuid);
    }

    mShader->SetMatrix4Uniform("uViewProj", viewProj);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, buf.Size() * sizeof(float), buf.Data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_LINES, 0, m_lines.Size() * 2);
    glBindVertexArray(0);
}

void JoltWireframeRenderer::Init()
{
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
}