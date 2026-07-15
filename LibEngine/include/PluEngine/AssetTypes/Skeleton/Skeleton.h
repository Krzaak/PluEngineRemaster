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
    struct SkeletonAttachPoint
    {
        REFLECTION_BODY_SKELETONATTACHPOINT()

        PLU_PROPERTY()
        String AttachPointName;

        PLU_PROPERTY()
        String ParentNodeName;

        //All transforms are relative to the parent node
        PLU_PROPERTY()
        Vec3 RelativeLocation;

        PLU_PROPERTY()
        Vec3 RelativeRotation;
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
        [[nodiscard]] UInt64 CountBones() const;

        [[nodiscard]] TUsePointer<SkeletonNode> FindNodeByName(const String& nodeName) const;

        // Builds a flat, DFS pre-order palette of *copies* of this skeleton's bones,
        // index-aligned with SkeletalVertex::BoneIndices (same ordering as the import
        // palette: SkeletonBone nodes in DFS pre-order, non-bone nodes skipped). The
        // copies are standalone (Children left empty) so per-instance animation can
        // overwrite their matrices without corrupting the shared skeleton asset.
        // outPalette is cleared first; a null pointer is ignored.
        void CreateBonePalette(DynamicArray<TOwningPointer<SkeletonBone>>* outPalette) const;

        // Builds a DFS pre-order palette of *copies* of every node (bones and plain
        // nodes alike), with the parent/child hierarchy preserved on the copies. Use
        // this as the animatable working tree — traverse Children to compute global
        // transforms without touching the asset. outPalette[0] is the root copy.
        // outPalette is cleared first; a null pointer is ignored.
        void CreateNodePalette(DynamicArray<TOwningPointer<SkeletonNode>>* outPalette) const;

        //Attach points
        GameHashMap<String, TOwningPointer<SkeletonAttachPoint>> AttachPoints;
    };
}

#endif //PLUENGINE_SKELETON_H
