//
// Created by Plutex on 2026-03-09.
//

#include "PluEngine/Physics/PhysicsPointRenderer.h"
#include <Jolt/Physics/Body/Body.h>
#include <glm/gtc/type_ptr.hpp>

#include "EngineAssets.h"
#include "PluEngine/Managers/ShadersManager.h"
#include "PluEngine/Shaders/ShaderProgram.h"

using namespace Plu;

JoltPointRenderer::JoltPointRenderer(const TUsePointer<IShaderManager> &shaderManager)
{
    mShaderManager = shaderManager;
    Init();
}

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

    if (buf.IsEmpty()) return;

    if (!mShader) {
        mShader = mShaderManager->GetShaderProgram(EngineAssets::DebugLine);
    }

    if (!mShader->IsLoaded()) {
        mShaderManager->LoadShader(mShader->Uuid);
    }

    mShader->SetMatrix4Uniform("uViewProj", viewProj);

    glPointSize(pointSize);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, buf.Size() * sizeof(float), buf.Data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_POINTS, 0, m_points.Size());
    glBindVertexArray(0);
    glPointSize(1.0f);
}

void JoltPointRenderer::Init()
{

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
}