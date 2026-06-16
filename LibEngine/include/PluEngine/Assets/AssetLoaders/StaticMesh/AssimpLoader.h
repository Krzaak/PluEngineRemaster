//
// Created by Plutex on 11.01.2026.
// Fixed version with improved error handling and validation
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
        // Pomocnicza funkcja clamp
        inline float Clamp(float value, float min, float max)
        {
            if (value < min) return min;
            if (value > max) return max;
            return value;
        }

        // Poprawione pakowanie normala - zakres [-1, 1] na 10 bitów signed
        inline UInt32 PackNormal(const Vec3& normal)
        {
            // Clamp do zakresu [-1, 1]
            float nx = Clamp(normal.x, -1.0f, 1.0f);
            float ny = Clamp(normal.y, -1.0f, 1.0f);
            float nz = Clamp(normal.z, -1.0f, 1.0f);

            // Konwersja do 10-bitowego signed (-511 do 511)
            Int32 x = static_cast<Int32>(nx * 511.0f);
            Int32 y = static_cast<Int32>(ny * 511.0f);
            Int32 z = static_cast<Int32>(nz * 511.0f);

            // Clamp do 10 bitów i maskowanie
            x = static_cast<Int32>(Clamp(static_cast<float>(x), -512.0f, 511.0f)) & 0x3FF;
            y = static_cast<Int32>(Clamp(static_cast<float>(y), -512.0f, 511.0f)) & 0x3FF;
            z = static_cast<Int32>(Clamp(static_cast<float>(z), -512.0f, 511.0f)) & 0x3FF;

            return (z << 20) | (y << 10) | x;
        }

        inline Vec3 UnpackNormal(UInt32 packed)
        {
            // Wyciągnij 10-bitowe wartości
            Int32 x = (packed & 0x3FF);
            Int32 y = ((packed >> 10) & 0x3FF);
            Int32 z = ((packed >> 20) & 0x3FF);

            // Rozszerz znak dla signed 10-bit
            if (x & 0x200) x |= 0xFFFFFC00;
            if (y & 0x200) y |= 0xFFFFFC00;
            if (z & 0x200) z |= 0xFFFFFC00;

            // Normalizuj z powrotem do [-1, 1]
            return Vec3(
                Clamp(x / 511.0f, -1.0f, 1.0f),
                Clamp(y / 511.0f, -1.0f, 1.0f),
                Clamp(z / 511.0f, -1.0f, 1.0f)
            );
        }

        // Poprawione pakowanie UV - clamp do [0, 1]
        inline UInt16 PackUV(float uv)
        {
            // Clamp do zakresu [0, 1] dla UV
            float clamped = Clamp(uv, 0.0f, 1.0f);
            return static_cast<UInt16>(clamped * 65535.0f);
        }

        inline float UnpackUV(UInt16 packed)
        {
            return packed / 65535.0f;
        }

        // Pakowanie koloru
        inline UInt32 PackColor(const Vec3& color)
        {
            UInt8 r = static_cast<UInt8>(Clamp(color.x, 0.0f, 1.0f) * 255.0f);
            UInt8 g = static_cast<UInt8>(Clamp(color.y, 0.0f, 1.0f) * 255.0f);
            UInt8 b = static_cast<UInt8>(Clamp(color.z, 0.0f, 1.0f) * 255.0f);
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

    // Konwersja pojedynczego mesha z Assimp - POPRAWIONA WERSJA
    inline bool ConvertAssimpMesh(const aiMesh* assimpMesh, EditorMeshData& meshData, UInt16 materialIndex)
    {
        if (!assimpMesh)
        {
            return false;
        }

        // Sprawdź limity
        if (assimpMesh->mNumVertices == 0 || assimpMesh->mNumVertices > UINT32_MAX)
        {
            return false;
        }

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

                // Normalizuj jeśli potrzeba
                float length = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
                if (length > 0.0001f)
                {
                    normal.x /= length;
                    normal.y /= length;
                    normal.z /= length;
                }

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

        // Konwersja indeksów z walidacją
        UInt32 vertexCount = meshData.Vertices.Size();

        for (UInt32 i = 0; i < assimpMesh->mNumFaces; ++i)
        {
            const aiFace& face = assimpMesh->mFaces[i];

            // Sprawdź czy face jest trójkątem (powinien być przez aiProcess_Triangulate)
            if (face.mNumIndices != 3)
            {
                // Pomiń non-triangle faces (nie powinno się zdarzyć)
                continue;
            }

            // Walidacja indeksów przed dodaniem
            for (UInt32 j = 0; j < 3; ++j)
            {
                if (face.mIndices[j] >= vertexCount)
                {
                    // Indeks poza zakresem!
                    return false;
                }
            }

            // Dodaj indeksy - możesz odwrócić kolejność jeśli winding jest zły
            meshData.Indices.PushBack(face.mIndices[0]);
            meshData.Indices.PushBack(face.mIndices[1]);
            meshData.Indices.PushBack(face.mIndices[2]);

            // ALTERNATYWNIE - jeśli winding jest odwrócony, użyj:
            // meshData.Indices.PushBack(face.mIndices[0]);
            // meshData.Indices.PushBack(face.mIndices[2]);
            // meshData.Indices.PushBack(face.mIndices[1]);
        }

        return true;
    }

    // Mergowanie wielu meshy w jeden - POPRAWIONA WERSJA
    inline bool MergeMeshes(const DynamicArray<EditorMeshData>& meshes, EditorMeshData& outMerged)
    {
        if (meshes.Size() == 0)
        {
            return false;
        }

        outMerged.Name = "MergedMesh";

        UInt32 totalVertices = 0;
        UInt32 totalIndices = 0;

        // Oblicz całkowitą ilość wierzchołków i indeksów
        for (const auto& mesh : meshes)
        {
            totalVertices += mesh.Vertices.Size();
            totalIndices += mesh.Indices.Size();
        }

        // Sprawdź overflow
        if (totalVertices > UINT32_MAX || totalIndices > UINT32_MAX)
        {
            return false;
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

            // Dodaj indeksy z offsetem i walidacją
            for (const auto& index : mesh.Indices)
            {
                UInt32 newIndex = index + vertexOffset;

                // Walidacja
                if (newIndex >= outMerged.Vertices.Size())
                {
                    return false;
                }

                outMerged.Indices.PushBack(newIndex);
            }

            vertexOffset += mesh.Vertices.Size();
        }

        return true;
    }

    // Zapis binarny mesha - POPRAWIONA WERSJA
    inline bool SaveMeshBinary(const PathW& filepath, const EditorMeshData& meshData)
    {
        FILE* file = nullptr;

#ifdef _WIN32
        _wfopen_s(&file, filepath.CStr(), L"wb");
#else
        file = fopen(filepath.ToString().ToNarrow().CStr(), "wb");
#endif

        if (!file)
        {
            return false;
        }

        bool success = true;

        // Magic number + wersja
        UInt32 magic = 0x41554C50; // "PLUA"
        UInt32 version = 1;
        success &= (fwrite(&magic, sizeof(UInt32), 1, file) == 1);
        success &= (fwrite(&version, sizeof(UInt32), 1, file) == 1);

        // Typ assetu
        const char* assetType = "StaticMesh";
        UInt32 typeLength = 10;
        success &= (fwrite(&typeLength, sizeof(UInt32), 1, file) == 1);
        success &= (fwrite(assetType, sizeof(char), typeLength, file) == typeLength);

        // UUID
        PluUUID uuid = PluUUID();
        UInt64 uuidInt = uuid.getUUID();
        success &= (fwrite(&uuidInt, sizeof(UInt64), 1, file) == 1);

        // Nazwa mesha
        UInt32 nameLength = meshData.Name.Length();
        success &= (fwrite(&nameLength, sizeof(UInt32), 1, file) == 1);
        success &= (fwrite(meshData.Name.CStr(), sizeof(char), nameLength, file) == nameLength);

        // Indeks materiału
        success &= (fwrite(&meshData.MaterialIndex, sizeof(UInt16), 1, file) == 1);

        // Wierzchołki
        UInt32 vertexCount = meshData.Vertices.Size();
        success &= (fwrite(&vertexCount, sizeof(UInt32), 1, file) == 1);
        success &= (fwrite(meshData.Vertices.Data(), sizeof(Vertex), vertexCount, file) == vertexCount);

        // Indeksy
        UInt32 indexCount = meshData.Indices.Size();
        success &= (fwrite(&indexCount, sizeof(UInt32), 1, file) == 1);
        success &= (fwrite(meshData.Indices.Data(), sizeof(UInt32), indexCount, file) == indexCount);

        fclose(file);
        return success;
    }

    // Odczyt binarny mesha - POPRAWIONA WERSJA
    inline bool LoadMeshBinary(const PathW& filepath, EditorMeshData& meshData)
    {
        FILE* file = nullptr;

#ifdef _WIN32
        _wfopen_s(&file, filepath.CStr(), L"rb");
#else
        file = fopen(filepath.ToString().ToNarrow().CStr(), "rb");
#endif

        if (!file)
        {
            return false;
        }

        // Sprawdź magic number i wersję
        UInt32 magic = 0;
        UInt32 version = 0;
        if (fread(&magic, sizeof(UInt32), 1, file) != 1 ||
            fread(&version, sizeof(UInt32), 1, file) != 1)
        {
            fclose(file);
            return false;
        }

        if (magic != 0x41554C50 || version != 1)
        {
            fclose(file);
            return false;
        }

        // Typ assetu
        UInt32 typeLength = 0;
        if (fread(&typeLength, sizeof(UInt32), 1, file) != 1 || typeLength > 1024)
        {
            fclose(file);
            return false;
        }

        char* typeBuffer = new char[typeLength + 1];
        if (fread(typeBuffer, sizeof(char), typeLength, file) != typeLength)
        {
            delete[] typeBuffer;
            fclose(file);
            return false;
        }
        typeBuffer[typeLength] = '\0';
        delete[] typeBuffer;

        // UUID
        UInt64 uuidInt;
        if (fread(&uuidInt, sizeof(UInt64), 1, file) != 1)
        {
            fclose(file);
            return false;
        }
        meshData.uuid = uuidInt;

        // Nazwa mesha
        UInt32 nameLength = 0;
        if (fread(&nameLength, sizeof(UInt32), 1, file) != 1 || nameLength > 1024)
        {
            fclose(file);
            return false;
        }

        char* nameBuffer = new char[nameLength + 1];
        if (fread(nameBuffer, sizeof(char), nameLength, file) != nameLength)
        {
            delete[] nameBuffer;
            fclose(file);
            return false;
        }
        nameBuffer[nameLength] = '\0';
        meshData.Name = String(nameBuffer);
        delete[] nameBuffer;

        // Indeks materiału
        if (fread(&meshData.MaterialIndex, sizeof(UInt16), 1, file) != 1)
        {
            fclose(file);
            return false;
        }

        // Wierzchołki
        UInt32 vertexCount = 0;
        if (fread(&vertexCount, sizeof(UInt32), 1, file) != 1 || vertexCount > 100000000)
        {
            fclose(file);
            return false;
        }

        meshData.Vertices.Resize(vertexCount);
        if (fread(meshData.Vertices.Data(), sizeof(Vertex), vertexCount, file) != vertexCount)
        {
            fclose(file);
            return false;
        }

        // Indeksy
        UInt32 indexCount = 0;
        if (fread(&indexCount, sizeof(UInt32), 1, file) != 1 || indexCount > 100000000)
        {
            fclose(file);
            return false;
        }

        meshData.Indices.Resize(indexCount);
        if (fread(meshData.Indices.Data(), sizeof(UInt32), indexCount, file) != indexCount)
        {
            fclose(file);
            return false;
        }

        fclose(file);
        return true;
    }

    // Funkcja debug do zapisu informacji o meshu
    inline void DebugPrintMeshInfo(const EditorMeshData& meshData, const PathW& debugPath)
    {
        FILE* debug = nullptr;

#ifdef _WIN32
        _wfopen_s(&debug, debugPath.CStr(), L"w");
#else
        debug = fopen(debugPath.ToString().ToNarrow().CStr(), "w");
#endif

        if (!debug) return;

        fprintf(debug, "=== MESH DEBUG INFO ===\n");
        fprintf(debug, "Name: %s\n", meshData.Name.CStr());
        fprintf(debug, "Vertices: %lu\n", meshData.Vertices.Size());
        fprintf(debug, "Indices: %lu\n", meshData.Indices.Size());
        fprintf(debug, "Triangles: %lu\n", meshData.Indices.Size() / 3);
        fprintf(debug, "Material Index: %u\n\n", meshData.MaterialIndex);

        // Wypisz pierwsze 10 trójkątów
        fprintf(debug, "=== FIRST 10 TRIANGLES ===\n");
        UInt32 trianglesToPrint = meshData.Indices.Size() / 3;
        if (trianglesToPrint > 10) trianglesToPrint = 10;

        for (UInt32 i = 0; i < trianglesToPrint; ++i)
        {
            UInt32 idx0 = meshData.Indices[i * 3 + 0];
            UInt32 idx1 = meshData.Indices[i * 3 + 1];
            UInt32 idx2 = meshData.Indices[i * 3 + 2];

            fprintf(debug, "Triangle %u: indices [%u, %u, %u]\n", i, idx0, idx1, idx2);

            if (idx0 < meshData.Vertices.Size() &&
                idx1 < meshData.Vertices.Size() &&
                idx2 < meshData.Vertices.Size())
            {
                const Vertex& v0 = meshData.Vertices[idx0];
                const Vertex& v1 = meshData.Vertices[idx1];
                const Vertex& v2 = meshData.Vertices[idx2];

                fprintf(debug, "  v0: pos(%.3f, %.3f, %.3f)\n", v0.Position.x, v0.Position.y, v0.Position.z);
                fprintf(debug, "  v1: pos(%.3f, %.3f, %.3f)\n", v1.Position.x, v1.Position.y, v1.Position.z);
                fprintf(debug, "  v2: pos(%.3f, %.3f, %.3f)\n", v2.Position.x, v2.Position.y, v2.Position.z);
            }
            else
            {
                fprintf(debug, "  ERROR: Invalid indices!\n");
            }
        }

        // Sprawdź czy są złe indeksy
        fprintf(debug, "\n=== INDEX VALIDATION ===\n");
        bool hasInvalidIndices = false;
        for (UInt32 i = 0; i < meshData.Indices.Size(); ++i)
        {
            if (meshData.Indices[i] >= meshData.Vertices.Size())
            {
                fprintf(debug, "ERROR: Index %u = %u is out of range (max: %u)\n",
                    i, meshData.Indices[i], meshData.Vertices.Size() - 1);
                hasInvalidIndices = true;
            }
        }

        if (!hasInvalidIndices)
        {
            fprintf(debug, "All indices are valid!\n");
        }

        fclose(debug);
    }

    // Główna funkcja importu - POPRAWIONA WERSJA
    inline bool ImportAssetStaticMeshAssimp(const PathW& originPath, const PathW& loadToDirectory,
        const MeshImportOptions& options = MeshImportOptions(),
        DynamicArray<PathW>* importedAssets = nullptr)
    {
        Assimp::Importer importer;

        // Flagi importu - usunięto aiProcess_JoinIdenticalVertices który może powodować problemy
        unsigned int flags =
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_ImproveCacheLocality |
            aiProcess_CalcTangentSpace |
            aiProcess_ValidateDataStructure |
            aiProcess_FindDegenerates |           // Usuń zdegenerowane trójkąty
            aiProcess_SortByPType;                 // Sortuj po typie primitywu

        // Opcjonalnie: odwróć winding order jeśli potrzebujesz
        // flags |= aiProcess_FlipWindingOrder;

        // Import sceny
        String originPathUtf8 = originPath.ToString().ToNarrow();
        const aiScene* scene = importer.ReadFile(originPathUtf8.CStr(), flags);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            // Błąd importu
            return false;
        }

        if (scene->mNumMeshes == 0)
        {
            return false;
        }

        DynamicArray<EditorMeshData> meshes;
        meshes.Reserve(scene->mNumMeshes);

        // Konwertuj wszystkie meshe
        for (UInt32 i = 0; i < scene->mNumMeshes; ++i)
        {
            EditorMeshData meshData;
            meshData.Name = String(scene->mMeshes[i]->mName.C_Str());

            if (meshData.Name.IsEmpty())
            {
                meshData.Name = String("Mesh_") + String::FromInt(i);
            }

            // Konwersja z walidacją
            if (!ConvertAssimpMesh(scene->mMeshes[i], meshData, scene->mMeshes[i]->mMaterialIndex))
            {
                // Pomiń mesh z błędami
                continue;
            }

            meshes.PushBack(meshData);
        }

        if (meshes.Size() == 0)
        {
            return false;
        }

        // Utwórz nazwę pliku z oryginalnej ścieżki
        StringW baseFileName = originPath.GetStem().CStr();

        // Merguj lub zapisz osobno
        if (options.MergeMeshes && meshes.Size() > 1)
        {
            EditorMeshData mergedMesh;
            if (!MergeMeshes(meshes, mergedMesh))
            {
                return false;
            }

            PathW outputPath = loadToDirectory.ToString() + L"/" +
                PathW(baseFileName + StringW(PLU_BINARY_EXT_W)).ToString();

            // Debug info
            PathW debugPath = loadToDirectory.ToString() + L"/" +
                PathW(baseFileName + StringW(L"_debug.txt")).ToString();
            DebugPrintMeshInfo(mergedMesh, debugPath);

            if (importedAssets)
            {
                importedAssets->Clear();
                importedAssets->PushBack(outputPath);
            }

            return SaveMeshBinary(outputPath, mergedMesh);
        }
        else if (meshes.Size() == 1)
        {
            PathW outputPath = loadToDirectory.ToString() + L"/" +
                PathW(baseFileName + StringW(PLU_BINARY_EXT_W)).ToString();

            // Debug info
            PathW debugPath = loadToDirectory.ToString() + L"/" +
                PathW(baseFileName + StringW(L"_debug.txt")).ToString();
            DebugPrintMeshInfo(meshes[0], debugPath);

            if (importedAssets)
            {
                importedAssets->Clear();
                importedAssets->PushBack(outputPath);
            }

            return SaveMeshBinary(outputPath, meshes[0]);
        }
        else if (meshes.Size() > 1)
        {
            // Zapisz każdy mesh osobno
            bool success = true;
            if (importedAssets)
            {
                importedAssets->Clear();
            }

            for (UInt32 i = 0; i < meshes.Size(); ++i)
            {
                PathW meshPath = loadToDirectory.ToString() + L"/" +
                    PathW(baseFileName + StringW(L"_") + StringW::FromInt(i) + StringW(PLU_BINARY_EXT_W)).ToString();

                // Debug info
                PathW debugPath = loadToDirectory.ToString() + L"/" +
                    PathW(baseFileName + StringW(L"_") + StringW::FromInt(i) + StringW(L"_debug.txt")).ToString();
                DebugPrintMeshInfo(meshes[i], debugPath);

                if (SaveMeshBinary(meshPath, meshes[i]))
                {
                    if (importedAssets)
                    {
                        importedAssets->PushBack(meshPath);
                    }
                }
                else
                {
                    success = false;
                }
            }

            return success;
        }

        return false;
    }
}

#endif //PLUENGINE_ASSIMPLOADER_H