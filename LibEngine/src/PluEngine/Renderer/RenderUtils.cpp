//
// Created by Plutex on 6/9/26.
//

#include "PluEngine/Renderer/RenderUtils.h"

#include <cfloat>

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

namespace Plu
{
    DynamicArray<Vec3> GetFrustumCornersWorldSpace(const Matrix4& proj, const Matrix4& view)
    {
        const Matrix4 inv = glm::inverse(proj * view);
        DynamicArray<Vec3> corners;

        for (int x = 0; x < 2; x++)
            for (int y = 0; y < 2; y++)
                for (int z = 0; z < 2; z++)
                {
                    Vec4 pt = inv * Vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                    corners.EmplaceBack(Vec3(pt) / pt.w);
                }
        return corners;
    }

    Matrix4 GetLightViewMatrix(const DynamicArray<Vec3>& corners, const Vec3& lightDir)
    {
        Vec3 center = Vec3(0.0f);
        for (const auto& c : corners)
            center += c;
        center /= static_cast<float>(corners.Size());

        Vec3 normalizedLightDir = glm::normalize(lightDir);
        Vec3 up = (glm::abs(glm::dot(normalizedLightDir, Vec3(0.0f, 1.0f, 0.0f))) > 0.99f)
              ? Vec3(0.0f, 0.0f, 1.0f)
              : Vec3(0.0f, 1.0f, 0.0f);

        return glm::lookAt(center - normalizedLightDir, center, up);
    }

    Matrix4 GetLightProjectionMatrix(const DynamicArray<Vec3>& corners, const Matrix4& lightView)
    {
        float minX =  FLT_MAX, maxX = -FLT_MAX;
        float minY =  FLT_MAX, maxY = -FLT_MAX;
        float minZ =  FLT_MAX, maxZ = -FLT_MAX;

        for (const auto& c : corners)
        {
            Vec4 ls = lightView * Vec4(c, 1.0f);
            minX = std::min(minX, ls.x); maxX = std::max(maxX, ls.x);
            minY = std::min(minY, ls.y); maxY = std::max(maxY, ls.y);
            minZ = std::min(minZ, ls.z); maxZ = std::max(maxZ, ls.z);
        }

        // Odsuwamy tylko zNear w stronę źródła światła (w przestrzeni widoku światła to kierunek dodatni osi Z w konwencji odwróconej,
        // ale w surowych współrzędnych "maxZ" oznacza punkt najbliżej światła).
        float zOffset = 50.0f;
        maxZ += zOffset;

        // Dopasowanie do konwencji GLM (zNear, zFar jako odległości dodatnie, gdzie zNear < zFar przed kamerą)
        // Dla standardowego OpenGL praworęcznego:
        return glm::ortho(minX, maxX, minY, maxY, -maxZ, -minZ);
    }
}
