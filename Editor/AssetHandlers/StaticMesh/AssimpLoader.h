//
// Created by Plutex on 11.01.2026.
//

#ifndef PLUENGINE_ASSIMPLOADER_H
#define PLUENGINE_ASSIMPLOADER_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "StaticMeshAssetImporter.h"
#include "PluEngine/Core.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/PluTypes.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"

namespace Plu
{
    // Pomocnicze funkcje konwersji
    namespace MeshImportHelpers
    {
        inline UInt32 PackNormal(const Vec3& normal)
        {
            // Pakowanie normala do formatu 10_10_10_2
            Int32 x = static_cast<Int32>(normal.x * 511.0f);
            Int32 y = static_cast<Int32>(normal.y * 511.0f);
            Int32 z = static_cast<Int32>(normal.z * 511.0f);

            x = (x & 0x3FF);
            y = (y & 0x3FF);
            z = (z & 0x3FF);

            return (z << 20) | (y << 10) | x;
        }

        inline Vec3 UnpackNormal(UInt32 packed)
        {
            Int32 x = (packed & 0x3FF);
            Int32 y = ((packed >> 10) & 0x3FF);
            Int32 z = ((packed >> 20) & 0x3FF);

            if (x & 0x200) x |= 0xFFFFFC00;
            if (y & 0x200) y |= 0xFFFFFC00;
            if (z & 0x200) z |= 0xFFFFFC00;

            return Vec3(x / 511.0f, y / 511.0f, z / 511.0f);
        }

        inline UInt16 PackUV(float uv)
        {
            return static_cast<UInt16>(uv * 65535.0f);
        }

        inline float UnpackUV(UInt16 packed)
        {
            return packed / 65535.0f;
        }

        inline UInt32 PackColor(const Vec3& color)
        {
            UInt8 r = static_cast<UInt8>(color.x * 255.0f);
            UInt8 g = static_cast<UInt8>(color.y * 255.0f);
            UInt8 b = static_cast<UInt8>(color.z * 255.0f);
            UInt8 a = 255;

            return (a << 24) | (b << 16) | (g << 8) | r;
        }

        inline Vec3 UnpackColor(UInt32 packed)
        {
            UInt8 r = packed & 0xFF;
            UInt8 g = (packed >> 8) & 0xFF;
            UInt8 b = (packed >> 16) & 0xFF;

            return Vec3(r / 255.0f, g / 255.0f, b / 255.0f);
        }
    }

    // Konwersja pojedynczego mesha z Assimp
    inline void ConvertAssimpMesh(const aiMesh* assimpMesh, EditorMeshData& meshData, UInt16 materialIndex)
    {
        meshData.MaterialIndex = materialIndex;
        meshData.Vertices.Reserve(assimpMesh->mNumVertices);

        // Konwersja wierzchołków
        for (UInt32 i = 0; i < assimpMesh->mNumVertices; ++i)
        {
            Vertex vertex = {};

            // Pozycja
            vertex.Position = Vec3(
                assimpMesh->mVertices[i].x,
                assimpMesh->mVertices[i].y,
                assimpMesh->mVertices[i].z
            );

            // Normal
            if (assimpMesh->HasNormals())
            {
                Vec3 normal(
                    assimpMesh->mNormals[i].x,
                    assimpMesh->mNormals[i].y,
                    assimpMesh->mNormals[i].z
                );
                vertex.Normal = MeshImportHelpers::PackNormal(normal);
            }
            else
            {
                vertex.Normal = MeshImportHelpers::PackNormal(Vec3(0, 1, 0));
            }

            // UV
            if (assimpMesh->HasTextureCoords(0))
            {
                vertex.UV[0] = MeshImportHelpers::PackUV(assimpMesh->mTextureCoords[0][i].x);
                vertex.UV[1] = MeshImportHelpers::PackUV(assimpMesh->mTextureCoords[0][i].y);
            }
            else
            {
                vertex.UV[0] = 0;
                vertex.UV[1] = 0;
            }

            // Kolor
            if (assimpMesh->HasVertexColors(0))
            {
                Vec3 color(
                    assimpMesh->mColors[0][i].r,
                    assimpMesh->mColors[0][i].g,
                    assimpMesh->mColors[0][i].b
                );
                vertex.Color = MeshImportHelpers::PackColor(color);
            }
            else
            {
                vertex.Color = 0xFFFFFFFF; // Biały
            }

            meshData.Vertices.PushBack(vertex);
        }

        // Konwersja indeksów
        for (UInt32 i = 0; i < assimpMesh->mNumFaces; ++i)
        {
            const aiFace& face = assimpMesh->mFaces[i];
            for (UInt32 j = 0; j < face.mNumIndices; ++j)
            {
                meshData.Indices.PushBack(face.mIndices[j]);
            }
        }
    }

    // Mergowanie wielu meshy w jeden
    inline void MergeMeshes(const DynamicArray<EditorMeshData>& meshes, EditorMeshData& outMerged)
    {
        if (meshes.Size() == 0) return;

        outMerged.Name = "MergedMesh";

        UInt32 totalVertices = 0;
        UInt32 totalIndices = 0;

        // Oblicz całkowitą ilość wierzchołków i indeksów
        for (const auto& mesh : meshes)
        {
            totalVertices += mesh.Vertices.Size();
            totalIndices += mesh.Indices.Size();
        }

        outMerged.Vertices.Reserve(totalVertices);
        outMerged.Indices.Reserve(totalIndices);
        outMerged.MaterialIndex = meshes[0].MaterialIndex;

        UInt32 vertexOffset = 0;

        // Merguj wszystkie meshe
        for (const auto& mesh : meshes)
        {
            // Dodaj wierzchołki
            for (const auto& vertex : mesh.Vertices)
            {
                outMerged.Vertices.PushBack(vertex);
            }

            // Dodaj indeksy z offsetem
            for (const auto& index : mesh.Indices)
            {
                outMerged.Indices.PushBack(index + vertexOffset);
            }

            vertexOffset += mesh.Vertices.Size();
        }
    }

    // Zapis binarny mesha
    inline bool SaveMeshBinary(const PathW& filepath, const EditorMeshData& meshData)
    {
        FILE* file = nullptr;

#ifdef _WIN32
        _wfopen_s(&file, filepath.CStr(), L"wb");
#else
        file = fopen(filepath.ToString().ToNarrow().CStr(), "wb");
#endif

        if (!file) return false;

        // Magic number + wersja
        UInt32 magic = 0x41554C50; // "PLUA"
        UInt32 version = 1;
        fwrite(&magic, sizeof(UInt32), 1, file);
        fwrite(&version, sizeof(UInt32), 1, file);

        // Typ assetu
        const char* assetType = "StaticMesh";
        UInt32 typeLength = 10; // długość "StaticMesh"
        fwrite(&typeLength, sizeof(UInt32), 1, file);
        fwrite(assetType, sizeof(char), typeLength, file);

        //uuid
        PluUUID uuid = PluUUID();
        UInt64 uuidInt = uuid.getUUID();
        fwrite(&uuidInt, sizeof(UInt64), 1 ,file);

        // Nazwa mesha
        UInt32 nameLength = meshData.Name.Length();
        fwrite(&nameLength, sizeof(UInt32), 1, file);
        fwrite(meshData.Name.CStr(), sizeof(char), nameLength, file);

        // Indeks materiału
        fwrite(&meshData.MaterialIndex, sizeof(UInt16), 1, file);

        // Wierzchołki
        UInt32 vertexCount = meshData.Vertices.Size();
        fwrite(&vertexCount, sizeof(UInt32), 1, file);
        fwrite(meshData.Vertices.Data(), sizeof(Vertex), vertexCount, file);

        // Indeksy
        UInt32 indexCount = meshData.Indices.Size();
        fwrite(&indexCount, sizeof(UInt32), 1, file);
        fwrite(meshData.Indices.Data(), sizeof(UInt32), indexCount, file);

        fclose(file);
        return true;
    }

    // Odczyt binarny mesha
    inline bool LoadMeshBinary(const PathW& filepath, EditorMeshData& meshData)
    {
        FILE* file = nullptr;

#ifdef _WIN32
        _wfopen_s(&file, filepath.CStr(), L"rb");
#else
        file = fopen(filepath.ToString().ToNarrow().CStr(), "rb");
#endif

        if (!file) return false;

        // Sprawdź magic number i wersję
        UInt32 magic = 0;
        UInt32 version = 0;
        fread(&magic, sizeof(UInt32), 1, file);
        fread(&version, sizeof(UInt32), 1, file);

        if (magic != 0x41554C50 || version != 1)
        {
            fclose(file);
            return false;
        }

        // Typ assetu
        UInt32 typeLength = 0;
        fread(&typeLength, sizeof(UInt32), 1, file);
        char* typeBuffer = new char[typeLength + 1];
        fread(typeBuffer, sizeof(char), typeLength, file);
        typeBuffer[typeLength] = '\0';
        // Opcjonalnie możesz sprawdzić typ: if (strcmp(typeBuffer, "StaticMesh") != 0) { ... }
        delete[] typeBuffer;

        //uuid
        UInt64 uuidInt;
        fread(&uuidInt, sizeof(UInt64), 1, file);
        meshData.uuid = uuidInt;

        // Nazwa mesha
        UInt32 nameLength = 0;
        fread(&nameLength, sizeof(UInt32), 1, file);
        char* nameBuffer = new char[nameLength + 1];
        fread(nameBuffer, sizeof(char), nameLength, file);
        nameBuffer[nameLength] = '\0';
        meshData.Name = String(nameBuffer);
        delete[] nameBuffer;

        // Indeks materiału
        fread(&meshData.MaterialIndex, sizeof(UInt16), 1, file);

        // Wierzchołki
        UInt32 vertexCount = 0;
        fread(&vertexCount, sizeof(UInt32), 1, file);
        meshData.Vertices.Resize(vertexCount);
        fread(meshData.Vertices.Data(), sizeof(Vertex), vertexCount, file);

        // Indeksy
        UInt32 indexCount = 0;
        fread(&indexCount, sizeof(UInt32), 1, file);
        meshData.Indices.Resize(indexCount);
        fread(meshData.Indices.Data(), sizeof(UInt32), indexCount, file);

        fclose(file);
        return true;
    }

    // Główna funkcja importu
    inline bool ImportAssetStaticMeshAssimp(const PathW& originPath, const PathW& loadToDirectory, const MeshImportOptions& options = MeshImportOptions(),
        DynamicArray<PathW>* importedAssets = nullptr)
    {
        Assimp::Importer importer;

        // Flagi importu
        unsigned int flags =
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ImproveCacheLocality |
            aiProcess_OptimizeMeshes |
            aiProcess_ValidateDataStructure |
            aiProcess_OptimizeGraph;

        // Import sceny
        String originPathUtf8 = originPath.ToString().ToNarrow();
        const aiScene* scene = importer.ReadFile(originPathUtf8.CStr(), flags);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            return false;
        }

        DynamicArray<EditorMeshData> meshes;

        // Konwertuj wszystkie meshe
        for (UInt32 i = 0; i < scene->mNumMeshes; ++i)
        {
            EditorMeshData meshData;
            meshData.Name = String(scene->mMeshes[i]->mName.C_Str());

            if (meshData.Name.IsEmpty())
            {
                meshData.Name = String("Mesh_") + String::FromInt(i);
            }

            ConvertAssimpMesh(scene->mMeshes[i], meshData, scene->mMeshes[i]->mMaterialIndex);
            meshes.PushBack(meshData);
        }

        // Utwórz nazwę pliku z oryginalnej ścieżki
        StringW baseFileName = originPath.GetStem().CStr();

        // Merguj lub zapisz osobno
        if (options.MergeMeshes && meshes.Size() > 1)
        {
            EditorMeshData mergedMesh;
            MergeMeshes(meshes, mergedMesh);
            PathW outputPath = loadToDirectory.ToString() + L"/" + PathW(baseFileName + StringW(PLU_BINARY_EXT_W)).ToString();
            if (importedAssets) {
                importedAssets->Clear();
                importedAssets->PushBack(outputPath);
            }
            return SaveMeshBinary(outputPath, mergedMesh);
        }
        else if (meshes.Size() == 1)
        {
            PathW outputPath = loadToDirectory.ToString() + L"/" + PathW(baseFileName + StringW(PLU_BINARY_EXT_W)).ToString();
            if (importedAssets) {
                importedAssets->Clear();
                importedAssets->PushBack(outputPath);
            }
            return SaveMeshBinary(outputPath, meshes[0]);
        }
        else if (meshes.Size() > 1)
        {
            // Zapisz każdy mesh osobno
            bool success = true;
            if (importedAssets) {
                importedAssets->Clear();
            }
            for (UInt32 i = 0; i < meshes.Size(); ++i)
            {
                PathW meshPath = loadToDirectory.ToString() + L"/" +
                    PathW(baseFileName + StringW(L"_") + StringW::FromInt(i) + StringW(PLU_BINARY_EXT_W)).ToString();

                success &= SaveMeshBinary(meshPath, meshes[i]);
                if (success) {
                    if (importedAssets) {
                        importedAssets->PushBack(meshPath);
                    }
                }
            }
            return success;
        }

        return false;
    }
}

#endif //PLUENGINE_ASSIMPLOADER_H