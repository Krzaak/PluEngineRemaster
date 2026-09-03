//
// Created by Plutex on 8/12/26.
//

#ifndef PLUENGINE_PHYSICSUTILS_H
#define PLUENGINE_PHYSICSUTILS_H
#include "Jolt/Jolt.h"
#include "Jolt/Math/Real.h"

#include "PluEngine/PluTypes.h"

namespace Plu
{
    // Jolt <-> GLM conversions. They live here, not in PluUtils, because including Jolt from a
    // public PluCore header means every module in the engine pulls it in and the bottom layer has
    // to link a physics library it never uses.
    //
    // Kept inline: they are two field copies each, called per body per frame.
    inline JPH::RVec3 ToJPH(const Vec3& V) {
        return {V.x, V.y, V.z};
    }

    inline Vec3 ToGLM(const JPH::RVec3& V) {
        return {V.GetX(), V.GetY(), V.GetZ()};
    }

    inline JPH::Vec3 ToJPHVec3(const Vec3& V) {
        return {V.x, V.y, V.z};
    }

    inline Vec3 ToGLMFromVec3(const JPH::Vec3& V) {
        return {V.GetX(), V.GetY(), V.GetZ()};
    }
}

#endif //PLUENGINE_PHYSICSUTILS_H
