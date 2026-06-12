//
// Created by Plutex on 6/9/26.
//

#ifndef PLUENGINE_RENDERUTILS_H
#define PLUENGINE_RENDERUTILS_H
#include "PluEngine/PluTypes.h"

namespace Plu
{
    constexpr float kCameraNearClip = 0.1f;
    constexpr float kCameraFarClip  = 100000.0f;

    // Dane pojedynczej kaskady cienia dla światła kierunkowego (CSM)
    struct ShadowCascadeData
    {
        Matrix4 viewProj;       // lightProj * lightView dla tej kaskady
        float   splitDistance;  // odległość (w przestrzeni widoku kamery) końca tej kaskady
    };

    DynamicArray<Vec3> GetFrustumCornersWorldSpace(const Matrix4& proj, const Matrix4& view);
    Matrix4 GetLightViewMatrix(const DynamicArray<Vec3>& corners, const Vec3& lightDir);

    // zOffset pozwala kontrolować, jak daleko "w stronę światła" odsuwamy płaszczyznę near
    // (przydatne w CSM, gdzie różne kaskady mogą wymagać różnego marginesu).
    Matrix4 GetLightProjectionMatrix(const DynamicArray<Vec3>& corners, const Matrix4& lightView, float zOffset = 50.0f);

    // --- Cascaded Shadow Maps ---

    // Wyznacza odległości podziału kaskad między nearClip i farClip.
    // lambda = 0  -> podział liniowy (równe odcinki)
    // lambda = 1  -> podział logarytmiczny (gęściej blisko kamery)
    // lambda = 0.5 -> mieszanka obu (typowy "practical split scheme")
    DynamicArray<float> GetCascadeSplits(int cascadeCount, float nearClip, float farClip, float lambda = 0.5f);

    // Tworzy macierz projekcji perspektywicznej dla pod-frustum jednej kaskady
    // (te same fov/aspect co kamera główna, ale inny near/far).
    Matrix4 GetCascadeProjectionMatrix(float fovYRadians, float aspect, float nearPlane, float farPlane);

    // Liczy macierze światła (view * proj) dla każdej kaskady na podstawie kamery i kierunku światła.
    // cascadeSplits powinno mieć cascadeSplits.Size() == liczba kaskad, posortowane rosnąco,
    // gdzie ostatni element to farClip (najczęściej wynik GetCascadeSplits).
    DynamicArray<ShadowCascadeData> GetCascadedLightMatrices(
        const Matrix4& cameraView,
        float fovYRadians, float aspect,
        float nearClip, float farClip,
        const Vec3& lightDir,
        const DynamicArray<float>& cascadeSplits);
}

#endif //PLUENGINE_RENDERUTILS_H
