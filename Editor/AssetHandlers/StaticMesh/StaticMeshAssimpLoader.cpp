//
// Created by Plutex on 2026-02-07.
//

// StaticMeshImporter.cpp
#include "StaticMeshAssimpLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "PluEngine/PluPaths.h"

namespace Plu
{
	namespace
    {
        // Pakowanie normal (x,y,z) do formatu 10_10_10_2
        UInt32 PackNormal(const Vec3& normal)
        {
            int x = static_cast<int>(normal.x * 511.0f);
            int y = static_cast<int>(normal.y * 511.0f);
            int z = static_cast<int>(normal.z * 511.0f);

            x = (x < -511) ? -511 : ((x > 511) ? 511 : x);
            y = (y < -511) ? -511 : ((y > 511) ? 511 : y);
            z = (z < -511) ? -511 : ((z > 511) ? 511 : z);

            return ((x & 0x3FF) << 0) | ((y & 0x3FF) << 10) | ((z & 0x3FF) << 20);
        }

        // Pakowanie UV do formatu 16-bit
        UInt16 PackUV(float uv)
        {
            return static_cast<UInt16>(uv * 65535.0f);
        }

        // Pakowanie koloru RGBA do UInt32
        UInt32 PackColor(const aiColor4D& color)
        {
            UInt8 r = static_cast<UInt8>(color.r * 255.0f);
            UInt8 g = static_cast<UInt8>(color.g * 255.0f);
            UInt8 b = static_cast<UInt8>(color.b * 255.0f);
            UInt8 a = static_cast<UInt8>(color.a * 255.0f);

            return (r << 0) | (g << 8) | (b << 16) | (a << 24);
        }

        // Konwersja macierzy Assimp do GLM
        glm::mat4 AssimpToGLM(const aiMatrix4x4& mat)
        {
            return glm::mat4(
                mat.a1, mat.b1, mat.c1, mat.d1,
                mat.a2, mat.b2, mat.c2, mat.d2,
                mat.a3, mat.b3, mat.c3, mat.d3,
                mat.a4, mat.b4, mat.c4, mat.d4
            );
        }

        void ProcessMesh(aiMesh* mesh, MeshData& meshData, const StaticMeshImportProps& props,
                         const glm::mat4& transform, bool isMerging)
        {
            // W przypadku merge używamy aktualnego rozmiaru bufora jako bazy
            // W przypadku osobnych meshów zawsze zaczynamy od 0
            UInt32 baseVertexIndex = isMerging ? meshData.Vertices.Size() : 0;

            // Przetwarzaj wierzchołki
            for (UInt32 i = 0; i < mesh->mNumVertices; i++)
            {
                Vertex vertex = {};

                // Pozycja z transformacją
                glm::vec4 pos = transform * glm::vec4(
                    mesh->mVertices[i].x,
                    mesh->mVertices[i].y,
                    mesh->mVertices[i].z,
                    1.0f
                );

                vertex.Position = Vec3(
                    pos.x * props.Scale,
                    pos.y * props.Scale,
                    pos.z * props.Scale
                );

                // Normalna z transformacją (bez translacji, w = 0)
                if (mesh->HasNormals())
                {
                    glm::vec4 normal = transform * glm::vec4(
                        mesh->mNormals[i].x,
                        mesh->mNormals[i].y,
                        mesh->mNormals[i].z,
                        0.0f
                    );
                    glm::vec3 n = glm::normalize(glm::vec3(normal));
                    vertex.Normal = PackNormal(Vec3(n.x, n.y, n.z));
                }
                else
                {
                    vertex.Normal = PackNormal(Vec3(0, 1, 0));
                }

                // UV
                if (mesh->HasTextureCoords(0))
                {
                    float u = mesh->mTextureCoords[0][i].x;
                    float v = props.FlipUVs ? (1.0f - mesh->mTextureCoords[0][i].y) : mesh->mTextureCoords[0][i].y;
                    vertex.UV[0] = PackUV(u);
                    vertex.UV[1] = PackUV(v);
                }
                else
                {
                    vertex.UV[0] = 0;
                    vertex.UV[1] = 0;
                }

                // Kolor
                if (mesh->HasVertexColors(0))
                {
                    vertex.Color = PackColor(mesh->mColors[0][i]);
                }
                else
                {
                    vertex.Color = 0xFFFFFFFF; // Biały domyślnie
                }

                meshData.Vertices.PushBack(vertex);
            }

            // Przetwarzaj indeksy
            // Dla merge: dodajemy baseVertexIndex bo łączymy wiele meshów w jeden bufor
            // Dla osobnych meshów: baseVertexIndex = 0, bo każdy mesh ma własny bufor od 0
            PLU_CORE_INFO("Mesh '{}' has {} faces, primitive types: {}",
              mesh->mName.C_Str(),
              mesh->mNumFaces,
              mesh->mPrimitiveTypes);

            for (UInt32 i = 0; i < mesh->mNumFaces; i++)
            {
                aiFace face = mesh->mFaces[i];
                if (face.mNumIndices != 3)
                {
                    PLU_CORE_WARN("Face {} has {} indices (not a triangle)!", i, face.mNumIndices);
                }
                for (UInt32 j = 0; j < face.mNumIndices; j++)
                {
                    meshData.Indices.PushBack(baseVertexIndex + face.mIndices[j]);
                }
            }

            // Material index
            meshData.MaterialIndex = static_cast<UInt16>(mesh->mMaterialIndex);

            PLU_TRACE("Processed mesh: {} vertices, {} indices (base index: {})",
                      mesh->mNumVertices, mesh->mNumFaces * 3, baseVertexIndex);
        }

        void ProcessNode(aiNode* node, const aiScene* scene, DynamicArray<MeshData>& meshes,
                         const StaticMeshImportProps& props, const glm::mat4& parentTransform,
                         DynamicArray<String>& meshNames)
        {
            // Oblicz globalną transformację dla tego noda
            glm::mat4 nodeTransform = AssimpToGLM(node->mTransformation);
            glm::mat4 globalTransform = parentTransform * nodeTransform;

            // Przetwarzaj wszystkie meshe w nodzie
            for (UInt32 i = 0; i < node->mNumMeshes; i++)
            {
                aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

                if (props.Merge)
                {
                    // Jeśli merge, dodaj do pierwszego MeshData
                    if (meshes.Size() == 0)
                    {
                        meshes.PushBack(MeshData());
                        meshNames.PushBack(String("MergedMesh"));
                    }
                    ProcessMesh(mesh, meshes[0], props, globalTransform, true);
                }
                else
                {
                    // Jeśli nie merge, stwórz nowy MeshData dla każdego mesha
                    MeshData meshData;
                    ProcessMesh(mesh, meshData, props, globalTransform, false);
                    meshes.PushBack(meshData);

                    // Nazwa mesha z Assimp lub z noda
                    String meshName = mesh->mName.length > 0 ?
                        String(mesh->mName.C_Str()) :
                        String(node->mName.C_Str());

                    if (meshName.Length() == 0)
                    {
                        meshName = String("Mesh_") + String::FromInt(meshes.Size() - 1);
                    }

                    meshNames.PushBack(meshName);

                    PLU_CORE_INFO("Created separate mesh '{}': {} verts, {} indices",
                                 meshName.CStr(),
                                 meshData.Vertices.Size(),
                                 meshData.Indices.Size());
                }
            }

            // Rekurencyjnie przetwarzaj dzieci
            for (UInt32 i = 0; i < node->mNumChildren; i++)
            {
                ProcessNode(node->mChildren[i], scene, meshes, props, globalTransform, meshNames);
            }
        }
    }

    namespace MeshImporter
    {
        bool ImportStaticMesh(StaticMeshImportProps props, PathW import, PathW outDir)
        {
            PLU_CORE_INFO("Importing mesh from: {}", String::FromWide(import.CStr()).CStr());

            Assimp::Importer importer;

            UInt32 flags =
               //aiProcess_SortByPType |
               aiProcess_GenSmoothNormals |
               aiProcess_FlipWindingOrder |
               aiProcess_CalcTangentSpace;

            if (props.FlipUVs)
            {
                flags |= aiProcess_FlipUVs;
            }

            if (props.GenerateNormals)
            {
                flags |= aiProcess_GenNormals;
            }

            const aiScene* scene = importer.ReadFile(String::FromWide(import.CStr()).CStr(), flags);

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            {
                PLU_CORE_ERROR("Assimp Error: {}", importer.GetErrorString());
                return false;
            }

            DynamicArray<MeshData> meshes;
            DynamicArray<String> meshNames;

            // Macierz identycznościowa jako początkowa transformacja
            glm::mat4 identityTransform = glm::mat4(1.0f);

            ProcessNode(scene->mRootNode, scene, meshes, props, identityTransform, meshNames);

            if (meshes.Size() == 0)
            {
                PLU_CORE_ERROR("No meshes found in file!");
                return false;
            }

            // Zapisz meshe
            String fileName = String::FromWide(import.GetStem().CStr());

            if (props.Merge)
            {
                // Jeden plik z nazwą pliku .fbx
                StaticMesh staticMesh;
                staticMesh.StaticMeshData = meshes[0];
                staticMesh.IsLoaded = false;
                staticMesh.VertexCount = 0;
                staticMesh.VBO = 0;
                staticMesh.VAO = 0;
                staticMesh.EBO = 0;

                PathW outPath = outDir / (import.GetStem() + (PLU_BINARY_EXT_W));

                if (!SaveStaticMesh(outPath, &staticMesh))
                {
                    PLU_CORE_ERROR("Failed to save merged mesh!");
                    return false;
                }

                PLU_CORE_INFO("Saved merged mesh: {} ({} vertices, {} indices)",
                             String::FromWide(outPath.CStr()).CStr(),
                             meshes[0].Vertices.Size(),
                             meshes[0].Indices.Size());
            }
            else
            {
                // Wiele plików z nazwami modeli
                for (UInt32 i = 0; i < meshes.Size(); i++)
                {
                    StaticMesh staticMesh;
                    staticMesh.StaticMeshData = meshes[i];
                    staticMesh.IsLoaded = false;
                    staticMesh.VertexCount = 0;
                    staticMesh.VBO = 0;
                    staticMesh.VAO = 0;
                    staticMesh.EBO = 0;

                    // Użyj nazwy z Assimp
                    String meshName = meshNames[i];

                    // Usuń niedozwolone znaki z nazwy pliku
                    meshName.Replace("/", "_");
                    meshName.Replace("\\", "_");
                    meshName.Replace(":", "_");
                    meshName.Replace("|", "_");

                    PathW outPath = outDir / (StringW::FromNarrow(meshName.CStr()) + (PLU_BINARY_EXT_W));

                    if (!SaveStaticMesh(outPath, &staticMesh))
                    {
                        PLU_CORE_ERROR("Failed to save mesh {}!", meshName.CStr());
                        continue;
                    }

                    PLU_CORE_INFO("Saved mesh: {} ({} vertices, {} indices)",
                                 String::FromWide(outPath.CStr()).CStr(),
                                 meshes[i].Vertices.Size(),
                                 meshes[i].Indices.Size());
                }
            }

            return true;
        }

        bool SaveStaticMesh(PathW path, StaticMesh* mesh)
        {
            FILE* file = nullptr;

    #ifdef _WIN32
            _wfopen_s(&file, path.CStr(), L"wb");
    #else
            file = fopen(String::FromWide(path.CStr()).CStr(), "wb");
    #endif

            if (!file)
            {
                PLU_CORE_ERROR("Failed to open file for writing: {}", String::FromWide(path.CStr()).CStr());
                return false;
            }

            // Magic number i wersja
            UInt32 magic = 0x41554C50;  // 'PLUA'
            UInt32 version = 1;
            fwrite(&magic, sizeof(UInt32), 1, file);
            fwrite(&version, sizeof(UInt32), 1, file);

            // Typ assetu
            const char* typeName = "StaticMesh";
            UInt32 typeLength = static_cast<UInt32>(strlen(typeName));
            fwrite(&typeLength, sizeof(UInt32), 1, file);
            fwrite(typeName, sizeof(char), typeLength, file);

            UInt64 uuid = mesh->Uuid;
            fwrite(&uuid,sizeof(UInt64),1,file);

            // Zapisz MeshData
            // Vertices
            UInt32 vertexCount = mesh->StaticMeshData.Vertices.Size();
            fwrite(&vertexCount, sizeof(UInt32), 1, file);
            fwrite(mesh->StaticMeshData.Vertices.Data(), sizeof(Vertex), vertexCount, file);

            // Indices
            UInt32 indexCount = mesh->StaticMeshData.Indices.Size();
            fwrite(&indexCount, sizeof(UInt32), 1, file);
            fwrite(mesh->StaticMeshData.Indices.Data(), sizeof(UInt32), indexCount, file);

            // Material index
            fwrite(&mesh->StaticMeshData.MaterialIndex, sizeof(UInt16), 1, file);

            fclose(file);
            return true;
        }

        bool LoadStaticMesh(PathW path, StaticMesh* outMesh)
        {
            PLU_TRACE("Loading asset at: {}", String::FromWide(path.CStr()).CStr());

            FILE* file = nullptr;

    #ifdef _WIN32
            _wfopen_s(&file, path.CStr(), L"rb");
    #else
            file = fopen(String::FromWide(path.CStr()).CStr(), "rb");
    #endif

            if (!file)
            {
                PLU_CORE_ERROR("Failed to open file: {}", String::FromWide(path.CStr()).CStr());
                return false;
            }

            // Sprawdź magic number i wersję
            UInt32 magic = 0;
            UInt32 version = 0;
            fread(&magic, sizeof(UInt32), 1, file);
            fread(&version, sizeof(UInt32), 1, file);

            if (magic != 0x41554C50 || version != 1)
            {
                PLU_ERROR("File {} has invalid magic or version!", String::FromWide(path.CStr()).CStr());
                fclose(file);
                return false;
            }

            // Typ assetu
            UInt32 typeLength = 0;
            fread(&typeLength, sizeof(UInt32), 1, file);
            char* typeBuffer = new char[typeLength + 1];
            fread(typeBuffer, sizeof(char), typeLength, file);
            typeBuffer[typeLength] = '\0';

            if (strcmp(typeBuffer, "StaticMesh") != 0)
            {
                PLU_ERROR("File {} is not a StaticMesh!", String::FromWide(path.CStr()).CStr());
                delete[] typeBuffer;
                fclose(file);
                return false;
            }
            delete[] typeBuffer;

            UInt64 uuid;
            fread(&uuid, sizeof(UInt64), 1, file);
            outMesh->Uuid = uuid;

            // Wczytaj MeshData
            // Vertices
            UInt32 vertexCount = 0;
            fread(&vertexCount, sizeof(UInt32), 1, file);
            outMesh->StaticMeshData.Vertices.Resize(vertexCount);
            fread(outMesh->StaticMeshData.Vertices.Data(), sizeof(Vertex), vertexCount, file);

            // Indices
            UInt32 indexCount = 0;
            fread(&indexCount, sizeof(UInt32), 1, file);
            outMesh->StaticMeshData.Indices.Resize(indexCount);
            fread(outMesh->StaticMeshData.Indices.Data(), sizeof(UInt32), indexCount, file);

            // Material index
            fread(&outMesh->StaticMeshData.MaterialIndex, sizeof(UInt16), 1, file);

            fclose(file);

            // Zainicjalizuj pozostałe pola
            outMesh->IsLoaded = false;
            outMesh->VertexCount = 0;
            outMesh->VBO = 0;
            outMesh->VAO = 0;
            outMesh->EBO = 0;

            PLU_CORE_INFO("StaticMesh loaded successfully from: {}", String::FromWide(path.CStr()).CStr());
            return true;
        }
    }
}