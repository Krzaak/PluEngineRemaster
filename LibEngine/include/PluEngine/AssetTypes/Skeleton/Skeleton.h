//
// Created by Plutex on 7/5/26.
//

#ifndef PLUENGINE_SKELETON_H
#define PLUENGINE_SKELETON_H
#include "PluEngine/Managers/AssetsManager.h"
#include "Skeleton.generated.h"
#include "PluEngine/PluTypes.h"

namespace Plu
{
    PLU_STRUCT()
    struct SkeletonNode
    {
        REFLECTION_BODY_SKELETONNODE()

        // Polymorphic: Children may hold SkeletonBone instances via a base pointer,
        // so a virtual destructor is required for correct destruction (and enables
        // dynamic_cast used when (de)serializing the tree).
        virtual ~SkeletonNode() = default;

        PLU_STRUCT()
        String NodeName;

        Matrix4 LocalMatrix;

        DynamicArray<TOwningPointer<SkeletonNode>> Children;
    };

    PLU_STRUCT()
    struct SkeletonBone : SkeletonNode
    {
        REFLECTION_BODY_SKELETONBONE()

        Matrix4 OffsetMatrix;
    };

    PLU_STRUCT()
    struct Skeleton : IAssetData
    {
        REFLECTION_BODY_SKELETON()

        PLU_STRUCT()
        String SkeletonName;

        TOwningPointer<SkeletonNode> RootNode;

        bool IsIdentical(Skeleton& other);

        // Number of SkeletonBone nodes in the hierarchy (used as a dedup tie-breaker:
        // among identical skeletons the bone-richest one is preferred).
        UInt64 CountBones() const;
    };
}

#endif //PLUENGINE_SKELETON_H
