//
// Created by Plutex on 6/9/26.
//

#include "PluEngine/Renderer/RenderUtils.h"

#include <cfloat>

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

namespace Plu
{
    DynamicArray<Vec4> GetFrustumCornersWorldSpace(const Matrix4& proj, const Matrix4& view)
    {
        const Matrix4 inv = glm::inverse(proj * view);

        DynamicArray<Vec4> corners;
        for (int x = 0; x < 2; x++)
            for (int y = 0; y < 2; y++)
                for (int z = 0; z < 2; z++)
                {
                    // NDC corners: -1 lub +1 na każdej osi
                    Vec4 pt = inv * Vec4(2.0f*x-1, 2.0f*y-1, 2.0f*z-1, 1.0f);
                    corners.EmplaceBack(pt / pt.w);
                }
        return corners;
    }

    Matrix4 GetLightViewMatrix(const DynamicArray<Vec4>& corners, const Vec3& lightDir)
    {
        Vec3 center = Vec3(0);
        for (const auto& c : corners)
            center += Vec3(c);
        center /= corners.Size();

        return glm::lookAt(center - lightDir, center, Vec3(0, 1, 0));
    }

    Matrix4 GetLightProjectionMatrix(const DynamicArray<Vec4>& corners, const Matrix4& lightView)
    {
        float minX =  FLT_MAX, maxX = -FLT_MAX;
        float minY =  FLT_MAX, maxY = -FLT_MAX;
        float minZ =  FLT_MAX, maxZ = -FLT_MAX;

        for (const auto& c : corners)
        {
            Vec4 ls = lightView * c; // transformacja do light space
            minX = std::min(minX, ls.x); maxX = std::max(maxX, ls.x);
            minY = std::min(minY, ls.y); maxY = std::max(maxY, ls.y);
            minZ = std::min(minZ, ls.z); maxZ = std::max(maxZ, ls.z);
        }

        // Poszerzenie zNear/zFar żeby złapać obiekty poza frustumem rzucające cień
        float zMult = 10.0f;
        if (minZ < 0) minZ *= zMult;
        else          minZ /= zMult;
        if (maxZ < 0) maxZ /= zMult;
        else          maxZ *= zMult;

        return glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
    }
}
