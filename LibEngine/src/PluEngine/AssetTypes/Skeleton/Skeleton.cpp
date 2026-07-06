//
// Created by Plutex on 7/6/26.
//

#include "PluEngine/AssetTypes/Skeleton/Skeleton.h"

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

bool Plu::Skeleton::IsIdentical(Skeleton &other)
{
    // Identity is purely structural: the bone hierarchy defines a skeleton.
    // SkeletonName and Uuid are metadata and are intentionally NOT compared, so two
    // meshes that share one armature (e.g. "Body_Skeleton" vs "Clothes_Skeleton")
    // are still recognised as the same skeleton and deduped on import.
    return NodesIdentical(RootNode.GetRaw(), other.RootNode.GetRaw());
}
