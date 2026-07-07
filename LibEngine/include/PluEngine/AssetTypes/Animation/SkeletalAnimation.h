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
    struct PLU_API AnimationKeyFrame
    {
        REFLECTION_BODY_ANIMATIONKEYFRAME()

        double Timestamp;

        Vec3 Location;
        Quaternion Rotation;
        Vec3 Scale;

        bool IsLocationKeyFrame;
        bool IsScaleKeyFrame;
        bool IsRotationKeyFrame;
    };

    PLU_STRUCT()
    struct PLU_API AnimationTrack
    {
        REFLECTION_BODY_ANIMATIONTRACK()

        TUsePointer<SkeletonNode> Node;

        GameHashMap<double, AnimationKeyFrame> KeyFrames;
    };

    PLU_STRUCT()
    struct PLU_API Animation : IAssetData
    {
        REFLECTION_BODY_ANIMATION()

        TUsePointer<Skeleton> AnimationSkeleton;

        int FramesAmount;
        float FramesPerSecond;

        GameHashMap<String, AnimationTrack> Tracks;
    };
}

#endif //PLUENGINE_SKELETALANIMATION_H
