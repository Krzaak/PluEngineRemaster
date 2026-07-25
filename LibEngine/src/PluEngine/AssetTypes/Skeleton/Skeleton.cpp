//
// Created by Plutex on 7/6/26.
//

#include "PluEngine/AssetTypes/Skeleton/Skeleton.h"

#include "PluEngine/Timer.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

Matrix4 Plu::SkeletonAttachPoint::GetLocalMatrix() const
{
    return glm::translate(Matrix4(1.0f), RelativeLocation) *
        glm::mat4_cast(Quaternion(glm::radians(RelativeRotation)));
}

namespace Plu
{
    // Deep structural comparison of two nodes (name, local transform, and recursively
    // their children). Uuid is intentionally ignored so two independently-imported
    // copies of the same skeleton compare equal.
    //
    // Bone-ness is deliberately lenient: a node that is a bone in one skeleton but a
    // plain node in the other is still treated as matching (this happens when two
    // meshes share one armature but skin slightly different bone subsets, e.g. a leaf
    // end-effector). Only when BOTH nodes are bones do their offset matrices have to
    // match.
    static bool NodesIdentical(const SkeletonNode* a, const SkeletonNode* b)
    {
        if (!a || !b) return a == b;                 // both null == identical
        if (a->NodeName != b->NodeName) return false;
        if (a->LocalMatrix != b->LocalMatrix) return false;

        const auto* boneA = dynamic_cast<const SkeletonBone*>(a);
        const auto* boneB = dynamic_cast<const SkeletonBone*>(b);
        if (boneA && boneB && boneA->OffsetMatrix != boneB->OffsetMatrix) return false;

        if (a->Children.Size() != b->Children.Size()) return false;
        for (UInt64 i = 0; i < a->Children.Size(); ++i)
            if (!NodesIdentical(a->Children[i].GetRaw(), b->Children[i].GetRaw())) return false;

        return true;
    }
}

namespace Plu
{
    static UInt64 CountBonesRec(const SkeletonNode* n)
    {
        if (!n) return 0;
        UInt64 count = (dynamic_cast<const SkeletonBone*>(n) != nullptr) ? 1 : 0;
        for (UInt64 i = 0; i < n->Children.Size(); ++i)
            count += CountBonesRec(n->Children[i].GetRaw());
        return count;
    }
}

UInt64 Plu::Skeleton::CountBones() const
{
    return CountBonesRec(RootNode.GetRaw());
}

namespace Plu
{
    static TUsePointer<SkeletonNode> FindNodeByNameRec(TUsePointer<SkeletonNode> n, const String& nameToFind)
    {
        if (!n) return nullptr;
        if (n->NodeName == nameToFind) return n;
        for (UInt64 i = 0; i < n->Children.Size(); ++i) {
            if (TUsePointer<SkeletonNode> node = FindNodeByNameRec(n->Children[i], nameToFind)) {
                return node;
            }
        }
        return nullptr;
    }
}

Plu::TUsePointer<Plu::SkeletonNode> Plu::Skeleton::FindNodeByName(const String &nodeName) const
{
    return FindNodeByNameRec(RootNode, nodeName);
}

namespace Plu
{
    // Standalone copy of a single bone: name + local + offset, no Children. Used for the
    // flat skinning palette where each entry is animated independently.
    static void CollectBoneCopies(const SkeletonNode* node, DynamicArray<TOwningPointer<SkeletonBone>>& out)
    {
        if (!node) return;
        if (const auto* bone = dynamic_cast<const SkeletonBone*>(node))
        {
            TOwningPointer<SkeletonBone> copy = CreateOwning<SkeletonBone>();
            copy->NodeName    = bone->NodeName;
            copy->LocalMatrix = bone->LocalMatrix;
            copy->OffsetMatrix = bone->OffsetMatrix;
            out.PushBack(copy);
        }
        for (UInt64 i = 0; i < node->Children.Size(); ++i)
            CollectBoneCopies(node->Children[i].GetRaw(), out);
    }

    // Deep copy of a node preserving its dynamic type (bone vs plain node) and its
    // whole subtree, so the returned tree can be animated without touching the asset.
    static TOwningPointer<SkeletonNode> CopyNodeTree(const SkeletonNode* src)
    {
        TOwningPointer<SkeletonNode> dst;
        if (const auto* bone = dynamic_cast<const SkeletonBone*>(src))
        {
            TOwningPointer<SkeletonBone> boneCopy = CreateOwning<SkeletonBone>();
            boneCopy->OffsetMatrix = bone->OffsetMatrix;
            dst = boneCopy;
        }
        else
            dst = CreateOwning<SkeletonNode>();

        dst->NodeName    = src->NodeName;
        dst->LocalMatrix = src->LocalMatrix;
        for (UInt64 i = 0; i < src->Children.Size(); ++i)
            dst->Children.PushBack(CopyNodeTree(src->Children[i].GetRaw()));
        return dst;
    }

    // Flattens an (already copied) tree into DFS pre-order, keeping the strong refs so
    // the hierarchy stays alive for as long as the palette does.
    static void FlattenNodeTree(const TOwningPointer<SkeletonNode>& node, DynamicArray<TOwningPointer<SkeletonNode>>& out)
    {
        if (!node) return;
        out.PushBack(node);
        for (UInt64 i = 0; i < node->Children.Size(); ++i)
            FlattenNodeTree(node->Children[i], out);
    }
}

void Plu::Skeleton::CreateBonePalette(DynamicArray<TOwningPointer<SkeletonBone>>* outPalette) const
{
    if (!outPalette) return;
    outPalette->Clear();
    CollectBoneCopies(RootNode.GetRaw(), *outPalette);
}

void Plu::Skeleton::CreateNodePalette(DynamicArray<TOwningPointer<SkeletonNode>>* outPalette) const
{
    if (!outPalette) return;
    outPalette->Clear();
    if (!RootNode) return;
    TOwningPointer<SkeletonNode> rootCopy = CopyNodeTree(RootNode.GetRaw());
    FlattenNodeTree(rootCopy, *outPalette);
}

Int32 Plu::SkeletonPoseLayout::FindIndex(const String& nodeName) const
{
    const Int32* found = NameToIndex.Find(nodeName);
    return found ? *found : -1;
}

void Plu::SkeletonPoseLayout::MakeBindPose(Pose& outPose) const
{
    outPose = LocalBindTransform;
}

void Plu::SkeletonPoseLayout::ComposeGlobals(const Pose& localPose, Pose& outGlobalPose) const
{
    const UInt64 nodeCount = NodeCount();
    if (outGlobalPose.Size() != nodeCount) outGlobalPose.Resize(nodeCount);

    for (UInt64 i = 0; i < nodeCount; ++i)
    {
        // Fall back to bind when the incoming pose is short (pose built for another skeleton).
        const BoneTransform& local = i < localPose.Size() ? localPose[i] : LocalBindTransform[i];
        const Int32 parent = ParentIndex[i];
        outGlobalPose[i] = (parent < 0) ? local : outGlobalPose[static_cast<UInt64>(parent)].Compose(local);
    }
}

void Plu::SkeletonPoseLayout::BuildBonePalette(const Pose& globalPose, DynamicArray<std::pair<Matrix4, Matrix4>>& outPalette) const
{
    outPalette.Clear();
    outPalette.Reserve(BoneCount);

    const UInt64 nodeCount = NodeCount();
    for (UInt64 i = 0; i < nodeCount && i < globalPose.Size(); ++i)
    {
        if (BoneSlot[i] < 0) continue;
        outPalette.PushBack({OffsetMatrix[i], globalPose[i].ToMatrix()});
    }
}

void Plu::SkeletonPoseLayout::BuildSubtreeMask(Int32 rootIndex, float insideWeight, float outsideWeight,
                                               DynamicArray<float>& outWeights) const
{
    const UInt64 nodeCount = NodeCount();
    outWeights.Resize(nodeCount);

    if (rootIndex < 0) {
        for (UInt64 i = 0; i < nodeCount; ++i) outWeights[i] = outsideWeight;
        return;
    }

    // Separate from outWeights so insideWeight == 0 doesn't get mistaken for "outside" while
    // propagating membership down the tree.
    DynamicArray<UInt8> inSubtree;
    inSubtree.Resize(nodeCount);

    for (UInt64 i = 0; i < static_cast<UInt64>(rootIndex); ++i) {
        inSubtree[i] = 0;
        outWeights[i] = outsideWeight;
    }
    for (UInt64 i = static_cast<UInt64>(rootIndex); i < nodeCount; ++i) {
        const Int32 parent = ParentIndex[i];
        const bool inside = (static_cast<Int32>(i) == rootIndex)
            || (parent >= 0 && inSubtree[static_cast<UInt64>(parent)]);
        inSubtree[i] = inside ? 1 : 0;
        outWeights[i] = inside ? insideWeight : outsideWeight;
    }
}

void Plu::SkeletonPoseLayout::Clear()
{
    ParentIndex.Clear();
    BoneSlot.Clear();
    LocalMatrix.Clear();
    OffsetMatrix.Clear();
    NodeName.Clear();
    NameToIndex.Clear();
    LocalBindTransform.Clear();
    BoneCount = 0;
}

namespace Plu
{
    // Appends `node` then recurses into its children, so nodes land in DFS pre-order and a
    // parent is always written before its children (ParentIndex[i] < i). Bone slots are handed
    // out in the same order CollectBoneCopies uses, keeping the layout aligned with
    // CreateBonePalette and with SkeletalVertex::BoneIndices.
    // Every node in the subtree, bones and plain nodes alike (CountBonesRec counts bones only).
    static UInt64 CountNodesRec(const SkeletonNode* n)
    {
        if (!n) return 0;
        UInt64 count = 1;
        for (UInt64 i = 0; i < n->Children.Size(); ++i)
            count += CountNodesRec(n->Children[i].GetRaw());
        return count;
    }

    static void FlattenForLayout(const SkeletonNode* node, Int32 parentIndex, SkeletonPoseLayout& out)
    {
        if (!node) return;

        const Int32 selfIndex = static_cast<Int32>(out.ParentIndex.Size());
        const auto* bone = dynamic_cast<const SkeletonBone*>(node);

        out.ParentIndex.PushBack(parentIndex);
        out.BoneSlot.PushBack(bone ? static_cast<Int32>(out.BoneCount) : -1);
        out.LocalMatrix.PushBack(node->LocalMatrix);
        out.LocalBindTransform.PushBack(BoneTransform::FromMatrix(node->LocalMatrix));
        out.OffsetMatrix.PushBack(bone ? bone->OffsetMatrix : Matrix4(1.0f));
        out.NodeName.PushBack(node->NodeName);
        out.NameToIndex.Insert(node->NodeName, selfIndex);

        if (bone) out.BoneCount++;

        for (UInt64 i = 0; i < node->Children.Size(); ++i)
            FlattenForLayout(node->Children[i].GetRaw(), selfIndex, out);
    }
}

const Plu::SkeletonPoseLayout& Plu::Skeleton::GetPoseLayout() const
{
    if (!mPoseLayoutBuilt)
    {
        PLU_PROFILE_SCOPE("Skeleton Pose Layout Build");
        mPoseLayout.Clear();
        if (RootNode)
        {
            const UInt64 nodeCount = CountNodesRec(RootNode.GetRaw());
            mPoseLayout.ParentIndex.Reserve(nodeCount);
            mPoseLayout.BoneSlot.Reserve(nodeCount);
            mPoseLayout.LocalMatrix.Reserve(nodeCount);
            mPoseLayout.LocalBindTransform.Reserve(nodeCount);
            mPoseLayout.OffsetMatrix.Reserve(nodeCount);
            mPoseLayout.NodeName.Reserve(nodeCount);
            FlattenForLayout(RootNode.GetRaw(), -1, mPoseLayout);
        }
        mPoseLayoutBuilt = true;
    }
    return mPoseLayout;
}

void Plu::Skeleton::InvalidatePoseLayout() const
{
    mPoseLayout.Clear();
    mPoseLayoutBuilt = false;
}

bool Plu::Skeleton::IsIdentical(Skeleton &other)
{
    // Identity is purely structural: the bone hierarchy defines a skeleton.
    // SkeletonName and Uuid are metadata and are intentionally NOT compared, so two
    // meshes that share one armature (e.g. "Body_Skeleton" vs "Clothes_Skeleton")
    // are still recognised as the same skeleton and deduped on import.
    return NodesIdentical(RootNode.GetRaw(), other.RootNode.GetRaw());
}
