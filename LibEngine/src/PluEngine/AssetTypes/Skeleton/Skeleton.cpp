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

bool Plu::Skeleton::IsIdentical(Skeleton &other)
{
    // Identity is purely structural: the bone hierarchy defines a skeleton.
    // SkeletonName and Uuid are metadata and are intentionally NOT compared, so two
    // meshes that share one armature (e.g. "Body_Skeleton" vs "Clothes_Skeleton")
    // are still recognised as the same skeleton and deduped on import.
    return NodesIdentical(RootNode.GetRaw(), other.RootNode.GetRaw());
}
