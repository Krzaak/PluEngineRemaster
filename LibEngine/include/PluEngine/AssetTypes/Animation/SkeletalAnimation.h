//
// Created by Plutex on 7/7/26.
//

#ifndef PLUENGINE_SKELETALANIMATION_H
#define PLUENGINE_SKELETALANIMATION_H

#include "PluEngine/Core.h"
#include "PluEngine/Managers/AssetsManager.h"
#include "SkeletalAnimation.generated.h"
#include "PluEngine/PluTypes.h"

namespace Plu
{
    struct Skeleton;
    struct SkeletonNode;

    PLU_STRUCT()
    struct PLU_API AnimationVectorKey
    {
        REFLECTION_BODY_ANIMATIONVECTORKEY()

        double Timestamp; // assimp ticks

        Vec3 Value;
    };

    PLU_STRUCT()
    struct PLU_API AnimationQuatKey
    {
        REFLECTION_BODY_ANIMATIONQUATKEY()

        double Timestamp; // assimp ticks

        Quaternion Value;
    };

    PLU_STRUCT()
    struct PLU_API AnimationTrack
    {
        REFLECTION_BODY_ANIMATIONTRACK()

        TUsePointer<SkeletonNode> Node;

        // Per-channel keys, sorted by Timestamp ascending (ticks). A channel may be empty
        // (FBX pivot-split nodes often animate a single component) — sampling then returns
        // the fallback, whose defaults mirror the identity components the importer assumes.
        DynamicArray<AnimationVectorKey> LocationKeys;
        DynamicArray<AnimationQuatKey> RotationKeys;
        DynamicArray<AnimationVectorKey> ScaleKeys;

        // Sample at a time in ticks: interpolates between the surrounding keys
        // (lerp, slerp for rotation) and clamps outside the key range.
        Vec3 GetLocationAtTime(double timeTicks, const Vec3& fallback = Vec3(0.0f)) const;
        Quaternion GetRotationAtTime(double timeTicks, const Quaternion& fallback = Quaternion(1.0f, 0.0f, 0.0f, 0.0f)) const;
        Vec3 GetScaleAtTime(double timeTicks, const Vec3& fallback = Vec3(1.0f)) const;

        void SortKeys();
    };

    PLU_STRUCT()
    struct PLU_API Animation : IAssetData
    {
        REFLECTION_BODY_ANIMATION()

        TUsePointer<Skeleton> AnimationSkeleton;

        int FramesAmount;      // duration in ticks; valid sample times are [0, FramesAmount]
        float FramesPerSecond; // ticks per second

        GameHashMap<String, AnimationTrack> Tracks;
    };
}

#endif //PLUENGINE_SKELETALANIMATION_H
