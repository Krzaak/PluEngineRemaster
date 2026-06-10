//
// Created by Plutex on 6/9/26.
//

#ifndef PLUENGINE_RENDERUTILS_H
#define PLUENGINE_RENDERUTILS_H
#include "PluEngine/PluTypes.h"

namespace Plu
{
    DynamicArray<Vec3> GetFrustumCornersWorldSpace(const Matrix4& proj, const Matrix4& view);
    Matrix4 GetLightViewMatrix(const DynamicArray<Vec3>& corners, const Vec3& lightDir);
    Matrix4 GetLightProjectionMatrix(const DynamicArray<Vec3>& corners, const Matrix4& lightView);
}

#endif //PLUENGINE_RENDERUTILS_H
