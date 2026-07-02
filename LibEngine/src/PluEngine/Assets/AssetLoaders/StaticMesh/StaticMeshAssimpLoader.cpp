//
// Created by Plutex on 2026-02-07.
//

// StaticMeshImporter.cpp
#include "PluEngine/Assets/AssetLoaders/StaticMesh/StaticMeshAssimpLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>
#include <glm/matrix.hpp>
#include <cstring>

#include "PluEngine/Application.h"
#include "PluEngine/Timer.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/Assets/AssetLoaders/Textures/TextureImporter.h"

namespace Plu
{
	namespace
    {
        // Pakowanie normal (x,y,z) do formatu 10_10_10_2
	    UInt32 PackNormal(const Vec3& normal)
	    {
	        // Zakres dla signed 10-bit: [-512, 511]
	        auto Pack10 = [](float v) -> UInt32
	        {
	            int i = static_cast<int>(v * 511.0f);
	            if (i < -512) i = -512;
	            if (i >  511) i =  511;
	            // Rzutuj na unsigned — poprawnie zachowuje bit znaku
	            return static_cast<UInt32>(i) & 0x3FF;
	        };

	        return (Pack10(normal.x) << 0)
                 | (Pack10(normal.y) << 10)
                 | (Pack10(normal.z) << 20);
	    }

        // Pakowanie tangentu (x,y,z) + znaku bitangentu (handedness) do formatu 10_10_10_2.
        // sign musi być +1.0 lub -1.0; trafia do 2-bitowego pola w (GL_INT_2_10_10_10_REV).
	    UInt32 PackTangent(const Vec3& tangent, float sign)
	    {
	        auto Pack10 = [](float v) -> UInt32
	        {
	            int i = static_cast<int>(v * 511.0f);
	            if (i < -512) i = -512;
	            if (i >  511) i =  511;
	            return static_cast<UInt32>(i) & 0x3FF;
	        };

	        // w: +1 -> 0x1, -1 -> 0x3 (two's complement w 2 bitach)
	        UInt32 w = (sign < 0.0f) ? 0x3u : 0x1u;

	        return (Pack10(tangent.x) << 0)
                 | (Pack10(tangent.y) << 10)
                 | (Pack10(tangent.z) << 20)
                 | (w << 30);
	    }

        // Pakowanie UV do formatu 16-bit
	    UInt16 PackUV(float uv)
        {
            // Clamp do zakresu [0, 1] żeby obsłużyć wartości spoza standardowego zakresu
            float clampedUV = (uv < 0.0f) ? 0.0f : ((uv > 1.0f) ? 1.0f : uv);

            // Skaluj do zakresu [0, 65535]
            return static_cast<UInt16>(clampedUV * 65535.0f);
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

        // Most między wewnętrznym loggerem Assimpa a logami silnika — bez tego
        // ostrzeżenia parserów (FBX/glTF itp.) przepadają bezpowrotnie.
        class AssimpLogBridge final : public Assimp::LogStream
        {
        public:
            explicit AssimpLogBridge(Assimp::Logger::ErrorSeverity severity) : mSeverity(severity) {}

            void write(const char* message) override
            {
                // Assimp dokleja '\n' do każdej linii — utnij, żeby nie dublować
                std::size_t len = std::strlen(message);
                while (len > 0 && (message[len - 1] == '\n' || message[len - 1] == '\r'))
                {
                    --len;
                }

                switch (mSeverity)
                {
                    case Assimp::Logger::Err:  PLU_CORE_ERROR("[Assimp] {:.{}}", message, len); break;
                    case Assimp::Logger::Warn: PLU_CORE_WARN("[Assimp] {:.{}}", message, len);  break;
                    default:                   PLU_CORE_INFO("[Assimp] {:.{}}", message, len);  break;
                }
            }

        private:
            Assimp::Logger::ErrorSeverity mSeverity;
        };

        // Jednorazowo podpina strumienie Info/Warn/Err pod DefaultLogger Assimpa.
        // attachStream przejmuje własność wskaźników.
        void EnsureAssimpLoggerAttached()
        {
            if (!Assimp::DefaultLogger::isNullLogger())
            {
                return;
            }

            Assimp::DefaultLogger::create("", Assimp::Logger::NORMAL, 0);
            Assimp::Logger* logger = Assimp::DefaultLogger::get();
            logger->attachStream(new AssimpLogBridge(Assimp::Logger::Info), Assimp::Logger::Info);
            logger->attachStream(new AssimpLogBridge(Assimp::Logger::Warn), Assimp::Logger::Warn);
            logger->attachStream(new AssimpLogBridge(Assimp::Logger::Err),  Assimp::Logger::Err);
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

            // Macierz normalnych (inverse-transpose 3x3) — poprawne normalne/tangenty
            // także przy niejednorodnym skalowaniu w hierarchii nodów. Macierz modelu (mat3)
            // używamy dla tangenta/bitangenta (kierunki w płaszczyźnie powierzchni).
            const glm::mat3 modelMat3 = glm::mat3(transform);
            const glm::mat3 normalMatrix = glm::transpose(glm::inverse(modelMat3));

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

                // Normalna z macierzą normalnych (inverse-transpose)
                if (mesh->HasNormals())
                {
                    glm::vec3 n = glm::normalize(normalMatrix * glm::vec3(
                        mesh->mNormals[i].x,
                        mesh->mNormals[i].y,
                        mesh->mNormals[i].z
                    ));
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

                //Tangensy
                if (mesh->HasTangentsAndBitangents())
                {
                    // Tangent/bitangent transformujemy macierzą modelu (kierunki na powierzchni)
                    glm::vec3 t = glm::normalize(modelMat3 * glm::vec3(
                        mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z));
                    glm::vec3 b = glm::normalize(modelMat3 * glm::vec3(
                        mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z));

                    // Normalna macierzą normalnych — spójnie z atrybutem Normal
                    glm::vec3 n = (mesh->HasNormals())
                        ? glm::normalize(normalMatrix * glm::vec3(
                            mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z))
                        : glm::vec3(0.0f, 1.0f, 0.0f);

                    // handedness: znak bitangentu względem cross(N, T)
                    float sign = (glm::dot(glm::cross(n, t), b) < 0.0f) ? -1.0f : 1.0f;
                    vertex.Tangent = PackTangent(Vec3(t.x, t.y, t.z), sign);
                }
                else
                {
                    vertex.Tangent = PackTangent(Vec3(1, 0, 0), 1.0f);
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
                    // Po aiProcess_Triangulate nie powinno się zdarzyć; pomijamy linie/punkty/n-gony,
                    // żeby nie wsadzić do bufora indeksów ścian niebędących trójkątami.
                    PLU_CORE_WARN("Face {} has {} indices (not a triangle) — skipping!", i, face.mNumIndices);
                    continue;
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

            EnsureAssimpLoggerAttached();
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