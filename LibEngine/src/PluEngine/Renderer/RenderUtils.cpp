//
// Created by Plutex on 6/9/26.
//

#include "PluEngine/Renderer/RenderUtils.h"

#include <cfloat>
#include <cmath>

#include "PluEngine/Timer.h"

#include "glm/common.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"

namespace Plu
{
    void GetFrustumCornersWorldSpace(const Matrix4& proj, const Matrix4& view, Vec3 OutCorners[8])
    {
        const Matrix4 inv = glm::inverse(proj * view);

        int index = 0;
        for (int x = 0; x < 2; x++)
            for (int y = 0; y < 2; y++)
                for (int z = 0; z < 2; z++)
                {
                    Vec4 pt = inv * Vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                    OutCorners[index++] = Vec3(pt) / pt.w;
                }
    }

    DynamicArray<Vec3> GetFrustumCornersWorldSpace(const Matrix4& proj, const Matrix4& view)
    {
        Vec3 corners[8];
        GetFrustumCornersWorldSpace(proj, view, corners);

        DynamicArray<Vec3> result;
        result.Reserve(8);
        for (const Vec3& c : corners)
            result.PushBack(c);
        return result;
    }

    Matrix4 GetCascadeProjectionMatrix(float fovYRadians, float aspect, float nearPlane, float farPlane)
    {
        return glm::perspective(fovYRadians, aspect, nearPlane, farPlane);
    }

    void ComputeCascadeSplits(const CascadeConfig& Config, float NearClip, DynamicArray<float>& OutSplits)
    {
        OutSplits.Clear();
        if (Config.CascadeCount <= 0) return;

        const float farClip = std::max(Config.ShadowDistance, NearClip + 1.0f);
        const float lambda  = glm::clamp(Config.SplitLambda, 0.0f, 1.0f);

        for (Int32 i = 1; i <= Config.CascadeCount; i++)
        {
            const float p = static_cast<float>(i) / static_cast<float>(Config.CascadeCount);

            // Logarithmic split — denser near the camera.
            const float logSplit = NearClip * std::pow(farClip / NearClip, p);
            // Uniform split — equal slices.
            const float uniformSplit = NearClip + (farClip - NearClip) * p;

            OutSplits.PushBack(logSplit * lambda + uniformSplit * (1.0f - lambda));
        }
    }

    void ComputeCascadeMatrices(
        const Matrix4& CameraView,
        float FovYRadians, float Aspect,
        float NearClip,
        const Vec3& LightDir,
        const CascadeConfig& Config,
        const DynamicArray<float>& Splits,
        DynamicArray<ShadowCascadeData>& OutCascades,
        const Int32* PerCascadeResolutions)
    {
        PLU_PROFILE_SCOPE("ComputeCascadeMatrices");

        OutCascades.Clear();
        if (Splits.IsEmpty()) return;

        const Vec3 nLightDir = glm::normalize(LightDir);
        const Vec3 up = (glm::abs(glm::dot(nLightDir, Vec3(0.0f, 1.0f, 0.0f))) > 0.99f)
            ? Vec3(0.0f, 0.0f, 1.0f)
            : Vec3(0.0f, 1.0f, 0.0f);

        // FIXED light basis: the eye sits one unit "up-light" of the world origin and looks
        // along the light direction. Nothing here is derived from the camera, so light space
        // is the same every frame as long as the sun does not rotate — which is exactly what
        // makes the texel snap below a real stabiliser rather than a no-op. (The old code put
        // the eye at `center - lightDir * radius`, which maps `center` onto the light-space
        // origin, so flooring it always yielded 0 and the basis slid with the camera.)
        const Matrix4 lightView = glm::lookAt(-nLightDir, Vec3(0.0f), up);

        // Far margin only (behind the cascade sphere, away from the light). The near side needs
        // no margin: GL_DEPTH_CLAMP in the shadow pass flattens casters in front of the near
        // plane onto it, so they still occlude without stretching the depth range (and losing
        // precision) the way the old 50 m near margin did.
        constexpr float kZFarMargin = 1.0f;

        // Quantisation of the bounding-sphere radius, in metres. Rounding the radius up to a
        // fixed grid stops the ortho extent (and therefore the texel size) from changing by a
        // hair every frame, which would defeat the snap even with a fixed basis.
        constexpr float kRadiusQuantum = 1.0f / 16.0f;

        Vec3 corners[8];
        float prevSplit = NearClip;
        for (UInt32 i = 0; i < Splits.Size(); i++)
        {
            const float currSplit = Splits[i];

            const Int32 resolution = PerCascadeResolutions ? PerCascadeResolutions[i] : Config.Resolution;
            const float resolutionF = static_cast<float>(std::max(resolution, 1));

            // This cascade's sub-frustum: the camera's fov/aspect with its own near/far.
            const Matrix4 cascadeProj = GetCascadeProjectionMatrix(FovYRadians, Aspect, prevSplit, currSplit);
            GetFrustumCornersWorldSpace(cascadeProj, CameraView, corners);

            Vec3 center = Vec3(0.0f);
            for (const Vec3& c : corners)
                center += c;
            center /= 8.0f;

            // Bounding sphere of the sub-frustum. Independent of camera orientation (at a fixed
            // fov/near/far), so the shadow map extent does not pulse when the camera rotates.
            float radius = 0.0f;
            for (const Vec3& c : corners)
                radius = std::max(radius, glm::length(c - center));
            radius = std::ceil(radius / kRadiusQuantum) * kRadiusQuantum;

            const float texelWorldSize = (2.0f * radius) / resolutionF;

            // Texel snap inside the fixed light basis — the whole point of the fixed basis.
            Vec3 centerLS = Vec3(lightView * Vec4(center, 1.0f));
            centerLS.x = std::floor(centerLS.x / texelWorldSize) * texelWorldSize;
            centerLS.y = std::floor(centerLS.y / texelWorldSize) * texelWorldSize;

            // The light looks down -z, so a point at light-space z has depth -z. The sphere
            // spans [centerLS.z - radius, centerLS.z + radius] → near = -(z + r), far = -(z - r).
            const float zNear = -(centerLS.z + radius);
            const float zFar  = -(centerLS.z - radius) + kZFarMargin;
            const Matrix4 lightProj = glm::ortho(
                centerLS.x - radius, centerLS.x + radius,
                centerLS.y - radius, centerLS.y + radius,
                zNear, zFar);

            ShadowCascadeData data;
            data.ViewProj       = lightProj * lightView;
            data.SplitDistance  = currSplit;
            data.TexelWorldSize = texelWorldSize;
            data.Radius         = radius;
            data.DepthRange     = zFar - zNear;
            OutCascades.PushBack(data);

            prevSplit = currSplit;
        }
    }

    Frustum ExtractFrustumPlanes(const Matrix4& viewProj)
    {
        // Gribb-Hartmann: każda płaszczyzna to kombinacja liniowa wierszy M, gdzie clip = M * pos.
        // Wewnątrz frustum: -w <= x <= w, -w <= y <= w, -w <= z <= w (konwencja NDC [-1,1] z glm::perspective/ortho).
        const Vec4 row0 = glm::row(viewProj, 0);
        const Vec4 row1 = glm::row(viewProj, 1);
        const Vec4 row2 = glm::row(viewProj, 2);
        const Vec4 row3 = glm::row(viewProj, 3);

        Frustum frustum;
        frustum.Planes[0] = row3 + row0; // Left
        frustum.Planes[1] = row3 - row0; // Right
        frustum.Planes[2] = row3 + row1; // Bottom
        frustum.Planes[3] = row3 - row1; // Top
        frustum.Planes[4] = row3 + row2; // Near
        frustum.Planes[5] = row3 - row2; // Far

        for (Vec4& plane : frustum.Planes) {
            const float length = glm::length(Vec3(plane));
            if (length > 0.0f) plane /= length;
        }
        return frustum;
    }

    bool SphereInFrustum(const Frustum& frustum, const Vec3& center, float radius)
    {
        for (const Vec4& plane : frustum.Planes) {
            if (glm::dot(Vec3(plane), center) + plane.w < -radius) return false;
        }
        return true;
    }

    bool SphereInFrustumNoNear(const Frustum& frustum, const Vec3& center, float radius)
    {
        // Index 4 is the near plane (see ExtractFrustumPlanes) — skipped on purpose, see the
        // declaration for why depth-clamped shadow casters live behind it.
        for (int i = 0; i < 6; i++) {
            if (i == 4) continue;
            const Vec4& plane = frustum.Planes[i];
            if (glm::dot(Vec3(plane), center) + plane.w < -radius) return false;
        }
        return true;
    }
}
