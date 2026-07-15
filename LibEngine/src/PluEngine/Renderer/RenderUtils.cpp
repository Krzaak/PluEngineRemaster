//
// Created by Plutex on 6/9/26.
//

#include "PluEngine/Renderer/RenderUtils.h"

#include <cfloat>
#include <cmath>

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

    Matrix4 GetLightProjectionMatrix(const DynamicArray<Vec3>& corners, const Matrix4& lightView, float zOffset)
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
        maxZ += zOffset;

        // Dopasowanie do konwencji GLM (zNear, zFar jako odległości dodatnie, gdzie zNear < zFar przed kamerą)
        // Dla standardowego OpenGL praworęcznego:
        return glm::ortho(minX, maxX, minY, maxY, -maxZ, -minZ);
    }

    DynamicArray<float> GetCascadeSplits(int cascadeCount, float nearClip, float farClip, float lambda)
    {
        DynamicArray<float> splits;

        for (int i = 1; i <= cascadeCount; i++)
        {
            const float p = static_cast<float>(i) / static_cast<float>(cascadeCount);

            // Podział logarytmiczny - gęściej blisko kamery
            const float logSplit = nearClip * std::pow(farClip / nearClip, p);

            // Podział liniowy - równe odcinki
            const float uniformSplit = nearClip + (farClip - nearClip) * p;

            // Mieszanka obu wg lambda
            const float split = logSplit * lambda + uniformSplit * (1.0f - lambda);

            splits.EmplaceBack(split);
        }

        return splits;
    }

    Matrix4 GetCascadeProjectionMatrix(float fovYRadians, float aspect, float nearPlane, float farPlane)
    {
        return glm::perspective(fovYRadians, aspect, nearPlane, farPlane);
    }

    DynamicArray<ShadowCascadeData> GetCascadedLightMatrices(
        const Matrix4& cameraView,
        float fovYRadians, float aspect,
        float nearClip, float farClip,
        const Vec3& lightDir,
        const DynamicArray<float>& cascadeSplits,
        const DynamicArray<float>& shadowMapResolutions)
    {
        DynamicArray<ShadowCascadeData> cascades;

        const Vec3 nLightDir = glm::normalize(lightDir);
        const Vec3 up = (glm::abs(glm::dot(nLightDir, Vec3(0.0f, 1.0f, 0.0f))) > 0.99f)
            ? Vec3(0.0f, 0.0f, 1.0f)
            : Vec3(0.0f, 1.0f, 0.0f);

        // Marginesy zakresu z (w jednostkach świata). Near odsuwamy w stronę światła, by łapać
        // casterów spoza frustum kaskady (obiekty między światłem a widzianą sceną) — musi być
        // hojny. Far (za sferą, od strony przeciwnej do światła) trzymamy mały: casterów tam
        // praktycznie nie ma, a każdy dodatkowy zakres z rozrzedza precyzję głębi = więcej acne.
        constexpr float kZNearMargin = 50.0f;
        constexpr float kZFarMargin  = 5.0f;

        float prevSplit = nearClip;
        for (size_t i = 0; i < cascadeSplits.Size(); i++)
        {
            const float currSplit = cascadeSplits[i];

            // Pod-frustum tej kaskady to ten sam fov/aspect kamery, ale z innym near/far
            const Matrix4 cascadeProj = GetCascadeProjectionMatrix(fovYRadians, aspect, prevSplit, currSplit);
            const DynamicArray<Vec3> corners = GetFrustumCornersWorldSpace(cascadeProj, cameraView);

            // Środek frustum
            Vec3 center = Vec3(0.0f);
            for (const auto& c : corners)
                center += c;
            center /= static_cast<float>(corners.Size());

            // Promień sfery otaczającej frustum. Jest niezależny od orientacji kamery
            // (przy stałym fov/near/far), więc rozmiar mapy cienia nie "pulsuje" przy obrocie.
            float radius = 0.0f;
            for (const auto& c : corners)
                radius = std::max(radius, glm::length(c - center));

            // Macierz widoku światła patrząca na środek sfery
            const Vec3 eye = center - nLightDir * radius;
            const Matrix4 lightView = glm::lookAt(eye, center, up);

            // Texel snapping: wyrównaj środek (w przestrzeni światła) do siatki teksela,
            // dzięki czemu cień nie "pływa" przy płynnym ruchu kamery. Rozdzielczość jest
            // per-kaskada — siatka snappingu musi odpowiadać tekselom TEJ kaskady.
            const float texelsPerUnit = shadowMapResolutions[i] / (radius * 2.0f);
            Vec3 centerLS = Vec3(lightView * Vec4(center, 1.0f));
            centerLS.x = std::floor(centerLS.x * texelsPerUnit) / texelsPerUnit;
            centerLS.y = std::floor(centerLS.y * texelsPerUnit) / texelsPerUnit;

            const float minX = centerLS.x - radius;
            const float maxX = centerLS.x + radius;
            const float minY = centerLS.y - radius;
            const float maxY = centerLS.y + radius;

            // W przestrzeni widoku światła środek sfery leży na z = -radius, sfera obejmuje
            // [-2*radius, 0]. Rozszerzamy near w stronę światła (ujemny near) o margines.
            const Matrix4 lightProj = glm::ortho(minX, maxX, minY, maxY, -kZNearMargin, 2.0f * radius + kZFarMargin);

            ShadowCascadeData data;
            data.viewProj = lightProj * lightView;
            data.splitDistance = currSplit;
            cascades.EmplaceBack(data);

            prevSplit = currSplit;
        }

        return cascades;
    }
}
