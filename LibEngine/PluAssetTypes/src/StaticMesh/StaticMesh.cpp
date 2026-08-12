//
// Created by Plutex on 6/21/26.
//

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
