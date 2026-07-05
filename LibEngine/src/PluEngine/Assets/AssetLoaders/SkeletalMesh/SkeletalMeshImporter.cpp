//
// Created by Plutex on 7/5/26.
//

#include "PluEngine/Assets/AssetLoaders/SkeletalMesh/SkeletalMeshImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "PluEngine/PluPaths.h"
#include "PluEngine/Assets/AssetLoaders/SkeletalMesh/SkeletalMeshAssetLoader.h"
#include "PluEngine/AssetTypes/Skeleton/Skeleton.h"
#include "PluEngine/Assets/AssetDescriptor.h"
#include "PluEngine/Managers/DiskManager.h"

namespace Plu
{
    glm::mat4 AssimpToGLM(const aiMatrix4x4& mat)
    {
        return glm::mat4(
            mat.a1, mat.b1, mat.c1, mat.d1,
            mat.a2, mat.b2, mat.c2, mat.d2,
            mat.a3, mat.b3, mat.c3, mat.d3,
            mat.a4, mat.b4, mat.c4, mat.d4
        );
    }

    // void ImportSkeletons(aiScene* scene, TUsePointer<EngineAssetManager> assetManager, DynamicArray<Skeleton>* skeletons)
    // {
    //     std::function<SkeletonBone(aiSkeletonBone*)> makeBoneFromAssimp = [](aiSkeletonBone* skeletonBone) -> SkeletonBone {
    //         SkeletonBone bone;
    //         bone.NodeName = skeletonBone->mNode->mName.C_Str();
    //         bone.LocalMatrix = AssimpToGLM(skeletonBone->mLocalMatrix);
    //         bone.OffsetMatrix = AssimpToGLM(skeletonBone->mOffsetMatrix);
    //         return bone;
    //     };
    //
    //     for (int i = 0; i < scene->mNumSkeletons; ++i) {
    //         aiSkeleton* skeletonAssimp = scene->mSkeletons[i];
    //
    //         Skeleton skeleton;
    //         skeleton.SkeletonName = skeletonAssimp->mName.C_Str();
    //
    //         GameHashMap<int, DynamicArray<unsigned int>> childrenMap;
    //
    //         std::function<void(int, SkeletonBone*)> makeBoneHierarchy = [&](int boneIndex, SkeletonBone* parent) {
    //             SkeletonBone bone = makeBoneFromAssimp(skeletonAssimp->mBones[boneIndex]);
    //             parent->Children.PushBack(bone);
    //             if (!childrenMap.Contains(boneIndex)) return;
    //             for (int child : *childrenMap.Find(boneIndex)) {
    //                 makeBoneHierarchy(child, &bone);
    //             }
    //         };
    //
    //         for (int j = 0; j < skeletonAssimp->mNumBones; ++j) {
    //             aiSkeletonBone* bone = skeletonAssimp->mBones[j];
    //             childrenMap[skeletonAssimp->mBones[i]->mParent].PushBack(i);
    //             if (bone->mParent == -1) {
    //                 skeleton.RootNode = makeBoneFromAssimp(bone);
    //             }
    //         }
    //
    //         for (int idx : childrenMap[-1]) {
    //             makeBoneHierarchy(idx, &skeleton.RootBone);
    //         }
    //         skeletons->PushBack(skeleton);
    //     }
    //     PLU_CORE_WARN("Skeletons imported with new Assimp API is {}", skeletons->Size());
    // } TODO wait for future

    void ImportSkeletonsLegacy(aiScene* scene, TUsePointer<EngineAssetManager> assetManager, DynamicArray<Skeleton>* skeletons)
    {
        for (int i = 0; i < scene->mNumMeshes; ++i) {
            aiMesh* mesh = scene->mMeshes[i];
            if (!mesh->HasBones()) continue;
            aiNode* root;
            std::function<void(aiNode*)> findRoot = [&](aiNode* bone) {
                if (bone->mParent->mName == aiString("RootNode")) {
                    root = bone;
                } else {
                    findRoot(bone->mParent);
                }
            };
            GameHashMap<String, aiBone*> bones;
            findRoot(mesh->mBones[0]->mNode);
            for (int j = 0; j < mesh->mNumBones; ++j) {
                aiBone* bone = mesh->mBones[j];
                aiNode* rootBefore = root;
                findRoot(bone->mNode);
                if (root != rootBefore) {
                    PLU_CORE_ERROR("Meshes with multiple root bones are not supported!");
                }
                bones.Insert(bone->mNode->mName.C_Str(), bone);
            }

            std::function<TOwningPointer<SkeletonNode>(aiNode*)> constructSkeleton = [&](aiNode* node) {
                bool isBone = bones.Contains(node->mName.C_Str());
                TOwningPointer<SkeletonNode> skeletonNode;
                if (isBone) {
                    TOwningPointer<SkeletonBone> bone = CreateOwning<SkeletonBone>();
                    bone->OffsetMatrix = AssimpToGLM(bones[node->mName.C_Str()]->mOffsetMatrix);
                    skeletonNode = bone;
                } else {
                    skeletonNode = CreateOwning<SkeletonNode>();
                }
                skeletonNode->NodeName = node->mName.C_Str();
                skeletonNode->LocalMatrix = AssimpToGLM(node->mTransformation);
                for (int i = 0; i < node->mNumChildren; ++i) {
                    skeletonNode->Children.PushBack(constructSkeleton(node->mChildren[i]));
                }
                return skeletonNode;
            };
            TOwningPointer<SkeletonNode> rootSkeletonNode = constructSkeleton(root);
            Skeleton skeleton;
            skeleton.SkeletonName = String(mesh->mName.C_Str()) + "_Skeleton";
            skeleton.RootNode = rootSkeletonNode;
            skeletons->PushBack(skeleton);
        }
    }

    // =========================================================================
    // Binary skeleton serialization
    // =========================================================================
    namespace
    {
        // Standard binary-asset header magic/version expected by
        // EngineAssetManager::LoadBinaryDescriptor (magic 'PLUA', version in {1,2}).
        constexpr UInt32 kAssetMagic = 0x41554C50;      // 'PLUA'
        constexpr UInt32 kSkeletonVersion = 1;
        constexpr const char* kSkeletonTypeName = "Skeleton";

        void WriteSkeletonNode(BinaryFileWriter& writer, const SkeletonNode& node)
        {
            const bool isBone = dynamic_cast<const SkeletonBone*>(&node) != nullptr;
            writer.Write<UInt8>(isBone ? 1 : 0);
            writer.WriteString(node.NodeName);
            writer.Write(node.LocalMatrix);
            if (isBone)
                writer.Write(static_cast<const SkeletonBone&>(node).OffsetMatrix);

            // Only count/write valid children so the tree stays balanced on load.
            UInt32 childCount = 0;
            for (const TOwningPointer<SkeletonNode>& child : node.Children)
                if (child) ++childCount;

            writer.Write<UInt32>(childCount);
            for (const TOwningPointer<SkeletonNode>& child : node.Children)
                if (child) WriteSkeletonNode(writer, *child);
        }

        // Reads a full node (type tag + fields + children) and returns it. Mirrors
        // WriteSkeletonNode, so it works for both the root and any child.
        TOwningPointer<SkeletonNode> ReadSkeletonNode(BinaryFileReader& reader)
        {
            UInt8 isBone = 0;
            reader.Read(isBone);

            TOwningPointer<SkeletonNode> node;
            if (isBone) node = CreateOwning<SkeletonBone>();
            else        node = CreateOwning<SkeletonNode>();

            reader.ReadString(node->NodeName);
            reader.Read(node->LocalMatrix);
            if (isBone)
                reader.Read(static_cast<SkeletonBone&>(*node).OffsetMatrix);

            UInt32 childCount = 0;
            reader.Read(childCount);
            node->Children.Clear();
            for (UInt32 i = 0; i < childCount; ++i)
                node->Children.PushBack(ReadSkeletonNode(reader));

            return node;
        }

        template <typename PathT>
        bool SaveSkeletonImpl(const PathT& path, const Skeleton& skeleton)
        {
            PLU_PROFILE_SCOPE("SaveSkeleton");

            BinaryFileWriter writer(path);
            if (!writer.IsOpen()) return false;

            // Standard binary-asset descriptor header (magic/version/typeName/uuid),
            // so EngineAssetManager::LoadBinaryDescriptor can index this file.
            writer.Write<UInt32>(kAssetMagic);
            writer.Write<UInt32>(kSkeletonVersion);
            writer.WriteString(String(kSkeletonTypeName));   // UInt32 length + bytes == typeLength + typeName
            const UInt64 uuid = skeleton.Uuid;
            writer.Write(uuid);

            // Skeleton payload.
            writer.WriteString(skeleton.SkeletonName);
            const UInt8 hasRoot = skeleton.RootNode ? 1 : 0;
            writer.Write(hasRoot);
            if (skeleton.RootNode)
                WriteSkeletonNode(writer, *skeleton.RootNode);

            return writer.CloseFile();
        }

        template <typename PathT>
        bool LoadSkeletonImpl(const PathT& path, Skeleton& outSkeleton)
        {
            PLU_PROFILE_SCOPE("LoadSkeleton");

            BinaryFileReader reader(path);
            if (!reader.IsOpen()) return false;

            UInt32 magic = 0;
            UInt32 version = 0;
            reader.Read(magic);
            reader.Read(version);
            if (magic != kAssetMagic || version != kSkeletonVersion)
            {
                PLU_CORE_ERROR("Skeleton file has invalid magic/version!");
                return false;
            }

            String typeName;
            reader.ReadString(typeName);
            if (typeName != kSkeletonTypeName)
            {
                PLU_CORE_ERROR("Skeleton file has wrong asset type: {}", typeName.CStr());
                return false;
            }

            UInt64 uuid = 0;
            reader.Read(uuid);
            outSkeleton.Uuid = uuid;

            reader.ReadString(outSkeleton.SkeletonName);

            UInt8 hasRoot = 0;
            reader.Read(hasRoot);
            if (hasRoot)
                outSkeleton.RootNode = ReadSkeletonNode(reader);
            else
                outSkeleton.RootNode = TOwningPointer<SkeletonNode>();

            return !reader.HasError() && reader.CloseFile();
        }
    }

    bool SaveSkeleton(const Path& path, const Skeleton& skeleton)  { return SaveSkeletonImpl(path, skeleton); }
    bool SaveSkeleton(const PathW& path, const Skeleton& skeleton) { return SaveSkeletonImpl(path, skeleton); }
    bool LoadSkeleton(const Path& path, Skeleton& outSkeleton)     { return LoadSkeletonImpl(path, outSkeleton); }
    bool LoadSkeleton(const PathW& path, Skeleton& outSkeleton)    { return LoadSkeletonImpl(path, outSkeleton); }
}

bool Plu::ImportSkeletalMesh(Path skeletonPath, Path outDir, TUsePointer<EngineAssetManager> assetManager, SkeletalMeshImportOptions options)
{
    PLU_PROFILE_SCOPE("ImportSkeletalMesh");

    PLU_CORE_INFO("Importing skeletal mesh from: {}", skeletonPath.CStr());

    //EnsureAssimpLoggerAttached();
    Assimp::Importer importer;

    UInt32 flags =
       aiProcess_Triangulate |
       aiProcess_JoinIdenticalVertices |
       aiProcess_FlipWindingOrder |
       aiProcess_PopulateArmatureData |
       aiProcess_CalcTangentSpace;

    if (options.FlipUVs)
    {
        flags |= aiProcess_FlipUVs;
    }

    if (options.GenerateNormals)
    {
        flags |= aiProcess_GenNormals;
    }
    const aiScene* scene;
    try {
        scene = importer.ReadFile(skeletonPath.CStr(), flags);
    } catch (...) {
        PLU_ERROR("Error importing mesh at: {}", skeletonPath.CStr());
        return false;
    }

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        PLU_CORE_ERROR("Assimp Error: {}", importer.GetErrorString());
        return false;
    }

    DynamicArray<Skeleton> skeletons;
    //ImportSkeletons(const_cast<aiScene *>(scene), assetManager, &skeletons);
    ImportSkeletonsLegacy(const_cast<aiScene *>(scene), assetManager, &skeletons);

    // Phase 1: collapse identical skeletons in this batch into one representative each,
    // preferring the bone-richest (tie-breaker) so we don't drop a bone that only one
    // of two otherwise-identical skeletons happens to skin.
    DynamicArray<Skeleton*> representatives;
    for (auto& skeleton : skeletons) {
        bool merged = false;
        for (UInt64 i = 0; i < representatives.Size(); ++i) {
            if (!representatives[i]->IsIdentical(skeleton)) continue;
            if (skeleton.CountBones() > representatives[i]->CountBones()) {
                PLU_CORE_TRACE("Skeleton '{}' supersedes identical '{}' ({} vs {} bones)",
                    skeleton.SkeletonName.CStr(), representatives[i]->SkeletonName.CStr(),
                    skeleton.CountBones(), representatives[i]->CountBones());
                representatives[i] = &skeleton;
            } else {
                PLU_CORE_TRACE("Skipping identical skeleton (batch duplicate): {}", skeleton.SkeletonName.CStr());
            }
            merged = true;
            break;
        }
        if (!merged) representatives.PushBack(&skeleton);
    }

    // Phase 2: write each representative (skipping ones already on disk).
    for (Skeleton* rep : representatives) {
#ifdef PLU_ENGINE_EDITOR_BUILD
        // Skip if an identical skeleton asset with the same name already exists on disk.
        bool existsIdentical = false;
        for (auto& existingDesc : assetManager->GetAllAssetDescriptorsOfType(Skeleton::GetStaticClass())) {
            if (existingDesc->AssetName != rep->SkeletonName) continue;
            TUsePointer<Skeleton> existing = assetManager->GetAssetData(existingDesc);
            if (existing && existing->IsIdentical(*rep)) { existsIdentical = true; break; }
        }
        if (existsIdentical) {
            PLU_CORE_TRACE("Skipping identical skeleton (existing asset): {}", rep->SkeletonName.CStr());
            continue;
        }
#endif

        Path skeletonSavePath = outDir;
        skeletonSavePath /= rep->SkeletonName + PLU_BINARY_EXT;
        SaveSkeleton(skeletonSavePath, *rep);
        assetManager->LoadAssetDescriptor(skeletonSavePath);
    }
    return true;
}
