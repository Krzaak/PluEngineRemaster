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

        // Per-cascade shadow map resolution; clamped to one of 512 / 1024 / 2048 / 4096 / 8192.
        // This is the primary lever for sharp shadows — a shadow that still looks blocky with a
        // tight PCF radius has texels larger than screen pixels, and only more texels fix that.
        // VRAM cost is Resolution² x 4 B x CascadeCount: 268 MB at 4096/4 cascades, 1.07 GB at
        // 8192/4 — pair 8192 with a lower ShadowDistance or fewer cascades rather than defaulting to it.
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

        // Radius of the PCF disk, in texels. Larger = softer, blurrier shadow edges; 0 collapses
        // the filter to a single hardware 2x2 tap, which is the hardest edge the map can produce.
        PLU_PROPERTY(PyExport)
        float ShadowPcfRadius = 1.5f;

        // Derive the tap count from ShadowPcfRadius instead of using ShadowPcfTaps. There is
        // exactly one correct value for a given radius — ceil(pi * radius²), see
        // Plu::ComputeAutoPcfTapCount — so leaving this on is the sane default: no banding from
        // under-sampling a wide disk, no taps wasted re-reading the same texels on a tight one.
        // Turn it off only to profile or to force a specific sample budget.
        PLU_PROPERTY(PyExport)
        bool ShadowPcfAutoTaps = true;

        // Samples taken across the PCF disk, 1..Plu::kMaxShadowPcfTaps (32). One tap = no disk at
        // all. IGNORED while ShadowPcfAutoTaps is on. Raise it together with the radius: too few
        // samples over a wide radius shows the individual taps as banding.
        PLU_PROPERTY(PyExport)
        Int32 ShadowPcfTaps = 8;

        // Rotate the sample disk per pixel (interleaved gradient noise). Without it the filter
        // pattern is identical everywhere, so a small radius leaves the shadow edge stepping along
        // the cascade's texel grid — visible as blocky stairs. With it those stairs become a
        // one-pixel dither, which is what makes a sharp shadow read as sharp instead of pixelated.
        // Cost is the rotation only; turn it off to get back the old fully deterministic pattern.
        PLU_PROPERTY(PyExport)
        bool ShadowPcfRotate = true;

        // Fraction of each cascade used to cross-fade into the next one (0 = hard seam).
        PLU_PROPERTY(PyExport)
        float ShadowCascadeBlend = 0.15f;
    };
}

#endif //PLUENGINE_DIRECTIONALLIGHT_H
