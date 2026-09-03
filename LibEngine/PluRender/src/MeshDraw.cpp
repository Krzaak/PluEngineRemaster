//
// Created by Plutex on 8/12/26.
//

#include "PluEngine/Render/MeshDraw.h"

#include "glad/glad.h"

#include "PluEngine/AssetTypes/SkeletalMesh/SkeletalMesh.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/Render/RenderingManager.h"

void Plu::DrawStaticMesh(const Plu::StaticMesh *staticMesh, Plu::RenderingManager *renderingManager)
{
    glBindVertexArray(staticMesh->VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(staticMesh->IndexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    renderingManager->OnStaticMeshRender(const_cast<StaticMesh *>(staticMesh));
}

void Plu::DrawStaticMeshInstanced(const Plu::StaticMesh *staticMesh, Plu::RenderingManager *renderingManager, UInt32 instanceCount)
{
    glBindVertexArray(staticMesh->VAO);
    glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(staticMesh->IndexCount), GL_UNSIGNED_INT, nullptr, static_cast<GLsizei>(instanceCount));
    glBindVertexArray(0);
    renderingManager->OnStaticMeshRender(const_cast<StaticMesh *>(staticMesh));
}

void Plu::DrawSkeletalMesh(const Plu::SkeletalMesh* skeletalMesh, Plu::RenderingManager* renderingManager)
{
    glBindVertexArray(skeletalMesh->VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(skeletalMesh->IndexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    renderingManager->OnSkeletalMeshRender(const_cast<SkeletalMesh *>(skeletalMesh));
}
