//
// Created by Plutex on 6/9/26.
//

#ifndef PLUENGINE_RENDERUTILS_H
#define PLUENGINE_RENDERUTILS_H
#include <cstddef>

#include "PluEngine/Core.h"
#include "PluEngine/PluTypes.h"

namespace Plu
{
    constexpr float kCameraNearClip = 0.1f;
    constexpr float kCameraFarClip  = 100000.0f;
    // Shadow range is no longer a global constant — it is a per-light setting
    // (DirectionalLight::ShadowDistance, default 150 m) carried in the render snapshot.

    // Maximum number of directional cascades. Sizes both the GLSL arrays inside the ShadowData
    // uniform block and the shadow map texture array, so it is a hard upper bound — the runtime
    // count (DirectionalLight::ShadowCascadeCount) is clamped to it.
    constexpr Int32 kMaxShadowCascades = 4;

    // Dane pojedynczej kaskady cienia dla światła kierunkowego (CSM)
    struct ShadowCascadeData
    {
        Matrix4 ViewProj;       // lightProj * lightView dla tej kaskady
        float   SplitDistance;  // odległość (w przestrzeni widoku kamery) końca tej kaskady
        // World-space size of one shadow-map texel in this cascade (2*Radius / Resolution).
        // Drives the receiver-side normal offset — a bias expressed in texels is the only one
        // that stays correct across cascades of wildly different world extents.
        float   TexelWorldSize = 0.0f;
        // Radius of the bounding sphere of this cascade's camera sub-frustum, in metres.
        // Also the half-extent of the ortho projection in x/y.
        float   Radius = 0.0f;
        // Depth range (zFar - zNear) of this cascade's ortho projection, in metres. Converts a
        // world-space depth bias into the cascade's [0,1] depth: bias01 = biasMetres / DepthRange.
        float   DepthRange = 0.0f;
    };

    // std140 mirror of the `ShadowData` uniform block (binding 2) declared in PBR.frag.
    // Layout rules that matter here: every vec4/mat4 is 16 B aligned and an array of mat4 has a
    // 64 B stride, so all the vector/matrix members come first and the scalars are packed at the
    // end — that way no padding hides between fields and the C++ offsets match GLSL exactly
    // (asserted below).
    struct ShadowDataGPU
    {
        Matrix4 CascadeViewProj[kMaxShadowCascades];  //   0, 4 x 64 B
        Vec4    CascadeSplits;                        // 256, view-space end distance per cascade
        Vec4    CascadeTexelSizes;                    // 272, world size of one texel per cascade
        Vec4    CascadeDepthBias;                     // 288, depth bias in [0,1] depth per cascade
        Int32   CascadeCount;                         // 304, 0 = no directional shadows this frame
        float   ShadowFadeStart;                      // 308, metres; shadows start fading out here
        float   ShadowFadeEnd;                        // 312, metres; fully lit past this distance
        float   CascadeBlendFraction;                 // 316, fraction of a cascade used to cross-fade
        float   NormalBiasScale;                      // 320, normal offset in texels
        float   PcfRadiusTexels;                      // 324, PCF disk radius in texels
        Int32   DebugVisualizeCascades;               // 328, != 0 tints each cascade
        // 332, 1 / shadow map resolution. Passed explicitly instead of calling textureSize() in
        // the shader: textureSize on a shadow sampler is broken on some older drivers, and this
        // slot was padding anyway.
        float   InvShadowMapResolution;
        Int32   PcfTapCount;                          // 336, samples in the PCF disk (1 = one hardware tap)
        Int32   PcfRotateSamples;                     // 340, != 0 rotates the disk per pixel
        float   Padding[2];                           // 344, std140 rounds the block up to a multiple of 16
    };

    static_assert(sizeof(ShadowDataGPU) == 352, "ShadowDataGPU must match the std140 ShadowData block");
    static_assert(offsetof(ShadowDataGPU, CascadeSplits) == 256);
    static_assert(offsetof(ShadowDataGPU, CascadeTexelSizes) == 272);
    static_assert(offsetof(ShadowDataGPU, CascadeDepthBias) == 288);
    static_assert(offsetof(ShadowDataGPU, CascadeCount) == 304);
    static_assert(offsetof(ShadowDataGPU, DebugVisualizeCascades) == 328);
    static_assert(offsetof(ShadowDataGPU, PcfTapCount) == 336);
    static_assert(offsetof(ShadowDataGPU, PcfRotateSamples) == 340);

    // --- Spot lights ---

    // Hard cap on spot lights reaching the GPU in one frame. Sizes the SSBO (binding 5); lights
    // past it are dropped on MAIN by importance, so the ones that survive are the ones that
    // matter. Unrelated to the shadow budget below — a light can be lit without being shadowed.
    constexpr Int32 kMaxVisibleSpotLights = 64;
    // Layers of the spot shadow atlas, i.e. how many spot lights may cast a shadow in one frame.
    constexpr Int32 kMaxSpotShadowSlots   = 8;
    // Per-slot resolution. 1024² x 8 layers of D32F is ~32 MB — a fixed cost paid once, which is
    // the point of a fixed pool: shadow VRAM does not grow with the number of lights in a scene.
    constexpr Int32 kSpotShadowResolution = 1024;
    // Near plane of a spot's shadow projection, in metres. Close enough to the cone apex that no
    // real caster falls in front of it, which matters because the spot depth pass deliberately
    // does NOT enable GL_DEPTH_CLAMP (see Renderer::RenderSpotShadowPass).
    constexpr float kSpotShadowNearClip   = 0.05f;

    // std430 mirror of the `SpotLightGPU` struct in the `SpotLights` buffer (binding 5) declared
    // in PBR.frag. std430 aligns a vec3 to 16 B and lets the following scalar fill the gap, which
    // is exactly how the (vec3, float) pairs below are packed — no padding hides between them.
    struct SpotLightGPU
    {
        Matrix4 ShadowViewProj;            //   0, 64 B — identity when the light got no slot
        Vec3    Position;                  //  64
        float   Range;                     //  76, metres
        Vec3    Direction;                 //  80, direction of travel
        float   InnerConeCos;              //  92
        Vec3    Color;                     //  96, colour * intensity, premultiplied on MAIN
        float   OuterConeCos;              // 108
        Int32   ShadowSlot;                // 112, -1 = no shadow map this frame
        float   ShadowDepthBias;           // 116, METRES (see ShadowDepthBiasScale)
        float   ShadowNormalBias;          // 120, texels
        float   ShadowPcfRadius;           // 124, texels
        // 128, world size of one shadow texel PER METRE of distance from the light:
        // 2*tan(outerHalfAngle) / resolution. A perspective shadow map's texel grows with
        // distance — the problem CSM does not have — so the receiver-side normal offset has to be
        // scaled by the fragment's distance in the shader instead of by a per-cascade constant.
        float   ShadowTexelWorldPerMetre;
        Int32   ShadowPcfTaps;             // 132
        // 136, converts ShadowDepthBias from metres into this projection's [0,1] depth:
        // bias01 = ShadowDepthBias * ShadowDepthBiasScale / viewDepth².
        //
        // A perspective depth buffer is non-linear (z01 = f/(f-n) * (1 - n/d)), so a CONSTANT
        // bias in [0,1] depth is a constant only at one distance — at 5 m of a 15 m cone it
        // already means ~0.75 m of world offset and the shadow silently stops existing. The
        // exact derivative dz01/dd = f*n/((f-n)*d²) turns that back into a fixed physical
        // distance, which is what "2 cm of bias" has to mean everywhere in the cone.
        // Numerator f*n/(f-n) is per-light and constant, so it is precomputed here.
        float   ShadowDepthBiasScale;
        float   Padding;                   // 140, std430 rounds the struct to a multiple of 16
    };

    static_assert(sizeof(SpotLightGPU) == 144, "SpotLightGPU must match the std430 SpotLights element");
    static_assert(offsetof(SpotLightGPU, ShadowDepthBiasScale) == 136);
    static_assert(offsetof(SpotLightGPU, Position) == 64);
    static_assert(offsetof(SpotLightGPU, Direction) == 80);
    static_assert(offsetof(SpotLightGPU, Color) == 96);
    static_assert(offsetof(SpotLightGPU, ShadowSlot) == 112);
    static_assert(offsetof(SpotLightGPU, ShadowTexelWorldPerMetre) == 128);
    static_assert(offsetof(SpotLightGPU, ShadowPcfTaps) == 132);

    // std140 mirror of the `SpotLightData` uniform block (binding 4) in PBR.frag.
    //
    // Offset/Count are the whole investment in clustered forward: today the index buffer
    // (binding 6) is one global list and Offset is 0, so the shader walks every visible light.
    // Adding clusters later only changes how these two are computed — the loop in PBR.frag does
    // not change at all.
    struct SpotLightDataGPU
    {
        Int32 SpotLightOffset       = 0;  //  0, where this draw's index list starts
        Int32 SpotLightCount        = 0;  //  4, 0 = no spot lights this frame
        float InvSpotShadowResolution = 0.0f;  // 8, 1 / atlas slot resolution
        Int32 SpotShadowSlotCount   = 0;  // 12, 0 = spot shadows unavailable this frame
    };

    static_assert(sizeof(SpotLightDataGPU) == 16, "SpotLightDataGPU must match the std140 SpotLightData block");

    // Bounding sphere of a cone, for culling a spot light against the camera frustum. Much
    // tighter than a Range-radius sphere around the apex, and this is the ONLY test deciding
    // whether a light is sent to the GPU at all, so the tightness is the whole saving.
    //
    // Two cases, because the smallest enclosing sphere changes character at 45°: for a narrow
    // cone (half-angle <= 45°) the sphere touches the apex and the base rim, centred on the axis
    // at Range / (2*cos²θ); for a wide one the base circle itself is the widest part, so the
    // sphere is centred at Range*cosθ with radius Range*sinθ.
    PLU_API void ComputeSpotBoundingSphere(const Vec3& Apex, const Vec3& Dir, float Range,
                                           float HalfAngleRadians, Vec3& OutCenter, float& OutRadius);

    // Light-space matrix of a spot's shadow map: a square perspective projection covering the
    // full cone, from the apex along Dir, with far = Range. The "up" vector is picked as the
    // world axis least parallel to Dir so lookAt does not degenerate for a light shining
    // straight down — the common case for a lamp.
    PLU_API Matrix4 ComputeSpotLightMatrix(const Vec3& Apex, const Vec3& Dir, float Range,
                                           float OuterHalfAngleRadians);

    // Appends a cone wireframe (base circle + Segments spokes from the apex) to an interleaved
    // pos(3)+color(3) line buffer — the same format the physics debug renderer packs into, so it
    // renders through the existing debug-line pass with no new shader and no new renderer code.
    PLU_API void AppendConeWireframe(DynamicArray<float>& OutLineVerts, const Vec3& Apex, const Vec3& Dir,
                                     float Range, float HalfAngleRadians, const Vec3& Color, Int32 Segments = 24);

    // Upper bound on PCF samples per cascade, mirrored by MAX_PCF_TAPS in PBR.frag. The disk is
    // generated analytically (Vogel), so this only caps the shader loop — there is no sample table.
    constexpr Int32 kMaxShadowPcfTaps = 32;

    // Number of PCF taps that just covers a disk of PcfRadiusTexels without leaving gaps.
    // One tap is a hardware 2x2 comparison, i.e. it already averages ~1 texel², so a disk of
    // area pi*r² texels needs ceil(pi*r²) of them — below that the individual taps show up as
    // rings on a soft edge, above it every extra tap re-reads what a neighbour already covered.
    // Clamped to [1, kMaxShadowPcfTaps]; radius 0 collapses the filter to a single tap anyway.
    Int32 ComputeAutoPcfTapCount(float PcfRadiusTexels);

    // Per-frame configuration of the directional cascade set. Comes from the
    // DirectionalLight shadow settings carried in the render snapshot.
    struct CascadeConfig
    {
        Int32 CascadeCount   = 4;
        float ShadowDistance = 150.0f;  // metres; cascades cover [nearClip, ShadowDistance]
        float SplitLambda    = 0.9f;    // 0 = uniform split, 1 = logarithmic split
        Int32 Resolution     = 2048;    // shadow map resolution, identical for every cascade
    };

    DynamicArray<Vec3> GetFrustumCornersWorldSpace(const Matrix4& proj, const Matrix4& view);

    // Non-allocating variant of the above — writes the 8 corners into OutCorners.
    // Used on the cascade path, which must not allocate per frame.
    void GetFrustumCornersWorldSpace(const Matrix4& proj, const Matrix4& view, Vec3 OutCorners[8]);

    // --- Cascaded Shadow Maps ---

    // Tworzy macierz projekcji perspektywicznej dla pod-frustum jednej kaskady
    // (te same fov/aspect co kamera główna, ale inny near/far).
    Matrix4 GetCascadeProjectionMatrix(float fovYRadians, float aspect, float nearPlane, float farPlane);

    // Splits [NearClip, Config.ShadowDistance] into Config.CascadeCount view-space distances
    // (practical split scheme; SplitLambda blends logarithmic and uniform). Writes into
    // OutSplits, which is cleared but keeps its capacity — no per-frame allocation.
    void ComputeCascadeSplits(const CascadeConfig& Config, float NearClip, DynamicArray<float>& OutSplits);

    // Builds the light matrices for every cascade. Writes into OutCascades (cleared, capacity
    // kept — no per-frame allocation). Splits must hold Config.CascadeCount ascending distances
    // (i.e. the output of ComputeCascadeSplits).
    //
    // Stability: the light basis is fixed (it looks along LightDir from a fixed origin, nothing
    // camera-derived), the sub-frustum bounding-sphere radius is rounded up to a 1/16 m grid so
    // it stops pulsing, and the cascade centre is snapped to that cascade's texel grid inside
    // the fixed basis. Together these keep shadow edges pixel-stable while the camera moves —
    // a snap performed in a camera-derived basis (the old code) is a mathematical no-op.
    //
    // No near margin is added: casters between the light and the cascade are handled by
    // GL_DEPTH_CLAMP ("pancaking") in the shadow pass, which costs no depth precision.
    //
    // PerCascadeResolutions: optional override of Config.Resolution, one entry per cascade,
    // for the legacy mixed-resolution cascade set. Null = Config.Resolution everywhere.
    void ComputeCascadeMatrices(
        const Matrix4& CameraView,
        float FovYRadians, float Aspect,
        float NearClip,
        const Vec3& LightDir,
        const CascadeConfig& Config,
        const DynamicArray<float>& Splits,
        DynamicArray<ShadowCascadeData>& OutCascades,
        const Int32* PerCascadeResolutions = nullptr);

    // --- Frustum culling ---

    struct Frustum { Vec4 Planes[6]; };  // (nx,ny,nz,d), wewnątrz gdy dot(n,p)+d >= 0

    PLU_API Frustum ExtractFrustumPlanes(const Matrix4& viewProj);  // Gribb-Hartmann, znormalizowane
    PLU_API bool    SphereInFrustum(const Frustum& frustum, const Vec3& center, float radius);

    // Same test, ignoring the near plane. This is the correct one for culling shadow casters
    // against a cascade: the shadow pass renders with GL_DEPTH_CLAMP, so a caster in front of
    // the ortho near plane is pancaked ONTO it and still occludes. Testing the near plane would
    // cull exactly the objects between the light and the scene — the ones casting the shadows.
    PLU_API bool    SphereInFrustumNoNear(const Frustum& frustum, const Vec3& center, float radius);
}

#endif //PLUENGINE_RENDERUTILS_H
