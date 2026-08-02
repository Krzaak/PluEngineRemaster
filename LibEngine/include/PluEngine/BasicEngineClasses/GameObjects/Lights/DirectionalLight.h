//
// Created by Plutex on 4/19/26.
//

#ifndef PLUENGINE_DIRECTIONALLIGHT_H
#define PLUENGINE_DIRECTIONALLIGHT_H
#include "LightBaseObject.h"
#include "PluEngine/Core.h"
#include "DirectionalLight.generated.h"

namespace Plu
{
    PLU_CLASS()
    class PLU_API DirectionalLight : public LightBaseObject
    {
        REFLECTION_BODY_DIRECTIONALLIGHT()
    public:
        DirectionalLight() = default;
        virtual ~DirectionalLight() override = default;

        // --- Cascaded shadow map settings ---
        // Copied into the render snapshot each frame (DirectionalLightShadowSettings) and clamped
        // by the renderer, which rebuilds its GL resources when the resolution or cascade count
        // changes. Editing them live in the details panel takes effect on the next frame.

        PLU_PROPERTY(PyExport)
        bool CastShadows = true;

        // Metres. The cascades cover [camera near clip, ShadowDistance]; past it shadows fade out.
        // 150 m puts the last cascade's texel at ~7 cm (at 2048²) instead of the ~15 cm you get at
        // 300 m — raise it per scene when a level actually needs shadows that far out.
        PLU_PROPERTY(PyExport)
        float ShadowDistance = 150.0f;

        // 1..Plu::kMaxShadowCascades. More cascades = finer texels up close, one extra depth pass each.
        PLU_PROPERTY(PyExport)
        Int32 ShadowCascadeCount = 4;

        // 0 = uniform split (equal slices), 1 = logarithmic (much denser near the camera).
        PLU_PROPERTY(PyExport)
        float ShadowSplitLambda = 0.9f;

        // Per-cascade shadow map resolution; clamped to one of 512 / 1024 / 2048 / 4096.
        PLU_PROPERTY(PyExport)
        Int32 ShadowResolution = 2048;

        // Receiver-side normal offset, in texels of the cascade. Raise it to kill acne on grazing
        // surfaces, at the cost of shadows detaching from their caster (peter-panning).
        PLU_PROPERTY(PyExport)
        float ShadowNormalBias = 1.0f;

        // Receiver-side depth bias, in METRES of world space. Constant physical distance in every
        // cascade — the renderer converts it into each cascade's depth range.
        PLU_PROPERTY(PyExport)
        float ShadowDepthBias = 0.005f;

        // Radius of the PCF Poisson disk, in texels. Larger = softer, blurrier shadow edges.
        PLU_PROPERTY(PyExport)
        float ShadowPcfRadius = 1.5f;

        // Fraction of each cascade used to cross-fade into the next one (0 = hard seam).
        PLU_PROPERTY(PyExport)
        float ShadowCascadeBlend = 0.15f;
    };
}

#endif //PLUENGINE_DIRECTIONALLIGHT_H
