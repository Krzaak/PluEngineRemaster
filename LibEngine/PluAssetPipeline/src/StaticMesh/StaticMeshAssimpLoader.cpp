//
// Created by Plutex on 2026-02-07.
//

// StaticMeshImporter.cpp
#include "PluEngine/AssetPipeline/StaticMesh/StaticMeshAssimpLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/matrix.hpp>
#include <cstring>

#include "PluEngine/Timer.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/AssetCore/EngineAssetManager.h"
#include "PluEngine/AssetPipeline/Mesh/MeshProcessing.h"
#include "PluEngine/AssetPipeline/Textures/TextureImporter.h"

namespace Plu
{
	namespace
    {
        // Wyciąga embedded tekstury (aiScene::mTextures) i zapisuje każdą jako osobny asset .plubin.
        void ExtractEmbeddedTextures(const aiScene* scene, const PathW& outDir, const String& modelStem,
                                     TUsePointer<EngineAssetManager> assetManager)
        {
            if (scene->mNumTextures == 0)
                return;

            PLU_CORE_INFO("Model has {} embedded texture(s)", scene->mNumTextures);

            for (UInt32 i = 0; i < scene->mNumTextures; i++)
            {
                const aiTexture* tex = scene->mTextures[i];

                String texName = modelStem + String("_tex") + String::FromInt(i);
                PathW outPath = outDir / (StringW::FromNarrow(texName.CStr()) + PLU_BINARY_EXT_W);

                bool ok = false;
                if (tex->mHeight == 0)
                {
                    // Skompresowany blob (PNG/JPG/...): mWidth = rozmiar w bajtach, pcData = dane
                    ok = TextureImport::ImportTextureFromMemory(
                        reinterpret_cast<const unsigned char*>(tex->pcData), tex->mWidth, outPath);
                }
                else
                {
                    // Surowe, nieskompresowane ARGB8888 (aiTexel) — rzadkie; na razie pomijamy
                    PLU_CORE_WARN("Embedded texture {} ('{}') is raw/uncompressed — skipping (not supported yet)",
                                  i, tex->mFilename.C_Str());
                }

                if (ok)
                {
                    assetManager->LoadAssetDescriptor(outPath.ToString().ToNarrow());
                    PLU_CORE_INFO("Extracted embedded texture {} -> {}", i,
                                  String::FromWide(outPath.CStr()).CStr());
                }
            }
        }
    }

    namespace MeshImporter
    {
        bool ImportStaticMesh(StaticMeshImportProps props, PathW import, PathW outDir, TUsePointer<EngineAssetManager> assetManager)
        {
            PLU_PROFILE_SCOPE("ImportStaticMesh");

            Path pathNarrow = import.ToString().ToNarrow();
            PLU_CORE_INFO("Importing mesh from: {}", pathNarrow.CStr());

            MeshProcessing::EnsureAssimpLoggerAttached();
            Assimp::Importer importer;

            UInt32 flags =
               aiProcess_Triangulate |          // bez tego quady/n-gony rozjeżdżają topologię indeksów
               aiProcess_JoinIdenticalVertices |
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
            const aiScene* scene;
            try {
                scene = importer.ReadFile(pathNarrow.CStr(), flags);
            } catch (...) {
                PLU_ERROR("Error importing mesh at: {}", pathNarrow.CStr());
                return false;
            }

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            {
                PLU_CORE_ERROR("Assimp Error: {}", importer.GetErrorString());
                return false;
            }

            // Wyciągnij embedded tekstury (jeśli są) jako osobne assety obok meshów
            ExtractEmbeddedTextures(scene, outDir, String::FromWide(import.GetStem().CStr()), assetManager);

            DynamicArray<MeshData> meshes;
            DynamicArray<String> meshNames;

            // Macierz identycznościowa jako początkowa transformacja
            glm::mat4 identityTransform = glm::mat4(1.0f);

            MeshProcessing::ProcessNode(scene->mRootNode, scene, meshes, props.Scale, props.FlipUVs,
                                        props.Merge, identityTransform, meshNames);

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

                assetManager->LoadAssetDescriptor(outPath.ToString().ToNarrow());
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
                    meshName.Replace(".", "_");

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

                    assetManager->LoadAssetDescriptor(outPath.ToString().ToNarrow());
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
            UInt32 version = 2;  // v2: Vertex zawiera spakowany Tangent
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
            // Vertices — zapis per-pole (jawnie), żeby format nie zależał od layoutu/paddingu Vertex
            UInt32 vertexCount = mesh->StaticMeshData.Vertices.Size();
            fwrite(&vertexCount, sizeof(UInt32), 1, file);
            for (UInt32 i = 0; i < vertexCount; i++)
            {
                const Vertex& v = mesh->StaticMeshData.Vertices[i];
                fwrite(&v.Position, sizeof(Vec3),   1, file);
                fwrite(&v.Normal,   sizeof(UInt32), 1, file);
                fwrite(v.UV,        sizeof(UInt16), 2, file);
                fwrite(&v.Color,    sizeof(UInt32), 1, file);
                fwrite(&v.Tangent,  sizeof(UInt32), 1, file); // v2: spakowany tangent 10_10_10_2
            }

            // Indices
            UInt32 indexCount = mesh->StaticMeshData.Indices.Size();
            fwrite(&indexCount, sizeof(UInt32), 1, file);
            fwrite(mesh->StaticMeshData.Indices.Data(), sizeof(UInt32), indexCount, file);

            // Material index
            fwrite(&mesh->StaticMeshData.MaterialIndex, sizeof(UInt16), 1, file);

            // Collision shapes
            UInt32 collisionCount = mesh->CollisionShapes.Size();
            fwrite(&collisionCount, sizeof(UInt32), 1, file);
            for (UInt32 i = 0; i < collisionCount; i++)
            {
                UInt8 type = static_cast<UInt8>(mesh->CollisionShapes[i].Type);
                UInt8 mode = static_cast<UInt8>(mesh->CollisionShapes[i].ApproxMode);
                fwrite(&type, sizeof(UInt8), 1, file);
                fwrite(&mode, sizeof(UInt8), 1, file);
            }

            fclose(file);
            return true;
        }

        bool LoadStaticMesh(PathW path, StaticMesh* outMesh)
        {
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

            if (magic != 0x41554C50 || version != 2)
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
            // Vertices — odczyt per-pole, symetrycznie do zapisu
            UInt32 vertexCount = 0;
            fread(&vertexCount, sizeof(UInt32), 1, file);
            outMesh->StaticMeshData.Vertices.Resize(vertexCount);
            for (UInt32 i = 0; i < vertexCount; i++)
            {
                Vertex& v = outMesh->StaticMeshData.Vertices[i];
                fread(&v.Position, sizeof(Vec3),   1, file);
                fread(&v.Normal,   sizeof(UInt32), 1, file);
                fread(v.UV,        sizeof(UInt16), 2, file);
                fread(&v.Color,    sizeof(UInt32), 1, file);
                fread(&v.Tangent,  sizeof(UInt32), 1, file); // v2: spakowany tangent 10_10_10_2
            }

            // Indices
            UInt32 indexCount = 0;
            fread(&indexCount, sizeof(UInt32), 1, file);
            outMesh->StaticMeshData.Indices.Resize(indexCount);
            fread(outMesh->StaticMeshData.Indices.Data(), sizeof(UInt32), indexCount, file);

            // Material index
            fread(&outMesh->StaticMeshData.MaterialIndex, sizeof(UInt16), 1, file);

            // Collision shapes (optional — older files without this block are handled gracefully)
            UInt32 collisionCount = 0;
            if (fread(&collisionCount, sizeof(UInt32), 1, file) == 1)
            {
                outMesh->CollisionShapes.Resize(collisionCount);
                for (UInt32 i = 0; i < collisionCount; i++)
                {
                    UInt8 type = 0, mode = 0;
                    fread(&type, sizeof(UInt8), 1, file);
                    fread(&mode, sizeof(UInt8), 1, file);
                    outMesh->CollisionShapes[i].Type  = static_cast<StaticMeshCollisionType>(type);
                    outMesh->CollisionShapes[i].ApproxMode = static_cast<ApproximateCollisionMode>(mode);
                }
            }

            fclose(file);

            // Zainicjalizuj pozostałe pola
            outMesh->IsLoaded = false;
            outMesh->VertexCount = 0;
            outMesh->VBO = 0;
            outMesh->VAO = 0;
            outMesh->EBO = 0;
            return true;
        }
    }
}