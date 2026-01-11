//
// Created by Plutex on 10.01.2026.
//

#ifndef PLUENGINE_STATICMESH_H
#define PLUENGINE_STATICMESH_H
#include "PluEngine/PluTypes.h"
#include "PluEngine/Managers/AssetsManager.h"
#include "glad/glad.h"

namespace Plu
{
    struct Vertex
    {
        Vec3 Position;
        MaxUInt32 Normal;
        MaxUInt16 UV[2];
        MaxUInt32 Color;
    };
    struct MeshData
    {
        DynamicArray<Vertex> Vertices;
        DynamicArray<MaxUInt32> Indices;
        MaxUInt16 MaterialIndex;
    };
    struct StaticMesh : IAssetInfo
    {
        MeshData StaticMeshData; //This we load

        //This we do when needed
        MaxUInt16 VBO;
        MaxUInt16 VAO;
        MaxUInt16 EBO;
        MaxUInt16 VertexCount;
    };

    inline void SetupStaticMeshGL(MeshData* meshData, StaticMesh* staticMesh)
    {
        // Generuj bufory
        glGenVertexArrays(1, static_cast<GLuint *>(&staticMesh->VAO));
        glGenBuffers(1, static_cast<GLuint *>(&staticMesh->VBO));
        glGenBuffers(1, static_cast<GLuint *>(&staticMesh->EBO));

        // Binduj VAO
        glBindVertexArray(staticMesh->VAO);

        // Vertex Buffer
        glBindBuffer(GL_ARRAY_BUFFER, staticMesh->VBO);
        glBufferData(GL_ARRAY_BUFFER,
                     meshData->Vertices.Size() * sizeof(Vertex),
                     meshData->Vertices.Data(),
                     GL_STATIC_DRAW);

        // Element Buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, staticMesh->EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     meshData->Indices.Size() * sizeof(MaxUInt32),
                     meshData->Indices.Data(),
                     GL_STATIC_DRAW);

        // Vertex Attributes

        // Location 0: Position (Vec3 - 3 floats)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, Position)));

        // Location 1: Normal (MaxUInt32 - packed 10_10_10_2)
        glEnableVertexAttribArray(1);
        glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, Normal)));

        // Location 2: UV (2x MaxUInt16 - packed 16-bit)
        glEnableVertexAttribArray(2);
        glVertexAttribIPointer(2, 2, GL_UNSIGNED_SHORT, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, UV)));

        // Location 3: Color (MaxUInt32 - packed RGBA8)
        glEnableVertexAttribArray(3);
        glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, Color)));

        // Unbind
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // Zapisz vertex count
        staticMesh->VertexCount = meshData->Vertices.Size();
    }

    // Opcjonalna funkcja do czyszczenia
    inline void CleanupStaticMeshGL(StaticMesh* staticMesh)
    {
        if (staticMesh->VBO != 0)
        {
            glDeleteBuffers(1, static_cast<GLuint *>(&staticMesh->VBO));
            staticMesh->VBO = 0;
        }

        if (staticMesh->EBO != 0)
        {
            glDeleteBuffers(1, static_cast<GLuint *>(&staticMesh->EBO));
            staticMesh->EBO = 0;
        }

        if (staticMesh->VAO != 0)
        {
            glDeleteVertexArrays(1, static_cast<GLuint *>(&staticMesh->VAO));
            staticMesh->VAO = 0;
        }

        staticMesh->VertexCount = 0;
    }

    // Funkcja do renderowania
    inline void DrawStaticMesh(const StaticMesh* staticMesh)
    {
        glBindVertexArray(staticMesh->VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(staticMesh->VertexCount), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }
}

#endif //PLUENGINE_STATICMESH_H