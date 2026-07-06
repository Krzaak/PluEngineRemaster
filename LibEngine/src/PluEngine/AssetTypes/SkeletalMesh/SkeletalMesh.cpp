//
// Created by Plutex on 7/6/26.
//

#include "PluEngine/AssetTypes/SkeletalMesh/SkeletalMesh.h"

#include "PluEngine/AssetTypes/Skeleton/Skeleton.h"
#include "PluEngine/Managers/RenderingManager.h"

void Plu::SetupSkeletalMeshGL(Plu::SkeletalMeshData* meshData, Plu::SkeletalMesh* skeletalMesh)
{
    // Generuj bufory
    glGenVertexArrays(1, &skeletalMesh->VAO);
    glGenBuffers(1, &skeletalMesh->VBO);
    glGenBuffers(1, &skeletalMesh->EBO);

    // Binduj VAO
    glBindVertexArray(skeletalMesh->VAO);

    // Vertex Buffer
    glBindBuffer(GL_ARRAY_BUFFER, skeletalMesh->VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 meshData->Vertices.Size() * sizeof(SkeletalVertex),
                 meshData->Vertices.Data(),
                 GL_STATIC_DRAW);

    // Element Buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skeletalMesh->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 meshData->Indices.Size() * sizeof(UInt32),
                 meshData->Indices.Data(),
                 GL_STATIC_DRAW);

    // Vertex Attributes — pierwsze pięć jak w StaticMesh (SkeletalVertex dziedziczy po Vertex),
    // plus dwa dodatkowe dla skinningu.

    // Location 0: Position (Vec3 - 3 floats)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkeletalVertex), reinterpret_cast<void*>(offsetof(SkeletalVertex, Position)));

    // Location 1: Normal (spakowany 10_10_10_2)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_INT_2_10_10_10_REV, GL_TRUE, sizeof(SkeletalVertex), reinterpret_cast<void*>(offsetof(SkeletalVertex, Normal)));

    // Location 2: UV (2x UInt16 - packed 16-bit)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_UNSIGNED_SHORT, GL_TRUE, sizeof(SkeletalVertex), reinterpret_cast<void*>(offsetof(SkeletalVertex, UV)));

    // Location 3: Color (packed RGBA8)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(SkeletalVertex), reinterpret_cast<void*>(offsetof(SkeletalVertex, Color)));

    // Location 4: Tangent (packed 10_10_10_2 — xyz tangent, w = handedness)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_INT_2_10_10_10_REV, GL_TRUE, sizeof(SkeletalVertex), reinterpret_cast<void*>(offsetof(SkeletalVertex, Tangent)));

    // Location 5: BoneIndices (4x UInt32) — atrybut całkowitoliczbowy (glVertexAttribIPointer),
    // żeby indeksy palety kości trafiły do shadera jako ivec4 bez konwersji na float.
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, kMaxBoneInfluence, GL_UNSIGNED_INT, sizeof(SkeletalVertex), reinterpret_cast<void*>(offsetof(SkeletalVertex, BoneIndices)));

    // Location 6: BoneWeights (4x float)
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, kMaxBoneInfluence, GL_FLOAT, GL_FALSE, sizeof(SkeletalVertex), reinterpret_cast<void*>(offsetof(SkeletalVertex, BoneWeights)));

    // Unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Zapisz licznik wierzchołków i indeksów (draw używa IndexCount)
    skeletalMesh->VertexCount = meshData->Vertices.Size();
    skeletalMesh->IndexCount = meshData->Indices.Size();
    skeletalMesh->IsLoaded = true;
}

void Plu::CleanupSkeletalMeshGL(Plu::SkeletalMesh* skeletalMesh)
{
    if (skeletalMesh->VBO != 0)
    {
        glDeleteBuffers(1, static_cast<GLuint*>(&skeletalMesh->VBO));
        skeletalMesh->VBO = 0;
    }

    if (skeletalMesh->EBO != 0)
    {
        glDeleteBuffers(1, static_cast<GLuint*>(&skeletalMesh->EBO));
        skeletalMesh->EBO = 0;
    }

    if (skeletalMesh->VAO != 0)
    {
        glDeleteVertexArrays(1, static_cast<GLuint*>(&skeletalMesh->VAO));
        skeletalMesh->VAO = 0;
    }

    skeletalMesh->VertexCount = 0;
    skeletalMesh->IndexCount = 0;
    skeletalMesh->IsLoaded = false;
}

void Plu::DrawSkeletalMesh(const Plu::SkeletalMesh* skeletalMesh, Plu::RenderingManager* renderingManager)
{
    glBindVertexArray(skeletalMesh->VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(skeletalMesh->IndexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    renderingManager->OnSkeletalMeshRender(const_cast<SkeletalMesh *>(skeletalMesh));
}
