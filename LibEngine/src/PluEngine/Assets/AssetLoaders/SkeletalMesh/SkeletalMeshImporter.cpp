//
// Created by Plutex on 7/5/26.
//

#include "PluEngine/Assets/AssetLoaders/SkeletalMesh/SkeletalMeshImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/config.h>

#include <cmath>
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "PluEngine/PluPaths.h"
#include "PluEngine/Assets/AssetLoaders/SkeletalMesh/SkeletalMeshAssetLoader.h"
#include "PluEngine/Assets/AssetLoaders/Mesh/MeshProcessing.h"
#include "PluEngine/AssetTypes/Skeleton/Skeleton.h"
#include "PluEngine/AssetTypes/SkeletalMesh/SkeletalMesh.h"
#include "PluEngine/Assets/AssetDescriptor.h"
#include "PluEngine/AssetTypes/Animation/SkeletalAnimation.h"
#include "PluEngine/Managers/DiskManager.h"

namespace Plu
{
    using MeshProcessing::AssimpToGLM;

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

    namespace
    {
        // Assigns each SkeletonBone a palette index in DFS pre-order over the skeleton tree.
        // Vertex skinning references bones by this index; the ordering is deterministic and
        // matches a DFS over the (order-preserving) serialized tree, so it is stable on load.
        void BuildBonePalette(const SkeletonNode* node, GameHashMap<String, UInt32>& outMap)
        {
            if (!node) return;
            if (dynamic_cast<const SkeletonBone*>(node) != nullptr)
                outMap.Insert(node->NodeName, static_cast<UInt32>(outMap.Size()));
            for (const TOwningPointer<SkeletonNode>& child : node->Children)
                BuildBonePalette(child.GetRaw(), outMap);
        }

        // Places (boneIndex, weight) into a free slot of the vertex, or evicts the current
        // smallest influence when all slots are taken and this weight is larger — so each
        // vertex keeps its kMaxBoneInfluence strongest bones.
        void AddBoneInfluence(SkeletalVertex& v, UInt32 boneIndex, float weight)
        {
            if (weight <= 0.0f) return;

            for (int i = 0; i < kMaxBoneInfluence; ++i)
            {
                if (v.BoneWeights[i] == 0.0f)
                {
                    v.BoneIndices[i] = boneIndex;
                    v.BoneWeights[i] = weight;
                    return;
                }
            }

            int minSlot = 0;
            for (int i = 1; i < kMaxBoneInfluence; ++i)
                if (v.BoneWeights[i] < v.BoneWeights[minSlot]) minSlot = i;

            if (weight > v.BoneWeights[minSlot])
            {
                v.BoneIndices[minSlot] = boneIndex;
                v.BoneWeights[minSlot] = weight;
            }
        }

        // Renormalizes each vertex's influences to sum to 1. Vertices with no influence
        // (e.g. unskinned geometry in a skinned mesh) are bound rigidly to bone 0 so they
        // don't collapse to the origin once a skinning shader consumes them.
        void NormalizeBoneWeights(SkeletalMeshData& data)
        {
            for (SkeletalVertex& v : data.Vertices)
            {
                float sum = 0.0f;
                for (int i = 0; i < kMaxBoneInfluence; ++i) sum += v.BoneWeights[i];

                if (sum <= 0.0f)
                {
                    v.BoneIndices[0] = 0;
                    v.BoneWeights[0] = 1.0f;
                    continue;
                }
                for (int i = 0; i < kMaxBoneInfluence; ++i) v.BoneWeights[i] /= sum;
            }
        }

        // Finds the imported skeleton whose bone palette contains every bone the mesh skins
        // to. Returns nullptr if no candidate covers the mesh. On success fills outPalette.
        TUsePointer<Skeleton> FindSkeletonForMesh(aiMesh* mesh,
                                                  const DynamicArray<TUsePointer<Skeleton>>& skeletons,
                                                  GameHashMap<String, UInt32>& outPalette)
        {
            for (const TUsePointer<Skeleton>& skeleton : skeletons)
            {
                if (!skeleton) continue;

                GameHashMap<String, UInt32> palette;
                BuildBonePalette(skeleton->RootNode.GetRaw(), palette);

                bool coversAll = true;
                for (UInt32 b = 0; b < mesh->mNumBones; ++b)
                {
                    if (!palette.Contains(mesh->mBones[b]->mName.C_Str())) { coversAll = false; break; }
                }
                if (coversAll)
                {
                    outPalette = palette;
                    return skeleton;
                }
            }
            return {};
        }

        // Collects the names of *all* nodes (bones and plain nodes alike) in the skeleton.
        // Animation channels target nodes by name and often address non-bone nodes (pivots,
        // exporter helper nodes), so a bone-only palette is not enough to match them.
        void CollectNodeNames(const SkeletonNode* node, HashSet<String>& outNames)
        {
            if (!node) return;
            outNames.Insert(node->NodeName);
            for (const TOwningPointer<SkeletonNode>& child : node->Children)
                CollectNodeNames(child.GetRaw(), outNames);
        }

        // Picks the skeleton an animation belongs to by name coverage: how many of the
        // animation's channel target-nodes exist in the skeleton's node set. Returns the
        // best-covering skeleton (highest ratio) or null if nothing matches. Unlike mesh
        // matching we don't require full coverage — animations routinely carry channels for
        // nodes outside the skeleton (cameras, $AssimpFbx$ split nodes). outCoverage, when
        // provided, receives the winning ratio in [0,1].
        TUsePointer<Skeleton> FindSkeletonForAnimation(const aiAnimation* animation,
                                                       const DynamicArray<TUsePointer<Skeleton>>& skeletons,
                                                       float* outCoverage = nullptr)
        {
            TUsePointer<Skeleton> best;
            float bestCoverage = 0.0f;

            for (const TUsePointer<Skeleton>& skeleton : skeletons)
            {
                if (!skeleton) continue;

                HashSet<String> names;
                CollectNodeNames(skeleton->RootNode.GetRaw(), names);

                UInt32 matched = 0;
                for (UInt32 c = 0; c < animation->mNumChannels; ++c)
                    if (names.Contains(animation->mChannels[c]->mNodeName.C_Str()))
                        ++matched;

                const float coverage = animation->mNumChannels > 0
                    ? static_cast<float>(matched) / static_cast<float>(animation->mNumChannels)
                    : 0.0f;

                if (coverage > bestCoverage)
                {
                    bestCoverage = coverage;
                    best = skeleton;
                }
            }

            if (outCoverage) *outCoverage = bestCoverage;
            return best;
        }

        // Flattens the skeleton's node hierarchy into a DFS pre-order list (bones and plain
        // nodes alike). Used by the LocalMatrix animation-channel fallback below.
        void CollectNodes(const TUsePointer<SkeletonNode>& node,
                          DynamicArray<TUsePointer<SkeletonNode>>& out)
        {
            if (!node) return;
            out.PushBack(node);
            for (const TOwningPointer<SkeletonNode>& child : node->Children)
                CollectNodes(child, out);
        }

        // Composes a channel's bind-pose local transform from the first key of each animated
        // component (identity for any component the channel doesn't carry). This is the key we
        // compare against skeleton node LocalMatrix when the channel's exact name is missing.
        Matrix4 ComposeChannelBindLocal(const aiNodeAnim* channel)
        {
            Vec3 pos(0.0f);
            Quaternion rot(1.0f, 0.0f, 0.0f, 0.0f); // (w, x, y, z) identity
            Vec3 scale(1.0f);
            if (channel->mNumPositionKeys > 0) pos   = AssimpToGLM(channel->mPositionKeys[0].mValue);
            if (channel->mNumRotationKeys > 0) rot   = AssimpToGLM(channel->mRotationKeys[0].mValue);
            if (channel->mNumScalingKeys  > 0) scale = AssimpToGLM(channel->mScalingKeys[0].mValue);
            return glm::translate(Matrix4(1.0f), pos)
                 * glm::mat4_cast(rot)
                 * glm::scale(Matrix4(1.0f), scale);
        }

        // Component-wise matrix comparison within an absolute epsilon.
        bool LocalMatricesMatch(const Matrix4& a, const Matrix4& b, float eps = 1e-4f)
        {
            const float* pa = glm::value_ptr(a);
            const float* pb = glm::value_ptr(b);
            for (int i = 0; i < 16; ++i)
                if (std::fabs(pa[i] - pb[i]) > eps) return false;
            return true;
        }

        // Fallback node lookup for animation channels whose exact name isn't in the skeleton.
        // Assimp's FBX pivot splitting can leave the skeleton with a `..._$AssimpFbx$_PreRotation`
        // node while the animation only carries a `..._$AssimpFbx$_Rotation` channel; the two
        // often share the same local matrix. Returns the single unclaimed skeleton node whose
        // LocalMatrix equals `local` (within epsilon), or null when there is no match or the
        // match is ambiguous (more than one candidate).
        TUsePointer<SkeletonNode> FindNodeByLocalMatrix(const Skeleton& skeleton,
                                                        const Matrix4& local,
                                                        const HashSet<String>& claimed)
        {
            DynamicArray<TUsePointer<SkeletonNode>> nodes;
            CollectNodes(skeleton.RootNode, nodes);

            TUsePointer<SkeletonNode> match;
            UInt32 matchCount = 0;
            for (const TUsePointer<SkeletonNode>& node : nodes)
            {
                if (!node || claimed.Contains(node->NodeName)) continue;
                if (!LocalMatricesMatch(node->LocalMatrix, local)) continue;
                ++matchCount;
                if (!match) match = node;
            }

            if (matchCount > 1)
            {
                PLU_CORE_WARN("LocalMatrix fallback ambiguous: {} skeleton nodes share the channel's "
                              "local matrix — leaving channel unmatched", matchCount);
                return nullptr;
            }
            return match;
        }

        // Strips characters that are illegal in file names.
        String SanitizeAssetName(String name)
        {
            name.Replace("/", "_");
            name.Replace("\\", "_");
            name.Replace(":", "_");
            name.Replace("|", "_");
            name.Replace(".", "_");
            return name;
        }
    }

    // Imports every skinned mesh in the scene as a SkeletalMesh asset: geometry is built with
    // the shared MeshProcessing loop, per-vertex bone influences are gathered from aiMesh bones
    // and mapped onto the owning skeleton's bone palette. One asset is produced per aiMesh.
    void ImportSkeletalMeshes(const aiScene* scene, const Path& outDir,
                              const DynamicArray<TUsePointer<Skeleton>>& skeletons,
                              const SkeletalMeshImportOptions& options,
                              TUsePointer<EngineAssetManager> assetManager)
    {
        PLU_PROFILE_SCOPE("ImportSkeletalMeshes");

        for (UInt32 mi = 0; mi < scene->mNumMeshes; ++mi)
        {
            aiMesh* mesh = scene->mMeshes[mi];
            if (!mesh->HasBones()) continue;

            // Resolve the skeleton this mesh skins to (an explicit override wins).
            GameHashMap<String, UInt32> palette;
            TUsePointer<Skeleton> skeleton;
            if (options.SkeletonToUse)
            {
                skeleton = options.SkeletonToUse;
                BuildBonePalette(skeleton->RootNode.GetRaw(), palette);
            }
            else
            {
                skeleton = FindSkeletonForMesh(mesh, skeletons, palette);
            }

            if (!skeleton)
            {
                PLU_CORE_WARN("No matching skeleton for skeletal mesh '{}' — skipping", mesh->mName.C_Str());
                continue;
            }

            SkeletalMesh skeletalMesh;

            // Skeletal vertices stay in mesh-local space (identity transform); bind pose is
            // carried by the skeleton's offset matrices, so baking node transforms here would
            // double-transform. Scale still applies to positions.
            MeshProcessing::ProcessMeshGeometry(mesh, skeletalMesh.MeshData.Vertices,
                                                skeletalMesh.MeshData.Indices,
                                                skeletalMesh.MeshData.MaterialIndex,
                                                options.Scale, options.FlipUVs, glm::mat4(1.0f), false);

            // Scatter bone weights onto the vertices (mesh-local vertex ids, buffer starts at 0).
            for (UInt32 b = 0; b < mesh->mNumBones; ++b)
            {
                aiBone* bone = mesh->mBones[b];
                UInt32* boneIndex = palette.Find(bone->mName.C_Str());
                if (!boneIndex)
                {
                    PLU_CORE_WARN("Bone '{}' not found in skeleton palette — skipping its weights", bone->mName.C_Str());
                    continue;
                }

                for (UInt32 w = 0; w < bone->mNumWeights; ++w)
                {
                    const aiVertexWeight& vw = bone->mWeights[w];
                    if (vw.mVertexId >= skeletalMesh.MeshData.Vertices.Size()) continue;
                    AddBoneInfluence(skeletalMesh.MeshData.Vertices[vw.mVertexId], *boneIndex, vw.mWeight);
                }
            }

            NormalizeBoneWeights(skeletalMesh.MeshData);
            skeletalMesh.MeshSkeleton = skeleton;

            String meshName = mesh->mName.length > 0
                ? String(mesh->mName.C_Str())
                : (String("SkeletalMesh_") + String::FromInt(mi));
            meshName = SanitizeAssetName(meshName);

            Path savePath = outDir;
            savePath /= meshName + PLU_BINARY_EXT;

            if (!SaveSkeletalMesh(savePath, skeletalMesh))
            {
                PLU_CORE_ERROR("Failed to save skeletal mesh: {}", meshName.CStr());
                continue;
            }

            PLU_CORE_INFO("Saved skeletal mesh: {} ({} vertices, {} indices, skeleton '{}')",
                          meshName.CStr(),
                          skeletalMesh.MeshData.Vertices.Size(),
                          skeletalMesh.MeshData.Indices.Size(),
                          skeleton->SkeletonName.CStr());

            assetManager->LoadAssetDescriptor(savePath);
        }
    }

    void ImportAnimations(const aiScene* scene, Path outDir,
                          const DynamicArray<TUsePointer<Skeleton>>& skeletons,
                          const SkeletalMeshImportOptions& options, TUsePointer<EngineAssetManager> assetManager)
    {
        // Candidate skeletons to match animations against: the ones imported/reused in this
        // batch, plus an explicit override if the caller supplied one.
        DynamicArray<TUsePointer<Skeleton>> candidates = skeletons;
        if (options.SkeletonToUse) candidates.PushBack(options.SkeletonToUse);

        for (int i = 0; i < scene->mNumAnimations; i++) {
            aiAnimation* animation = scene->mAnimations[i];
            float FPS = animation->mTicksPerSecond;
            float duration = animation->mDuration;
            PLU_CORE_INFO("Animation '{}' FPS: {}, frames total: {}, duration: {}, channels: {}", animation->mName.C_Str(), FPS, duration, duration / FPS, animation->mNumChannels);

            // Resolve which skeleton this animation drives (by channel-node name coverage).
            float coverage = 0.0f;
            TUsePointer<Skeleton> owningSkeleton = FindSkeletonForAnimation(animation, candidates, &coverage);
            if (owningSkeleton)
                PLU_CORE_INFO("Animation '{}' belongs to skeleton '{}' ({:.0f}% channel coverage)",
                              animation->mName.C_Str(), owningSkeleton->SkeletonName.CStr(), coverage * 100.0f);
            else
                PLU_CORE_WARN("Animation '{}' matched no skeleton (no candidate covers its channels)",
                              animation->mName.C_Str());

            if (!owningSkeleton) {
                quit:
                continue;
            }

            TOwningPointer<Animation> newAnimation = CreateOwning<Animation>();
            newAnimation->Uuid = PluUUID();
            newAnimation->AnimationSkeleton = owningSkeleton;
            newAnimation->FramesAmount = static_cast<int>(animation->mDuration);
            newAnimation->FramesPerSecond = FPS;

            newAnimation->Tracks.Reserve(animation->mNumChannels);

            // Node names already bound to a track this animation, so the LocalMatrix fallback
            // can't reuse a node another channel already owns.
            HashSet<String> claimedNodeNames;

            for (int j = 0; j < animation->mNumChannels; j++) {
                aiNodeAnim* channel = animation->mChannels[j];
                String nodeName = channel->mNodeName.C_Str();

                AnimationTrack newTrack;
                newTrack.Node = owningSkeleton->FindNodeByName(channel->mNodeName.C_Str());

                if (!newTrack.Node) {
                    // Exact name absent (typical of Assimp FBX pivot splitting, e.g. the skeleton
                    // kept `..._$AssimpFbx$_PreRotation` while the channel is `..._$AssimpFbx$_Rotation`).
                    // Fall back to matching the channel's bind-pose local matrix against the skeleton.
                    Matrix4 channelLocal = ComposeChannelBindLocal(channel);
                    newTrack.Node = FindNodeByLocalMatrix(*owningSkeleton, channelLocal, claimedNodeNames);
                    if (newTrack.Node)
                        PLU_CORE_WARN("Channel '{}' had no name match; bound to node '{}' by LocalMatrix fallback",
                                      channel->mNodeName.C_Str(), newTrack.Node->NodeName.CStr());
                }

                if (!newTrack.Node) {
                    PLU_CORE_ERROR("Node not found! {}", channel->mNodeName.C_Str());
                    goto quit;
                }

                claimedNodeNames.Insert(newTrack.Node->NodeName);

                newTrack.LocationKeys.Reserve(channel->mNumPositionKeys);
                for (int p = 0; p < channel->mNumPositionKeys; ++p) {
                    const aiVectorKey* key = channel->mPositionKeys + p;
                    AnimationVectorKey outKey{};
                    outKey.Timestamp = key->mTime;
                    outKey.Value = AssimpToGLM(key->mValue);
                    newTrack.LocationKeys.PushBack(outKey);
                }
                newTrack.RotationKeys.Reserve(channel->mNumRotationKeys);
                for (int r = 0; r < channel->mNumRotationKeys; ++r) {
                    const aiQuatKey* key = channel->mRotationKeys + r;
                    AnimationQuatKey outKey{};
                    outKey.Timestamp = key->mTime;
                    outKey.Value = AssimpToGLM(key->mValue);
                    newTrack.RotationKeys.PushBack(outKey);
                }
                newTrack.ScaleKeys.Reserve(channel->mNumScalingKeys);
                for (int s = 0; s < channel->mNumScalingKeys; ++s) {
                    const aiVectorKey* key = channel->mScalingKeys + s;
                    AnimationVectorKey outKey{};
                    outKey.Timestamp = key->mTime;
                    outKey.Value = AssimpToGLM(key->mValue);
                    newTrack.ScaleKeys.PushBack(outKey);
                }
                // Assimp emits keys time-sorted, but the sampler's binary search requires it — enforce.
                newTrack.SortKeys();
                newAnimation->Tracks.Insert(newTrack.Node->NodeName, newTrack);
            }

            String animName = animation->mName.length > 0
                ? String(animation->mName.C_Str())
                : (String("Animation_") + String::FromInt(i));
            animName = SanitizeAssetName(animName);

            Path savePath = outDir;
            savePath /= animName + PLU_BINARY_EXT;

            if (!SaveAnimation(savePath, *newAnimation))
            {
                PLU_CORE_ERROR("Failed to save animation: {}", animName.CStr());
                continue;
            }

            PLU_CORE_INFO("Saved animation: {} ({} tracks, skeleton '{}')",
                          animName.CStr(), newAnimation->Tracks.Size(),
                          owningSkeleton->SkeletonName.CStr());

            assetManager->LoadAssetDescriptor(savePath);
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

        // =====================================================================
        // Binary skeletal-mesh serialization (same 'PLUA' descriptor header).
        // =====================================================================
        constexpr UInt32 kSkeletalMeshVersion = 1;
        constexpr const char* kSkeletalMeshTypeName = "SkeletalMesh";

        void WriteSkeletalVertex(BinaryFileWriter& writer, const SkeletalVertex& v)
        {
            // Per-field (not a raw struct blob) so the format is independent of Vertex padding.
            writer.Write(v.Position);
            writer.Write(v.Normal);
            writer.WriteArray(v.UV, 2);
            writer.Write(v.Color);
            writer.Write(v.Tangent);
            writer.WriteArray(v.BoneIndices, kMaxBoneInfluence);
            writer.WriteArray(v.BoneWeights, kMaxBoneInfluence);
        }

        void ReadSkeletalVertex(BinaryFileReader& reader, SkeletalVertex& v)
        {
            reader.Read(v.Position);
            reader.Read(v.Normal);
            reader.ReadArray(v.UV, 2);
            reader.Read(v.Color);
            reader.Read(v.Tangent);
            reader.ReadArray(v.BoneIndices, kMaxBoneInfluence);
            reader.ReadArray(v.BoneWeights, kMaxBoneInfluence);
        }

        template <typename PathT>
        bool SaveSkeletalMeshImpl(const PathT& path, const SkeletalMesh& mesh)
        {
            PLU_PROFILE_SCOPE("SaveSkeletalMesh");

            BinaryFileWriter writer(path);
            if (!writer.IsOpen()) return false;

            writer.Write<UInt32>(kAssetMagic);
            writer.Write<UInt32>(kSkeletalMeshVersion);
            writer.WriteString(String(kSkeletalMeshTypeName));
            const UInt64 uuid = mesh.Uuid;
            writer.Write(uuid);

            // Referenced skeleton (by UUID; 0 == none). Resolved on load via the asset manager.
            const UInt64 skeletonUuid = mesh.MeshSkeleton ? static_cast<UInt64>(mesh.MeshSkeleton->Uuid) : 0;
            writer.Write(skeletonUuid);

            const UInt32 vertexCount = mesh.MeshData.Vertices.Size();
            writer.Write(vertexCount);
            for (UInt32 i = 0; i < vertexCount; ++i)
                WriteSkeletalVertex(writer, mesh.MeshData.Vertices[i]);

            const UInt32 indexCount = mesh.MeshData.Indices.Size();
            writer.Write(indexCount);
            writer.WriteArray(mesh.MeshData.Indices.Data(), indexCount);

            writer.Write(mesh.MeshData.MaterialIndex);

            return writer.CloseFile();
        }

        template <typename PathT>
        bool LoadSkeletalMeshImpl(const PathT& path, SkeletalMesh& outMesh, TUsePointer<EngineAssetManager> assetManager)
        {
            PLU_PROFILE_SCOPE("LoadSkeletalMesh");

            BinaryFileReader reader(path);
            if (!reader.IsOpen()) return false;

            UInt32 magic = 0;
            UInt32 version = 0;
            reader.Read(magic);
            reader.Read(version);
            if (magic != kAssetMagic || version != kSkeletalMeshVersion)
            {
                PLU_CORE_ERROR("SkeletalMesh file has invalid magic/version!");
                return false;
            }

            String typeName;
            reader.ReadString(typeName);
            if (typeName != kSkeletalMeshTypeName)
            {
                PLU_CORE_ERROR("SkeletalMesh file has wrong asset type: {}", typeName.CStr());
                return false;
            }

            UInt64 uuid = 0;
            reader.Read(uuid);
            outMesh.Uuid = uuid;

            UInt64 skeletonUuid = 0;
            reader.Read(skeletonUuid);

            UInt32 vertexCount = 0;
            reader.Read(vertexCount);
            outMesh.MeshData.Vertices.Resize(vertexCount);
            for (UInt32 i = 0; i < vertexCount; ++i)
                ReadSkeletalVertex(reader, outMesh.MeshData.Vertices[i]);

            UInt32 indexCount = 0;
            reader.Read(indexCount);
            outMesh.MeshData.Indices.Resize(indexCount);
            reader.ReadArray(outMesh.MeshData.Indices.Data(), indexCount);

            reader.Read(outMesh.MeshData.MaterialIndex);

            if (reader.HasError()) return false;

            // Resolve the skeleton reference once the payload has been read.
            if (skeletonUuid != 0 && assetManager)
                outMesh.MeshSkeleton = assetManager->GetAssetData(PluUUID(skeletonUuid));

            return reader.CloseFile();
        }

        // =====================================================================
        // Binary animation serialization (same 'PLUA' descriptor header).
        // =====================================================================
        // v2: per-channel key arrays (v1 stored merged keyframes with channel flags).
        constexpr UInt32 kAnimationVersion = 2;
        constexpr const char* kAnimationTypeName = "Animation";

        // Per-field (not a raw struct blob) so the format is independent of padding.
        void WriteVectorKeys(BinaryFileWriter& writer, const DynamicArray<AnimationVectorKey>& keys)
        {
            writer.Write<UInt32>(static_cast<UInt32>(keys.Size()));
            for (const AnimationVectorKey& key : keys)
            {
                writer.Write(key.Timestamp);
                writer.Write(key.Value);
            }
        }

        void WriteQuatKeys(BinaryFileWriter& writer, const DynamicArray<AnimationQuatKey>& keys)
        {
            writer.Write<UInt32>(static_cast<UInt32>(keys.Size()));
            for (const AnimationQuatKey& key : keys)
            {
                writer.Write(key.Timestamp);
                writer.Write(key.Value);
            }
        }

        void ReadVectorKeys(BinaryFileReader& reader, DynamicArray<AnimationVectorKey>& outKeys)
        {
            UInt32 count = 0;
            reader.Read(count);
            outKeys.Reserve(count);
            for (UInt32 i = 0; i < count; ++i)
            {
                AnimationVectorKey key{};
                reader.Read(key.Timestamp);
                reader.Read(key.Value);
                outKeys.PushBack(key);
            }
        }

        void ReadQuatKeys(BinaryFileReader& reader, DynamicArray<AnimationQuatKey>& outKeys)
        {
            UInt32 count = 0;
            reader.Read(count);
            outKeys.Reserve(count);
            for (UInt32 i = 0; i < count; ++i)
            {
                AnimationQuatKey key{};
                reader.Read(key.Timestamp);
                reader.Read(key.Value);
                outKeys.PushBack(key);
            }
        }

        // v1 compatibility: one merged keyframe stream per track; split it into the
        // per-channel arrays using the stored channel flags.
        void ReadTrackKeysV1(BinaryFileReader& reader, AnimationTrack& track)
        {
            UInt32 keyCount = 0;
            reader.Read(keyCount);
            for (UInt32 k = 0; k < keyCount; ++k)
            {
                double timestamp = 0.0;
                Vec3 location(0.0f);
                Quaternion rotation(1.0f, 0.0f, 0.0f, 0.0f);
                Vec3 scale(1.0f);
                reader.Read(timestamp);
                reader.Read(location);
                reader.Read(rotation);
                reader.Read(scale);
                UInt8 isLoc = 0, isScale = 0, isRot = 0;
                reader.Read(isLoc);
                reader.Read(isScale);
                reader.Read(isRot);

                if (isLoc)
                {
                    AnimationVectorKey key{};
                    key.Timestamp = timestamp;
                    key.Value = location;
                    track.LocationKeys.PushBack(key);
                }
                if (isRot)
                {
                    AnimationQuatKey key{};
                    key.Timestamp = timestamp;
                    key.Value = rotation;
                    track.RotationKeys.PushBack(key);
                }
                if (isScale)
                {
                    AnimationVectorKey key{};
                    key.Timestamp = timestamp;
                    key.Value = scale;
                    track.ScaleKeys.PushBack(key);
                }
            }
            // v1 keyframes were hash-map ordered on disk — the sampler needs time-sorted arrays.
            track.SortKeys();
        }

        template <typename PathT>
        bool SaveAnimationImpl(const PathT& path, const Animation& animation)
        {
            PLU_PROFILE_SCOPE("SaveAnimation");

            BinaryFileWriter writer(path);
            if (!writer.IsOpen()) return false;

            writer.Write<UInt32>(kAssetMagic);
            writer.Write<UInt32>(kAnimationVersion);
            writer.WriteString(String(kAnimationTypeName));
            const UInt64 uuid = animation.Uuid;
            writer.Write(uuid);

            // Referenced skeleton (by UUID; 0 == none). Resolved on load via the asset manager.
            const UInt64 skeletonUuid = animation.AnimationSkeleton
                ? static_cast<UInt64>(animation.AnimationSkeleton->Uuid) : 0;
            writer.Write(skeletonUuid);

            writer.Write(animation.FramesAmount);
            writer.Write(animation.FramesPerSecond);

            const UInt32 trackCount = animation.Tracks.Size();
            writer.Write(trackCount);
            for (const auto& [nodeName, track] : animation.Tracks)
            {
                // Track key == target node name; the node pointer is rebound on load.
                writer.WriteString(nodeName);

                WriteVectorKeys(writer, track.LocationKeys);
                WriteQuatKeys(writer, track.RotationKeys);
                WriteVectorKeys(writer, track.ScaleKeys);
            }

            return writer.CloseFile();
        }

        template <typename PathT>
        bool LoadAnimationImpl(const PathT& path, Animation& outAnimation, TUsePointer<EngineAssetManager> assetManager)
        {
            PLU_PROFILE_SCOPE("LoadAnimation");

            BinaryFileReader reader(path);
            if (!reader.IsOpen()) return false;

            UInt32 magic = 0;
            UInt32 version = 0;
            reader.Read(magic);
            reader.Read(version);
            if (magic != kAssetMagic || version == 0 || version > kAnimationVersion)
            {
                PLU_CORE_ERROR("Animation file has invalid magic/version!");
                return false;
            }

            String typeName;
            reader.ReadString(typeName);
            if (typeName != kAnimationTypeName)
            {
                PLU_CORE_ERROR("Animation file has wrong asset type: {}", typeName.CStr());
                return false;
            }

            UInt64 uuid = 0;
            reader.Read(uuid);
            outAnimation.Uuid = uuid;

            UInt64 skeletonUuid = 0;
            reader.Read(skeletonUuid);

            reader.Read(outAnimation.FramesAmount);
            reader.Read(outAnimation.FramesPerSecond);

            // Resolve the skeleton first so track nodes can be rebound by name.
            TUsePointer<Skeleton> skeleton;
            if (skeletonUuid != 0 && assetManager)
                skeleton = assetManager->GetAssetData(PluUUID(skeletonUuid));
            outAnimation.AnimationSkeleton = skeleton;

            UInt32 trackCount = 0;
            reader.Read(trackCount);
            outAnimation.Tracks.Clear();
            outAnimation.Tracks.Reserve(trackCount);
            for (UInt32 t = 0; t < trackCount; ++t)
            {
                String nodeName;
                reader.ReadString(nodeName);

                AnimationTrack track;
                if (skeleton) track.Node = skeleton->FindNodeByName(nodeName);
                if (!track.Node)
                    PLU_CORE_WARN("Animation track node '{}' not found in skeleton — track left unbound", nodeName.CStr());

                if (version >= 2)
                {
                    ReadVectorKeys(reader, track.LocationKeys);
                    ReadQuatKeys(reader, track.RotationKeys);
                    ReadVectorKeys(reader, track.ScaleKeys);
                }
                else
                {
                    ReadTrackKeysV1(reader, track);
                }

                outAnimation.Tracks.Insert(nodeName, track);
            }

            return !reader.HasError() && reader.CloseFile();
        }
    }

    bool SaveSkeleton(const Path& path, const Skeleton& skeleton)  { return SaveSkeletonImpl(path, skeleton); }
    bool SaveSkeleton(const PathW& path, const Skeleton& skeleton) { return SaveSkeletonImpl(path, skeleton); }
    bool LoadSkeleton(const Path& path, Skeleton& outSkeleton)     { return LoadSkeletonImpl(path, outSkeleton); }
    bool LoadSkeleton(const PathW& path, Skeleton& outSkeleton)    { return LoadSkeletonImpl(path, outSkeleton); }

    bool SaveSkeletalMesh(const Path& path, const SkeletalMesh& mesh) { return SaveSkeletalMeshImpl(path, mesh); }
    bool LoadSkeletalMesh(const Path& path, SkeletalMesh& outMesh, TUsePointer<EngineAssetManager> assetManager)
    {
        return LoadSkeletalMeshImpl(path, outMesh, assetManager);
    }

    bool SaveAnimation(const Path& path, const Animation& animation)  { return SaveAnimationImpl(path, animation); }
    bool SaveAnimation(const PathW& path, const Animation& animation) { return SaveAnimationImpl(path, animation); }
    bool LoadAnimation(const Path& path, Animation& outAnimation, TUsePointer<EngineAssetManager> assetManager)
    {
        return LoadAnimationImpl(path, outAnimation, assetManager);
    }
    bool LoadAnimation(const PathW& path, Animation& outAnimation, TUsePointer<EngineAssetManager> assetManager)
    {
        return LoadAnimationImpl(path, outAnimation, assetManager);
    }
}

bool Plu::ImportSkeletalMesh(Path skeletonPath, Path outDir, TUsePointer<EngineAssetManager> assetManager, SkeletalMeshImportOptions options)
{
    PLU_PROFILE_SCOPE("ImportSkeletalMesh");

    PLU_CORE_INFO("Importing skeletal mesh from: {}", skeletonPath.CStr());

    //EnsureAssimpLoggerAttached();
    MeshProcessing::EnsureAssimpLoggerAttached();
    Assimp::Importer importer;

    // Collapse Assimp's FBX pivot chain. With pivots preserved (the default) each FBX node is
    // split into `<bone>_$AssimpFbx$_Translation/PreRotation/Rotation/Scaling/...` helper nodes;
    // components that are identity at bind pose are dropped from the hierarchy, yet animation
    // still carries channels for them (e.g. skeleton keeps `_PreRotation` but the clip drives
    // `_Rotation`) — so channel names never resolve to a skeleton node. Collapsing bakes the
    // whole chain into one transform per bone, so node and channel names both become the plain
    // bone name (`mixamorig:LeftArm`) and match exactly.
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

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

    if (!scene || !scene->mRootNode)
    {
        PLU_CORE_ERROR("Assimp Error: {}", importer.GetErrorString());
        return false;
    }

    if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE && !scene->HasAnimations()) {
        PLU_CORE_ERROR("Cannot import asset without animations, skeletons or meshes");
        return false;
    }

    // Skeletons the imported meshes will be linked to. When the caller supplies an explicit
    // skeleton we skip importing/saving new ones and let ImportSkeletalMeshes use the override.
    DynamicArray<TUsePointer<Skeleton>> savedSkeletons;

    if (!options.SkeletonToUse)
    {
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

        // Phase 2: write each representative (skipping ones already on disk) and keep a handle
        // to the resulting asset so the meshes can reference it.
        for (Skeleton* rep : representatives) {
#ifdef PLU_ENGINE_EDITOR_BUILD
            // Reuse an identical skeleton asset with the same name if one already exists on disk.
            TUsePointer<Skeleton> existingIdentical;
            for (auto& existingDesc : assetManager->GetAllAssetDescriptorsOfType(Skeleton::GetStaticClass())) {
                if (existingDesc->AssetName != rep->SkeletonName) continue;
                TUsePointer<Skeleton> existing = assetManager->GetAssetData(existingDesc);
                if (existing && existing->IsIdentical(*rep)) { existingIdentical = existing; break; }
            }
            if (existingIdentical) {
                PLU_CORE_TRACE("Skipping identical skeleton (existing asset): {}", rep->SkeletonName.CStr());
                savedSkeletons.PushBack(existingIdentical);
                continue;
            }
#endif

            Path skeletonSavePath = outDir;
            skeletonSavePath /= rep->SkeletonName + PLU_BINARY_EXT;
            SaveSkeleton(skeletonSavePath, *rep);
            assetManager->LoadAssetDescriptor(skeletonSavePath);

            TUsePointer<Skeleton> saved = assetManager->GetAssetData(rep->Uuid);
            if (saved) savedSkeletons.PushBack(saved);
        }
    }

    // Phase 3: import the skinned geometry itself, linking each mesh to its skeleton.
    ImportSkeletalMeshes(scene, outDir, savedSkeletons, options, assetManager);

    ImportAnimations(scene, outDir, savedSkeletons, options, assetManager);

    return true;
}
