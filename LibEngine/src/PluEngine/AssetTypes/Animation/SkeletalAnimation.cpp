//
// Created by Plutex on 7/7/26.
//

#include "PluEngine/AssetTypes/Animation/SkeletalAnimation.h"

#include <glm/gtc/quaternion.hpp>

namespace
{
    // Index of the last key with Timestamp <= time, or -1 when time precedes all keys.
    template <typename KeyT>
    Int64 LastKeyAtOrBefore(const DynamicArray<KeyT>& keys, double time)
    {
        Int64 lo = 0;
        Int64 hi = static_cast<Int64>(keys.Size()) - 1;
        Int64 result = -1;
        while (lo <= hi)
        {
            const Int64 mid = lo + (hi - lo) / 2;
            if (keys[static_cast<UInt64>(mid)].Timestamp <= time)
            {
                result = mid;
                lo = mid + 1;
            }
            else
            {
                hi = mid - 1;
            }
        }
        return result;
    }

    // 0..1 blend factor between two neighbouring keys (0 when they share a timestamp).
    template <typename KeyT>
    float BlendFactor(const KeyT& a, const KeyT& b, double time)
    {
        const double span = b.Timestamp - a.Timestamp;
        return span > 0.0 ? static_cast<float>((time - a.Timestamp) / span) : 0.0f;
    }
}

Vec3 Plu::AnimationTrack::GetLocationAtTime(double timeTicks, const Vec3& fallback) const
{
    if (LocationKeys.IsEmpty()) return fallback;

    const Int64 i = LastKeyAtOrBefore(LocationKeys, timeTicks);
    if (i < 0) return LocationKeys.Front().Value;
    if (i >= static_cast<Int64>(LocationKeys.Size()) - 1) return LocationKeys.Back().Value;

    const AnimationVectorKey& a = LocationKeys[static_cast<UInt64>(i)];
    const AnimationVectorKey& b = LocationKeys[static_cast<UInt64>(i) + 1];
    return glm::mix(a.Value, b.Value, BlendFactor(a, b, timeTicks));
}

Quaternion Plu::AnimationTrack::GetRotationAtTime(double timeTicks, const Quaternion& fallback) const
{
    if (RotationKeys.IsEmpty()) return fallback;

    const Int64 i = LastKeyAtOrBefore(RotationKeys, timeTicks);
    if (i < 0) return RotationKeys.Front().Value;
    if (i >= static_cast<Int64>(RotationKeys.Size()) - 1) return RotationKeys.Back().Value;

    const AnimationQuatKey& a = RotationKeys[static_cast<UInt64>(i)];
    const AnimationQuatKey& b = RotationKeys[static_cast<UInt64>(i) + 1];
    return glm::normalize(glm::slerp(a.Value, b.Value, BlendFactor(a, b, timeTicks)));
}

Vec3 Plu::AnimationTrack::GetScaleAtTime(double timeTicks, const Vec3& fallback) const
{
    if (ScaleKeys.IsEmpty()) return fallback;

    const Int64 i = LastKeyAtOrBefore(ScaleKeys, timeTicks);
    if (i < 0) return ScaleKeys.Front().Value;
    if (i >= static_cast<Int64>(ScaleKeys.Size()) - 1) return ScaleKeys.Back().Value;

    const AnimationVectorKey& a = ScaleKeys[static_cast<UInt64>(i)];
    const AnimationVectorKey& b = ScaleKeys[static_cast<UInt64>(i) + 1];
    return glm::mix(a.Value, b.Value, BlendFactor(a, b, timeTicks));
}

void Plu::AnimationTrack::SortKeys()
{
    const auto byTime = [](const auto& a, const auto& b) { return a.Timestamp < b.Timestamp; };
    LocationKeys.Sort(byTime);
    RotationKeys.Sort(byTime);
    ScaleKeys.Sort(byTime);
}
