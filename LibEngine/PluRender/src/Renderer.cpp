//
// Created by Plutex on 6/22/26.
//

#include "PluEngine/Render/Renderer.h"

#include <glad/glad.h>
#include <cmath>
#include "PluEngine/Application.h"
#include "PluEngine/AssetCore/EngineAssetManager.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/Core/Objects/EngineObjectManager.h"
#include "PluEngine/Render/RenderThreading.h"
#include "PluEngine/Render/RenderUtils.h"
#include "PluEngine/Render/ShaderProgram.h"
#include "PluEngine/Platform/Window.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/Render/RenderingManager.h"
#include "PluEngine/Render/ShadersManager.h"
#include "EngineAssets.h"
#include "PluEngine/Render/GPUProfiler.h"
#include "PluEngine/Timer.h"
#include "PluEngine/AssetTypes/SkeletalMesh/SkeletalMesh.h"
#include "PluEngine/PluUtils.h"

namespace
{
    // Per-cascade GPU scope names, built once. GPUProfileScope takes a String, so building them
    // inline would mean a heap allocation per cascade per frame on the render thread.
    // Drains the GL error queue and logs anything in it against a label.
    //
    // Worth the call: the only other places that drain are ShaderStorageBuffer/UniformBuffer, so
    // an error raised anywhere on the shadow path used to surface under whichever buffer Update
    // ran next — a name that has nothing to do with the culprit. Calling this at each step of the
    // shadow setup makes the log name the actual failing operation.
    bool CheckShadowGLError(const char* where)
    {
        bool clean = true;
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
        {
            clean = false;
            const char* msg;
            switch (err)
            {
                case GL_INVALID_ENUM:                  msg = "INVALID_ENUM"; break;
                case GL_INVALID_VALUE:                 msg = "INVALID_VALUE"; break;
                case GL_INVALID_OPERATION:             msg = "INVALID_OPERATION"; break;
                case GL_INVALID_FRAMEBUFFER_OPERATION: msg = "INVALID_FRAMEBUFFER_OPERATION"; break;
                case GL_OUT_OF_MEMORY:                 msg = "OUT_OF_MEMORY"; break;
                default:                               msg = "UNKNOWN"; break;
            }
            PLU_CORE_ERROR("OpenGL Error at {}: {} (0x{:x})", where, msg, err);
        }
        return clean;
    }

    const DynamicArray<Plu::String>& CascadeGpuScopeNames()
    {
        static const DynamicArray<Plu::String> names = [] {
            DynamicArray<Plu::String> result;
            result.Reserve(Plu::kMaxShadowCascades);
            for (Int32 c = 0; c < Plu::kMaxShadowCascades; c++) {
                Plu::String name = "Renderer::ShadowCascade";
                name += Plu::String::FromInt(c);
                result.PushBack(name);
            }
            return result;
        }();
        return names;
    }

    // Same trick for the spot shadow slots — built once, so the per-slot GPU scope costs no
    // allocation on the render thread.
    const DynamicArray<Plu::String>& SpotShadowGpuScopeNames()
    {
        static const DynamicArray<Plu::String> names = [] {
            DynamicArray<Plu::String> result;
            result.Reserve(Plu::kMaxSpotShadowSlots);
            for (Int32 s = 0; s < Plu::kMaxSpotShadowSlots; s++) {
                Plu::String name = "Renderer::SpotShadowSlot";
                name += Plu::String::FromInt(s);
                result.PushBack(name);
            }
            return result;
        }();
        return names;
    }
}

Plu::TUsePointer<Plu::FrameBuffer> Plu::Renderer::GetMainFrameBuffer()
{
    return mMainBuffer;
}

void Plu::Renderer::Initialize(ApplicationInfo *applicationInfo)
{
    mApplicationInfo = applicationInfo;
    EngineObjectHandle hdl = mApplicationInfo->AppObjectManager->CreateObject<FrameBuffer>();
    mMainBuffer = mApplicationInfo->AppObjectManager->GetObjectAsOwner<FrameBuffer>(hdl);
    TUsePointer<IWindow> window = mApplicationInfo->AppWindow;
    mMainBuffer->Create(window->GetWidth(), window->GetHeight(), mApplicationInfo->AppObjectManager, FrameBufferType::ColorDepth);

    // GL 4.5 guarantees at least 16 texture image units per stage, so kShadowTextureUnit (15) is
    // always legal — but a driver reporting less would silently drop every shadow lookup, which
    // is exactly the kind of failure worth naming out loud rather than debugging from pixels.
    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    mMaxTextureSize = static_cast<Int32>(maxTextureSize);

    GLint maxTextureUnits = 0;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
    if (maxTextureUnits <= static_cast<GLint>(kShadowTextureUnit)) {
        PLU_CORE_ERROR("Renderer::Initialize - GL_MAX_TEXTURE_IMAGE_UNITS is {}, but the shadow map array needs unit {}. "
                       "Directional shadows will not sample correctly.", maxTextureUnits, kShadowTextureUnit);
    }

    // Shadow atlas + its framebuffer, created eagerly at the default settings — the GL context is
    // on the render thread here, so the frame path only allocates GL objects when a light's
    // settings actually change the atlas geometry.
    {
        DirectionalLightShadowSettings defaults;
        defaults.Resolution = kDefaultShadowResolution;
        BuildCascadeAtlas(ClampShadowSettings(defaults));
    }

    // Spot shadow atlas — same deal, but its geometry is a pair of engine constants rather than
    // a per-light setting, so it is created once here and never rebuilt.
    RecreateSpotShadowResources();

    // Comparison sampler for the lighting pass. LINEAR + COMPARE_REF_TO_TEXTURE is what turns a
    // single texture() fetch into a bilinear 2x2 depth comparison (hardware PCF); the white
    // border makes everything outside a cascade read as lit.
    mShadowCompareSampler.Create();
    mShadowCompareSampler.SetFilter(GL_LINEAR, GL_LINEAR);
    constexpr float kShadowBorder[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    mShadowCompareSampler.SetWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, kShadowBorder);
    mShadowCompareSampler.SetCompareMode(GL_LEQUAL);

    // VAO/VBO debugowej geometrii fizyki — kontekst GL jest tu na wątku renderu.
    // Layout per wierzchołek: pos(3) + color(3), stride 6 floatów.
    glGenVertexArrays(1, &mDebugVao);
    glGenBuffers(1, &mDebugVbo);
    glBindVertexArray(mDebugVao);
    glBindBuffer(GL_ARRAY_BUFFER, mDebugVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    // Empty VAO for the attribute-less editor grid pass (see mGridVao in Renderer.h).
    glGenVertexArrays(1, &mGridVao);

    mSkeletalMatricesBuffer.Create(100);
    mInstanceBuffer.Create(100);

    // Shadow parameter block (binding 2). Bound once here — glBindBufferBase survives program
    // switches, and the buffer object is never reallocated (Update rewrites it in place), so
    // the binding stays valid for the whole run.
    mShadowDataBuffer.Create();
    mShadowDataBuffer.BindBase(2);

    // Spot light blocks (UBO 4, SSBO 5, SSBO 6). Allocated at the hard cap and bound once: MAIN
    // never sends more than kMaxVisibleSpotLights, so these buffers never reallocate and their
    // indexed bindings stay valid for the whole run (a Resize would create a new buffer ID and
    // silently leave the binding point holding the deleted one).
    mSpotLightDataBuffer.Create();
    mSpotLightDataBuffer.BindBase(4);
    mSpotLightBuffer.Create(kMaxVisibleSpotLights);
    mSpotLightBuffer.BindBase(5);
    mSpotLightIndexBuffer.Create(kMaxVisibleSpotLights);
    mSpotLightIndexBuffer.BindBase(6);
}

void Plu::Renderer::RecreateSpotShadowResources()
{
    if (mSpotShadowResolution == kSpotShadowResolution
        && mSpotShadowSlotCount == kMaxSpotShadowSlots
        && mSpotShadowArray) {
        return;
    }

    DestroySpotShadowResources();

    CheckShadowGLError("Renderer::RecreateSpotShadowResources (entry)");

    EngineObjectHandle textureHandle = mApplicationInfo->AppObjectManager->CreateObject<Texture>();
    mSpotShadowArray = mApplicationInfo->AppObjectManager->GetObjectAsOwner<Texture>(textureHandle);
    if (!mSpotShadowArray->CreateDepthArray(kSpotShadowResolution, kSpotShadowResolution, kMaxSpotShadowSlots)
        || !CheckShadowGLError("Renderer::RecreateSpotShadowResources (depth array)")) {
        PLU_CORE_ERROR("Renderer::RecreateSpotShadowResources - Failed to create the spot shadow atlas ({}x{}, {} slots)",
                       kSpotShadowResolution, kSpotShadowResolution, kMaxSpotShadowSlots);
        DestroySpotShadowResources();
        return;
    }

    mSpotShadowFrameBuffers.Reserve(static_cast<UInt32>(kMaxSpotShadowSlots));
    for (Int32 slot = 0; slot < kMaxSpotShadowSlots; slot++) {
        EngineObjectHandle fbHandle = mApplicationInfo->AppObjectManager->CreateObject<FrameBuffer>();
        TOwningPointer<FrameBuffer> fb = mApplicationInfo->AppObjectManager->GetObjectAsOwner<FrameBuffer>(fbHandle);
        // Same reasoning as the cascade layers: a framebuffer that failed to create silently
        // no-ops its Clear()/Bind(), so the slot would never be written and every receiver
        // sampling it would read "occluded". Drop spot shadows entirely instead.
        if (!fb->CreateWithDepthTextureLayer(mSpotShadowArray, slot, mApplicationInfo->AppObjectManager)
            || !CheckShadowGLError("Renderer::RecreateSpotShadowResources (slot framebuffer)")) {
            PLU_CORE_ERROR("Renderer::RecreateSpotShadowResources - Failed to create the framebuffer for spot shadow slot {} — spot shadows disabled", slot);
            mApplicationInfo->AppObjectManager->DestroyObject(fb->GetObjectHandle());
            fb->Destroy();
            DestroySpotShadowResources();
            return;
        }
        mSpotShadowFrameBuffers.PushBack(fb);
    }

    mSpotShadowResolution = kSpotShadowResolution;
    mSpotShadowSlotCount  = kMaxSpotShadowSlots;

    PLU_CORE_INFO("Spot shadow resources ready: {}x{} D32F array, {} slots", kSpotShadowResolution, kSpotShadowResolution, kMaxSpotShadowSlots);
}

void Plu::Renderer::DestroySpotShadowResources()
{
    // Framebuffers first — they reference the atlas texture and must not outlive it.
    for (UInt32 s = 0; s < mSpotShadowFrameBuffers.Size(); s++) {
        if (!mSpotShadowFrameBuffers[s]) continue;
        mApplicationInfo->AppObjectManager->DestroyObject(mSpotShadowFrameBuffers[s]->GetObjectHandle());
        mSpotShadowFrameBuffers[s]->Destroy();
        mSpotShadowFrameBuffers[s] = nullptr;
    }
    mSpotShadowFrameBuffers.Clear();

    if (mSpotShadowArray) {
        mApplicationInfo->AppObjectManager->DestroyObject(mSpotShadowArray->GetObjectHandle());
        mSpotShadowArray->Destroy();
        mSpotShadowArray = nullptr;
    }

    mSpotShadowResolution = -1;
    mSpotShadowSlotCount  = -1;
}

void Plu::Renderer::UnbindSpotShadowTexture()
{
    glActiveTexture(GL_TEXTURE0 + kSpotShadowTextureUnit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    SamplerObject::Unbind(kSpotShadowTextureUnit);
}

void Plu::Renderer::RequestSpotShadowView(Int32 slot)
{
    mSpotShadowViewRequest.store(slot, std::memory_order_relaxed);
}

Plu::TUsePointer<Plu::Texture> Plu::Renderer::GetSpotShadowView()
{
    return mSpotShadowView;
}

void Plu::Renderer::UpdateSpotShadowSlotView()
{
    const Int32 slot = mSpotShadowViewRequest.load(std::memory_order_relaxed);
    if (slot < 0 || !mSpotShadowArray || slot >= mSpotShadowSlotCount) return;

    PLU_PROFILE_SCOPE("Renderer::UpdateSpotShadowSlotView");

    if (!mSpotShadowView || mSpotShadowView->GetWidth() != mSpotShadowResolution) {
        if (mSpotShadowView) {
            mApplicationInfo->AppObjectManager->DestroyObject(mSpotShadowView->GetObjectHandle());
            mSpotShadowView->Destroy();
            mSpotShadowView = nullptr;
        }
        EngineObjectHandle handle = mApplicationInfo->AppObjectManager->CreateObject<Texture>();
        mSpotShadowView = mApplicationInfo->AppObjectManager->GetObjectAsOwner<Texture>(handle);
        mSpotShadowView->CreateDepth(mSpotShadowResolution, mSpotShadowResolution);
    }

    // Straight D32F -> D32F image copy; the ImGui backend can only bind a plain GL_TEXTURE_2D.
    glCopyImageSubData(mSpotShadowArray->GetID(), GL_TEXTURE_2D_ARRAY, 0, 0, 0, slot,
                       mSpotShadowView->GetID(), GL_TEXTURE_2D, 0, 0, 0, 0,
                       mSpotShadowResolution, mSpotShadowResolution, 1);
}

void Plu::Renderer::BuildCascadeAtlas(const DirectionalLightShadowSettings& Settings)
{
    const UInt32 cascadeCount = static_cast<UInt32>(Settings.CascadeCount);

    mCascadeResolutions.Clear();
    mCascadeResolutions.Resize(cascadeCount);
    mCascadeAtlasRects.Clear();
    mCascadeAtlasRects.Resize(cascadeCount);

    Int32 atlasWidth  = 0;
    Int32 atlasHeight = 0;
    Int32 baseResolution = Settings.Resolution;

    // Halve the base resolution until the atlas fits the driver's texture limit. Many cascades at
    // a high resolution stack into a very tall atlas (8192 base x 6 cascades wants 22528 rows,
    // past the usual 16384 cap), and silently dropping shadows for a setting the details panel
    // happily offers is worse than quietly rendering them one step softer.
    while (true)
    {
        ComputeCascadeResolutions(baseResolution, Settings.CascadeCount, Settings.ResolutionFalloff,
                                  mCascadeResolutions.Data());
        BuildShadowAtlasLayout(mCascadeResolutions.Data(), Settings.CascadeCount,
                               mCascadeAtlasRects.Data(), atlasWidth, atlasHeight);

        const bool fits = mMaxTextureSize <= 0
                       || (atlasWidth <= mMaxTextureSize && atlasHeight <= mMaxTextureSize);
        if (fits || baseResolution <= kMinCascadeResolution) break;

        baseResolution /= 2;
        PLU_CORE_WARN("Renderer::BuildCascadeAtlas - Shadow atlas {}x{} exceeds GL_MAX_TEXTURE_SIZE ({}); "
                      "dropping the base shadow resolution to {}",
                      atlasWidth, atlasHeight, mMaxTextureSize, baseResolution);
    }

    RecreateShadowResources(atlasWidth, atlasHeight);
}

void Plu::Renderer::RecreateShadowResources(Int32 AtlasWidth, Int32 AtlasHeight)
{
    if (AtlasWidth == mShadowAtlasWidth && AtlasHeight == mShadowAtlasHeight && mShadowAtlas) {
        return;
    }

    DestroyShadowResources();

    // Anything still in the error queue would otherwise be blamed on the calls below.
    CheckShadowGLError("Renderer::RecreateShadowResources (entry)");

    // Last line of defence: BuildCascadeAtlas already shrinks the base resolution until the atlas
    // fits, so reaching this means even kMinCascadeResolution was too much for the driver.
    if (mMaxTextureSize > 0 && (AtlasWidth > mMaxTextureSize || AtlasHeight > mMaxTextureSize)) {
        PLU_CORE_ERROR("Renderer::RecreateShadowResources - Shadow atlas {}x{} exceeds GL_MAX_TEXTURE_SIZE ({}). "
                       "Lower ShadowCascadeCount — directional shadows disabled.",
                       AtlasWidth, AtlasHeight, mMaxTextureSize);
        return;
    }

    // The framebuffer owns its depth texture (FrameBufferType::DepthOnly), so the atlas is simply
    // that texture — no separate allocation, and no way for the two to disagree on size.
    // D32F, not D16: the depth range is no longer padded by a 50 m near margin (GL_DEPTH_CLAMP
    // replaced it), so the extra bits go straight into fighting acne instead of covering slack.
    EngineObjectHandle fbHandle = mApplicationInfo->AppObjectManager->CreateObject<FrameBuffer>();
    mShadowAtlasFrameBuffer = mApplicationInfo->AppObjectManager->GetObjectAsOwner<FrameBuffer>(fbHandle);
    // A framebuffer that failed to create must NOT be kept: FrameBuffer::Clear() and Bind()
    // silently no-op / bind framebuffer 0 on an invalid object, so the atlas would never be
    // cleared nor rendered — and an uncleared depth map reads as "everything is occluded", i.e. a
    // fully black scene. Bail out of shadows entirely instead.
    if (!mShadowAtlasFrameBuffer->CreateDepthOnly(AtlasWidth, AtlasHeight, mApplicationInfo->AppObjectManager)
        || !CheckShadowGLError("Renderer::RecreateShadowResources (atlas framebuffer)")) {
        PLU_CORE_ERROR("Renderer::RecreateShadowResources - Failed to create the shadow atlas framebuffer ({}x{}) — directional shadows disabled",
                       AtlasWidth, AtlasHeight);
        DestroyShadowResources();
        return;
    }

    mShadowAtlas = mShadowAtlasFrameBuffer->GetDepthTexture();
    if (!mShadowAtlas || !mShadowAtlas->IsValid()) {
        PLU_CORE_ERROR("Renderer::RecreateShadowResources - The shadow atlas framebuffer has no depth texture — directional shadows disabled");
        DestroyShadowResources();
        return;
    }

    mShadowAtlasWidth  = AtlasWidth;
    mShadowAtlasHeight = AtlasHeight;

    PLU_CORE_INFO("Shadow resources ready: {}x{} D32F atlas ({:.1f} MB)",
                  AtlasWidth, AtlasHeight,
                  static_cast<double>(AtlasWidth) * AtlasHeight * 4.0 / (1024.0 * 1024.0));
}

void Plu::Renderer::UnbindShadowTexture()
{
    glActiveTexture(GL_TEXTURE0 + kShadowTextureUnit);
    glBindTexture(GL_TEXTURE_2D, 0);
    SamplerObject::Unbind(kShadowTextureUnit);
}

void Plu::Renderer::RequestShadowCascadeView(Int32 layer)
{
    mShadowLayerViewRequest.store(layer, std::memory_order_relaxed);
}

Plu::TUsePointer<Plu::Texture> Plu::Renderer::GetShadowCascadeView()
{
    return mShadowLayerView;
}

void Plu::Renderer::UpdateShadowLayerView()
{
    const Int32 layer = mShadowLayerViewRequest.load(std::memory_order_relaxed);
    if (layer < 0 || !mShadowAtlas || layer >= static_cast<Int32>(mCascadeAtlasRects.Size())) return;

    PLU_PROFILE_SCOPE("Renderer::UpdateShadowLayerView");

    const ShadowAtlasRect& rect = mCascadeAtlasRects[static_cast<UInt32>(layer)];
    if (rect.Size <= 0) return;

    // Destination is rebuilt whenever the size stops matching, so the viewer keeps working across
    // a resolution change — and across switching to a cascade of a DIFFERENT resolution, which is
    // the normal case now that the atlas is mixed.
    if (!mShadowLayerView || mShadowLayerView->GetWidth() != rect.Size) {
        if (mShadowLayerView) {
            mApplicationInfo->AppObjectManager->DestroyObject(mShadowLayerView->GetObjectHandle());
            mShadowLayerView->Destroy();
            mShadowLayerView = nullptr;
        }
        EngineObjectHandle handle = mApplicationInfo->AppObjectManager->CreateObject<Texture>();
        mShadowLayerView = mApplicationInfo->AppObjectManager->GetObjectAsOwner<Texture>(handle);
        mShadowLayerView->CreateDepth(rect.Size, rect.Size);
    }

    // Straight image copy of this cascade's rect — no framebuffer, no shader, no format
    // conversion. Both textures are D32F, so the driver can move the whole square in one go.
    glCopyImageSubData(mShadowAtlas->GetID(), GL_TEXTURE_2D, 0, rect.X, rect.Y, 0,
                       mShadowLayerView->GetID(), GL_TEXTURE_2D, 0, 0, 0, 0,
                       rect.Size, rect.Size, 1);
}

void Plu::Renderer::DestroyShadowResources()
{
    // Drop the observer BEFORE the framebuffer goes: the atlas texture is owned by it, so this
    // pointer dangles the moment Destroy() runs.
    mShadowAtlas = nullptr;

    if (mShadowAtlasFrameBuffer) {
        mApplicationInfo->AppObjectManager->DestroyObject(mShadowAtlasFrameBuffer->GetObjectHandle());
        mShadowAtlasFrameBuffer->Destroy();
        mShadowAtlasFrameBuffer = nullptr;
    }

    mShadowAtlasWidth  = -1;
    mShadowAtlasHeight = -1;
}

void Plu::Renderer::ResolveSnapshotMeshes(Plu::RenderSnapshot* snapshot)
{
    PLU_PROFILE_SCOPE("Renderer::ResolveSnapshotMeshes");
    const UInt32 staticBatchCount = snapshot->StaticMeshBatches.Size();
    mResolvedBatchMeshes.Clear();
    mResolvedBatchMeshes.Reserve(staticBatchCount);
    for (UInt32 i = 0; i < staticBatchCount; i++) {
        mResolvedBatchMeshes.PushBack(mApplicationInfo->AppAssetManager->GetAssetDataNoLoad(snapshot->StaticMeshBatches[i].MeshUUID));
    }

    const UInt32 skeletalMeshCount = snapshot->SkeletalMeshRenderObjects.Size();
    mResolvedSkeletalMeshes.Clear();
    mResolvedSkeletalMeshes.Reserve(skeletalMeshCount);
    for (UInt32 i = 0; i < skeletalMeshCount; i++) {
        mResolvedSkeletalMeshes.PushBack(mApplicationInfo->AppAssetManager->GetAssetDataNoLoad(snapshot->SkeletalMeshRenderObjects[i].MeshUUID));
    }
}

Plu::DirectionalLightShadowSettings Plu::Renderer::ClampShadowSettings(const DirectionalLightShadowSettings& Settings)
{
    // The settings come straight from a details panel, so anything can be in them. Clamping
    // here (render thread, one place) keeps every consumer — cascade math, GL allocation,
    // the UBO — free of defensive checks.
    DirectionalLightShadowSettings clamped = Settings;

    clamped.CascadeCount   = glm::clamp(clamped.CascadeCount, 1, kMaxShadowCascades);
    clamped.ShadowDistance = glm::clamp(clamped.ShadowDistance, 1.0f, kCameraFarClip);
    // 0 means "uniform"; beyond the cascade count every step lands on the same cascade, so
    // anything larger is the same thing said louder.
    clamped.ResolutionFalloff = glm::clamp(clamped.ResolutionFalloff, 0, kMaxShadowCascades);
    clamped.SplitLambda    = glm::clamp(clamped.SplitLambda, 0.0f, 1.0f);
    clamped.NormalBias     = glm::clamp(clamped.NormalBias, 0.0f, 16.0f);
    clamped.DepthBias      = glm::clamp(clamped.DepthBias, 0.0f, 1.0f);
    clamped.PcfRadius      = glm::clamp(clamped.PcfRadius, 0.0f, 8.0f);
    // Auto mode resolves to a concrete tap count HERE rather than in the shader, so everything
    // downstream (UBO, stats, anyone reading mShadowSettings) sees the number actually sampled.
    clamped.PcfTapCount    = clamped.PcfAutoTaps
                           ? ComputeAutoPcfTapCount(clamped.PcfRadius)
                           : glm::clamp(clamped.PcfTapCount, 1, kMaxShadowPcfTaps);
    clamped.CascadeBlend   = glm::clamp(clamped.CascadeBlend, 0.0f, 0.5f);

    // Contact shadows. The length cap is deliberate: a screen-space march is only trustworthy
    // over a short distance — past a metre the samples spread far enough apart to step over
    // ordinary geometry, and what the ray misses reads as a hole in the shadow, not as softness.
    clamped.ContactShadowSteps     = glm::clamp(clamped.ContactShadowSteps, 4, kMaxContactShadowSteps);
    clamped.ContactShadowLength    = glm::clamp(clamped.ContactShadowLength, 0.0f, 1.0f);
    clamped.ContactShadowThickness = glm::clamp(clamped.ContactShadowThickness, 0.001f, 1.0f);
    clamped.ContactShadowBias      = glm::clamp(clamped.ContactShadowBias, 0.0f, 0.5f);

    // Resolution snaps to a power-of-two step rather than clamping to a range: the array is
    // reallocated whenever it changes, and dragging a slider through arbitrary values would
    // reallocate ~67 MB of VRAM per frame.
    constexpr Int32 kAllowedResolutions[] = {512, 1024, 2048, 4096, 8192};
    Int32 best = kAllowedResolutions[0];
    Int32 bestDistance = std::abs(clamped.Resolution - best);
    for (Int32 candidate : kAllowedResolutions) {
        const Int32 distance = std::abs(clamped.Resolution - candidate);
        if (distance < bestDistance) {
            best = candidate;
            bestDistance = distance;
        }
    }
    clamped.Resolution = best;

    return clamped;
}

bool Plu::Renderer::ResolveDepthShaders()
{
    // Shader głębi instancingu (tylko pozycja, SSBO InstanceMatrices) dla static meshy — leniwa
    // kompilacja na wątku renderu. Depth pass jest silnikowy (nie opt-in per materiał jak główny
    // pass), a SSBO instancji jest już wypełniony i zbindowany (Renderer::mInstanceBuffer) dla
    // wszystkich batchy niezależnie od tego, czy materiał widocznego passu wspiera instancing.
    mDepthShader = mApplicationInfo->AppShaderManager->GetShaderProgram(EngineAssets::OnlyPositionInstancedShader);
    mSkeletalDepthReady = false;
    if (!mDepthShader) return false;
    if (!mDepthShader->IsLoaded()) {
        mApplicationInfo->AppShaderManager->LoadShader(EngineAssets::OnlyPositionInstancedShader);
        return false; // gotowe w kolejnej klatce
    }

    // Skinowany wariant — ładowany niezależnie: jeśli jeszcze nie gotowy, pomijamy tylko cienie
    // skeletalne (static-owe i tak lecą), a nie cały pass.
    mSkeletalDepthShader = mApplicationInfo->AppShaderManager->GetShaderProgram(EngineAssets::OnlyPositionSkeletalShader);
    if (mSkeletalDepthShader && !mSkeletalDepthShader->IsLoaded()) {
        mApplicationInfo->AppShaderManager->LoadShader(EngineAssets::OnlyPositionSkeletalShader);
    }
    mSkeletalDepthReady = mSkeletalDepthShader && mSkeletalDepthShader->IsLoaded();
    return true;
}

void Plu::Renderer::PrepareShadowCascades(Plu::RenderSnapshot *snapshot, const Matrix4& cameraView)
{
    PLU_PROFILE_SCOPE("Renderer::PrepareShadowCascades");
    mCascades.Clear();

    const DirectionalLightShadowSettings settings = ClampShadowSettings(snapshot->DirLight.Shadow);
    mShadowSettings = settings;

    if (!snapshot->HasDirLight || !settings.CastShadows) {
        // No directional shadows this frame. Clear the atlas ONCE on the transition so leftover
        // content (e.g. shadows baked during a previous PIE session) doesn't linger, then stop
        // touching it — with CascadeCount = 0 nothing samples it anyway.
        if (!mShadowMapsCleared) {
            if (mShadowAtlasFrameBuffer) {
                mShadowAtlasFrameBuffer->Clear(0.0f, 0.0f, 0.0f, 1.0f);
            }
            mShadowMapsCleared = true;
        }
        return;
    }

    // Resolution / cascade count / falloff are settings, so the GL resources may need rebuilding.
    // Safe here: the render thread owns the GL context, and nothing samples the atlas until the
    // main pass below.
    BuildCascadeAtlas(settings);
    if (!mShadowAtlas) {
        return;
    }
    mShadowMapsCleared = false;

    // No static depth shader, no depth pass — leaving mCascades empty makes
    // UpdateShadowDataBuffer publish CascadeCount = 0, so nothing samples a map we never wrote.
    if (!mDepthShader || !mDepthShader->IsLoaded()) return;

    TUsePointer<IWindow> window = mApplicationInfo->AppWindow;
    const float aspect = static_cast<float>(window->GetWidth()) / static_cast<float>(window->GetHeight());
    const float fovRad = glm::radians(snapshot->CameraFOV);

    // A higher lambda packs the near cascades tighter against the camera — sharper close-up
    // shadows. At the defaults (4 cascades, 150 m, lambda 0.9) the splits land around
    // 4.3 / 11 / 33 / 150 m; at lambda 0.95 they move in to 2.5 / 7.4 / 28.5 / 150 m.
    CascadeConfig cascadeConfig;
    cascadeConfig.CascadeCount   = settings.CascadeCount;
    cascadeConfig.ShadowDistance = settings.ShadowDistance;
    cascadeConfig.SplitLambda    = settings.SplitLambda;
    cascadeConfig.Resolution     = settings.Resolution;
    cascadeConfig.ResolutionFalloff = settings.ResolutionFalloff;

    ComputeCascadeSplits(cascadeConfig, kCameraNearClip, mCascadeSplits);
    // The resolutions passed here are the SAME ones the atlas was just laid out with, which is
    // what keeps the texel snap on the grid the cascade actually renders at.
    ComputeCascadeMatrices(
        cameraView, fovRad, aspect,
        kCameraNearClip,
        snapshot->DirLight.Direction,
        cascadeConfig,
        mCascadeSplits,
        mCascades,
        mCascadeResolutions.Data()
    );
}

void Plu::Renderer::EnsureDepthPrepassBuffer()
{
    if (!mMainBuffer) return;

    const Int32 width  = mMainBuffer->GetWidth();
    const Int32 height = mMainBuffer->GetHeight();
    if (width <= 0 || height <= 0) return;

    if (mDepthPrepassBuffer && mDepthPrepassBuffer->GetWidth() == width
        && mDepthPrepassBuffer->GetHeight() == height) {
        return;
    }

    // Rebuild rather than Resize: this only happens when the window changes size, and a fresh
    // framebuffer cannot end up half-migrated the way an in-place resize of a texture attachment
    // can. The observer goes first — the texture belongs to the framebuffer being destroyed.
    mSceneDepthTexture = nullptr;
    if (mDepthPrepassBuffer) {
        mApplicationInfo->AppObjectManager->DestroyObject(mDepthPrepassBuffer->GetObjectHandle());
        mDepthPrepassBuffer->Destroy();
        mDepthPrepassBuffer = nullptr;
    }

    EngineObjectHandle handle = mApplicationInfo->AppObjectManager->CreateObject<FrameBuffer>();
    mDepthPrepassBuffer = mApplicationInfo->AppObjectManager->GetObjectAsOwner<FrameBuffer>(handle);
    // DepthOnly gives a D32F texture. Nothing has to match the main buffer's D24S8 renderbuffer
    // any more (see RenderDepthPrepass on why the depth is not blitted), and the extra precision
    // goes straight into the contact-shadow ray, which linearises this value per sample.
    if (!mDepthPrepassBuffer->Create(width, height, mApplicationInfo->AppObjectManager, FrameBufferType::DepthOnly)
        || !CheckShadowGLError("Renderer::EnsureDepthPrepassBuffer")) {
        PLU_CORE_ERROR("Renderer::EnsureDepthPrepassBuffer - Failed to create the depth prepass framebuffer ({}x{}) — contact shadows and early-Z disabled",
                       width, height);
        mApplicationInfo->AppObjectManager->DestroyObject(mDepthPrepassBuffer->GetObjectHandle());
        mDepthPrepassBuffer->Destroy();
        mDepthPrepassBuffer = nullptr;
        return;
    }

    mSceneDepthTexture = mDepthPrepassBuffer->GetDepthTexture();
    PLU_CORE_INFO("Depth prepass buffer ready: {}x{} D32F", width, height);
}

bool Plu::Renderer::AreContactShadowsActive(const Plu::RenderSnapshot* snapshot) const
{
    return snapshot->HasDirLight
        && mShadowSettings.CastShadows
        && mShadowSettings.ContactShadows
        && mShadowSettings.ContactShadowLength > 0.0f;
}

void Plu::Renderer::UnbindSceneDepthTexture()
{
    glActiveTexture(GL_TEXTURE0 + kSceneDepthTextureUnit);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Plu::Renderer::RenderDepthPrepass(Plu::RenderSnapshot* snapshot, const Matrix4& viewProj)
{
    if (!mDepthPrepassBuffer || !mDepthShader || !mDepthShader->IsLoaded()) return;
    // Nothing samples the scene depth this frame, so the whole geometry pass would be waste.
    // Contact shadows are its only consumer today — add to AreContactShadowsActive when that
    // stops being true (SSAO, SSR, ...), or the new consumer will read a stale buffer.
    if (!AreContactShadowsActive(snapshot)) return;

    PLU_PROFILE_SCOPE("Renderer::RenderDepthPrepass");
    PLU_PROFILE_SCOPE_GPU("Renderer::DepthPrepass");

    // Same rule as the shadow atlas: the texture about to become the render target must not still
    // be bound for sampling from the previous frame's lighting pass.
    UnbindSceneDepthTexture();

    mDepthPrepassBuffer->Clear(0.0f, 0.0f, 0.0f, 1.0f);
    mDepthPrepassBuffer->Bind();

    const UInt32 staticBatchCount = snapshot->StaticMeshBatches.Size();
    const UInt32 cameraFrustum    = CameraFrustumIndex();

    // Static meshes — the same instanced depth shader and the same visible-index indirection the
    // cascades use, reading the camera's slice of the ranges CullShadowCasters produced.
    mDepthShader->SetMatrix4Uniform("lightSpaceMatrix", viewProj);
    for (UInt32 i = 0; i < staticBatchCount; i++) {
        const ShadowDrawRange& range = mShadowDrawRanges[cameraFrustum * staticBatchCount + i];
        if (range.Count == 0) continue;
        const TUsePointer<StaticMesh>& staticMesh = mResolvedBatchMeshes[i];
        if (!staticMesh || !staticMesh->IsLoaded) continue;
        mDepthShader->SetIntUniform("instanceBaseIndex", static_cast<int>(range.Offset));
        DrawStaticMeshInstanced(staticMesh.GetRaw(), mApplicationInfo->AppRenderingManager.GetRaw(), range.Count);
        snapshot->StatDrawCalls++;
    }

    // Skeletal meshes — skinned depth, so animated geometry occludes contact-shadow rays exactly
    // where it is drawn. Culled here (per object) because they have no batch ranges.
    if (mSkeletalDepthReady) {
        const UInt64 skeletalMeshCount = snapshot->SkeletalMeshRenderObjects.Size();
        mSkeletalDepthShader->SetMatrix4Uniform("lightSpaceMatrix", viewProj);
        for (UInt32 i = 0; i < skeletalMeshCount; i++) {
            SkeletalMeshRenderObject* renderObject = &snapshot->SkeletalMeshRenderObjects[i];
            const TUsePointer<SkeletalMesh>& skeletalMesh = mResolvedSkeletalMeshes[i];
            if (!skeletalMesh || !skeletalMesh->IsLoaded) continue;
            // No frustum test and no CastsShadow test — the lighting pass draws every skeletal
            // mesh unconditionally, and the prepass has to match it exactly (see CullShadowCasters).

            mSkeletalDepthShader->SetIntUniform("paletteBaseIndex", static_cast<int>(mSkeletalPaletteRanges[i].Offset));
            mSkeletalDepthShader->SetMatrix4Uniform("model", renderObject->ModelMatrix);
            DrawSkeletalMesh(skeletalMesh.GetRaw(), mApplicationInfo->AppRenderingManager.GetRaw());
            snapshot->StatDrawCalls++;
        }
    }

    mDepthPrepassBuffer->Unbind();

    // NO blit into the main buffer, and therefore no early-Z. It is tempting — the depth is right
    // there — but the depth shaders do not compute gl_Position the way the material shaders do:
    //
    //   prepass: lightSpaceMatrix * model * skinMatrix * pos   (proj*view multiplied on the CPU)
    //   main:    projection * view * model * (skinMatrix * pos)
    //
    // Same value mathematically, different grouping and different rounding, so the two disagree by
    // a few ulp. Handing this depth to a GL_LEQUAL lighting pass makes the losing fragments fail
    // the test and drop out to the background — blotches over skeletal meshes, whose chain is the
    // longest. Early-Z needs both passes to compute the position with the SAME expression
    // (and `invariant gl_Position`); until then the prepass exists purely to feed contact shadows.
    CheckShadowGLError("Renderer::RenderDepthPrepass");
}

void Plu::Renderer::RenderShadowPass(Plu::RenderSnapshot *snapshot)
{
    if (mCascades.IsEmpty() || !mShadowAtlasFrameBuffer) return;

    PLU_PROFILE_SCOPE("Renderer::RenderShadowPass");
    PLU_PROFILE_SCOPE_GPU("Renderer::RenderShadowPass");

    const UInt64 skeletalMeshCount = snapshot->SkeletalMeshRenderObjects.Size();

    // Front-face culling tylko na czas map cieni: do bufora głębi trafiają TYLNE ściany
    // obiektów, więc próg self-shadowingu (acne) przesuwa się na niewidoczną, odwróconą od
    // kamery stronę geometrii — najskuteczniejszy zabieg na acne płaskich/prostopadłych
    // powierzchni. Culling jest globalnie wyłączony (główny pass renderuje obie strony),
    // więc po passie przywracamy stan. Uwaga: dla otwartej/jednostronnej geometrii (pojedyncze
    // quady) może dać light-leak — wtedy normal-offset w PBR.frag łagodzi przypadki brzegowe.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    // Slope-scaled depth bias po stronie CASTERA: liczony per-trójkąt w jednostkach precyzji
    // bufora głębi, więc nie skaluje się z rozmiarem teksela kaskady i nie przesuwa cienia
    // w bok (w przeciwieństwie do normal-offsetu w PBR.frag). Dzięki temu ten sam bias
    // leczy acne dużych powierzchni w dalekich kaskadach, nie zjadając cieni małych obiektów.
    // Wartości przestrojone pod D32F: jednostka offsetu to najmniejszy rozróżnialny krok głębi,
    // który przy 32-bitowym floacie jest znacznie mniejszy niż przy dawnym D16.
    constexpr float kShadowPolygonOffsetFactor = 2.0f;
    constexpr float kShadowPolygonOffsetUnits  = 4.0f;
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(kShadowPolygonOffsetFactor, kShadowPolygonOffsetUnits);

    // The array is about to become the render target, so it must not still be bound for
    // sampling from the previous frame's main pass — see UnbindShadowTexture.
    UnbindShadowTexture();

    // Depth clamping ("pancaking"): casters between the light and the cascade sphere are in
    // front of the ortho near plane. Instead of pushing that plane 50 m towards the light —
    // which stretches the depth range of every cascade and costs precision everywhere — we
    // clamp them onto the near plane. Their exact depth is wrong, but they are nearer than
    // anything in the cascade anyway, so the comparison result is not.
    glEnable(GL_DEPTH_CLAMP);

    const UInt32 staticBatchCount = snapshot->StaticMeshBatches.Size();

    // One bind and one clear for the whole atlas: the cascades are regions of a single depth
    // texture now, so clearing them individually would need a scissor rect per cascade to cover
    // exactly the same texels this one call does.
    mShadowAtlasFrameBuffer->Clear(0.0f, 0.0f, 0.0f, 1.0f);
    mShadowAtlasFrameBuffer->Bind();  // sets glViewport to the full atlas; overridden per cascade below

    for (UInt32 c = 0; c < mCascades.Size(); c++) {
        PLU_PROFILE_SCOPE_GPU(CascadeGpuScopeNames()[c]);

        // The cascade's square in the atlas. This is the ONLY thing separating one cascade's
        // depth from another's — there is no per-cascade attachment any more.
        const ShadowAtlasRect& rect = mCascadeAtlasRects[c];
        glViewport(rect.X, rect.Y, rect.Size, rect.Size);

        // Static meshe — instanced shader głębi, batche z RenderSnapshotBuilder::BatchStaticMeshes.
        // Kamerowy culling z batchowania (VisibleCount) jest dla cieni bezużyteczny — caster poza
        // kadrem kamery może rzucać cień w kadr — więc pass cieni cullinguje sam, per kaskada, i
        // adresuje instancje przez skompaktowaną tablicę indeksów. Jeden glDrawElementsInstanced
        // na batch, a batch niewidoczny w tej kaskadzie odpada bez draw calla.
        mDepthShader->SetMatrix4Uniform("lightSpaceMatrix", mCascades[c].ViewProj);
        for (UInt32 i = 0; i < staticBatchCount; i++) {
            const ShadowDrawRange& range = mShadowDrawRanges[c * staticBatchCount + i];
            if (range.Count == 0) continue;
            const TUsePointer<StaticMesh>& staticMesh = mResolvedBatchMeshes[i];
            if (!staticMesh || !staticMesh->IsLoaded) continue;
            // For the depth shader instanceBaseIndex indexes the VISIBLE-INDEX buffer, not the
            // instance buffer — the extra indirection is what lets a cascade draw a subset.
            mDepthShader->SetIntUniform("instanceBaseIndex", static_cast<int>(range.Offset));
            DrawStaticMeshInstanced(staticMesh.GetRaw(), mApplicationInfo->AppRenderingManager.GetRaw(), range.Count);
            snapshot->StatDrawCalls++;
        }

        // Skeletal meshe — skinowany shader głębi z tą samą paletą kości co główny pass, dzięki
        // czemu cień podąża za animacją. Palety WSZYSTKICH meshy poszły na GPU raz na klatkę
        // (UploadSkeletalPalettes), więc tutaj zostaje tylko offset w tym buforze — dawniej każdy
        // obiekt nadpisywał wspólny bufor, per kaskada, czyli 5x ten sam upload co klatkę.
        if (mSkeletalDepthReady) {
            mSkeletalDepthShader->SetMatrix4Uniform("lightSpaceMatrix", mCascades[c].ViewProj);
            const Frustum cascadeFrustum = ExtractFrustumPlanes(mCascades[c].ViewProj);
            for (UInt32 i = 0; i < skeletalMeshCount; i++) {
                SkeletalMeshRenderObject* renderObject = &snapshot->SkeletalMeshRenderObjects[i];
                if (!renderObject->CastsShadow) continue;
                const TUsePointer<SkeletalMesh>& skeletalMesh = mResolvedSkeletalMeshes[i];
                if (!skeletalMesh || !skeletalMesh->IsLoaded) continue;
                if (!SphereInFrustumNoNear(cascadeFrustum, renderObject->BoundsCenter, renderObject->BoundsRadius)) {
                    snapshot->StatCulledCount++;
                    continue;
                }

                mSkeletalDepthShader->SetIntUniform("paletteBaseIndex", static_cast<int>(mSkeletalPaletteRanges[i].Offset));
                mSkeletalDepthShader->SetMatrix4Uniform("model", renderObject->ModelMatrix);
                DrawSkeletalMesh(skeletalMesh.GetRaw(), mApplicationInfo->AppRenderingManager.GetRaw());
                snapshot->StatDrawCalls++;
                mCascadeCasterCounts[c]++;
            }
        }

        CheckShadowGLError("Renderer::RenderShadowPass (cascade draw)");
    }

    mShadowAtlasFrameBuffer->Unbind();

    // Przywróć stan cullingu, polygon offsetu i depth clampa do domyślnego dla głównego passa.
    glDisable(GL_DEPTH_CLAMP);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.0f, 0.0f);
    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
}

UInt32 Plu::Renderer::CullShadowCasters(Plu::RenderSnapshot* snapshot)
{
    PLU_PROFILE_SCOPE("Renderer::CullShadowCasters");

    const UInt32 cascadeCount  = mCascades.Size();
    const UInt32 spotSlotCount = mSpotShadowMatrices.Size();
    // +1 for the camera frustum, which the depth prepass draws from. Appending it here rather
    // than culling separately keeps the whole frame at ONE visible-index upload on ONE binding.
    const UInt32 frustumCount  = cascadeCount + spotSlotCount + 1;
    const UInt32 batchCount    = snapshot->StaticMeshBatches.Size();

    mVisibleInstanceScratch.Clear();
    mShadowDrawRanges.Clear();
    mShadowDrawRanges.Resize(frustumCount * batchCount);
    // Cleared and resized HERE, so a frame that produced no cascades / no spot slots publishes
    // zeroed stats instead of last frame's numbers.
    mCascadeCasterCounts.Clear();
    mCascadeCasterCounts.Resize(cascadeCount);
    mSpotShadowCasterCounts.Clear();
    mSpotShadowCasterCounts.Resize(spotSlotCount);

    UInt32 culledCount = 0;

    const UInt32 cameraFrustum = CameraFrustumIndex();

    for (UInt32 f = 0; f < frustumCount; f++) {
        const bool isCascade = f < cascadeCount;
        const bool isCamera  = f == cameraFrustum;
        // The camera entry needs no frustum of its own — see the isCamera branch below.
        const Frustum shadowFrustum = isCamera
            ? Frustum{}
            : ExtractFrustumPlanes(isCascade ? mCascades[f].ViewProj
                                             : mSpotShadowMatrices[f - cascadeCount]);

        for (UInt32 b = 0; b < batchCount; b++) {
            const StaticMeshBatch& batch = snapshot->StaticMeshBatches[b];
            ShadowDrawRange& range = mShadowDrawRanges[f * batchCount + b];
            range.Offset = static_cast<UInt32>(mVisibleInstanceScratch.Size());
            range.Count  = 0;

            if (batch.TotalCount == 0) continue;

            if (isCamera) {
                // The camera's range is taken VERBATIM from the batch — the instances MAIN already
                // marked visible — instead of being re-culled here. Two reasons:
                //  * the prepass must draw exactly what the lighting pass draws. Re-deriving the
                //    frustum on this thread could disagree by an ulp, and an instance present in
                //    the depth buffer but absent from the colour pass occludes without ever being
                //    shaded — a black hole in the scene;
                //  * CastsShadow must NOT gate it: an object excluded from shadow casting is still
                //    visible, and leaving it out would punch a hole in the depth buffer that
                //    contact shadows march through.
                for (UInt32 v = 0; v < batch.VisibleCount; v++) {
                    mVisibleInstanceScratch.PushBack(batch.InstanceOffset + v);
                }
                range.Count = batch.VisibleCount;
                continue;
            }

            if (!batch.CastsShadow) continue;

            const UInt32 end = batch.InstanceOffset + batch.TotalCount;
            for (UInt32 instance = batch.InstanceOffset; instance < end; instance++) {
                const InstanceCullData& bounds = snapshot->StaticInstanceBounds[instance];
                // Cascades skip the near plane because depth-clamped casters legitimately sit in
                // front of it (see SphereInFrustumNoNear) — culling them would remove the very
                // objects casting into the cascade. A spot slot has no depth clamp (pancaking a
                // perspective projection would invent shadows right at the apex), so its near
                // plane is real and must be tested. The camera's near plane is as real as it gets.
                const bool visible = isCascade
                    ? SphereInFrustumNoNear(shadowFrustum, bounds.BoundsCenter, bounds.BoundsRadius)
                    : SphereInFrustum(shadowFrustum, bounds.BoundsCenter, bounds.BoundsRadius);
                if (!visible) {
                    // The camera's culling is already counted on MAIN (RenderSnapshotBuilder), so
                    // counting it again here would report every off-screen object twice.
                    if (!isCamera) culledCount++;
                    continue;
                }
                mVisibleInstanceScratch.PushBack(instance);
                range.Count++;
            }
            if (isCamera) {
                // No per-frustum stat for the camera — the prepass is not a shadow map.
            } else if (isCascade) {
                mCascadeCasterCounts[f] += range.Count;
            } else {
                mSpotShadowCasterCounts[f - cascadeCount] += range.Count;
            }
        }
    }

    if (!mVisibleInstanceScratch.IsEmpty()) {
        const Int32 needed = static_cast<Int32>(mVisibleInstanceScratch.Size());
        // Grow with 2x headroom, like the instance buffer: a scene whose visible-caster count
        // wobbles frame to frame would otherwise reallocate every frame.
        if (mVisibleInstanceBuffer.GetCount() < needed) {
            mVisibleInstanceBuffer.Resize(needed * 2);
        }
        mVisibleInstanceBuffer.Update(mVisibleInstanceScratch.Data(), needed);
    }
    // Bind after any Resize — Resize creates a new buffer ID and the indexed binding point would
    // otherwise still hold the deleted one.
    mVisibleInstanceBuffer.BindBase(3);

    return culledCount;
}

void Plu::Renderer::PrepareSpotShadowSlots(Plu::RenderSnapshot* snapshot)
{
    PLU_PROFILE_SCOPE("Renderer::PrepareSpotShadowSlots");

    mSpotShadowSlotOwners.Clear();
    mSpotShadowMatrices.Clear();

    if (snapshot->SpotLights.IsEmpty()) return;
    if (!mDepthShader || !mDepthShader->IsLoaded()) return;

    RecreateSpotShadowResources();
    if (mSpotShadowFrameBuffers.IsEmpty()) return;

    // The snapshot arrives sorted descending by importance (RenderSnapshotBuilder), so handing
    // out slots is just walking the front of the list — no sorting on the render thread.
    //
    // Every slot is redrawn from scratch each frame, so a light swapping slots between frames is
    // invisible. What IS visible is a light crossing the budget boundary: its shadow appears or
    // disappears. That is inherent to a fixed pool, and SpotLight::ShadowPriority exists so a
    // scene can pin the shadows it actually cares about above the competition.
    const UInt32 lightCount = snapshot->SpotLights.Size();
    const UInt32 slotBudget = static_cast<UInt32>(kMaxSpotShadowSlots);
    for (UInt32 i = 0; i < lightCount && mSpotShadowSlotOwners.Size() < slotBudget; i++) {
        const SpotLightRenderObject& light = snapshot->SpotLights[i];
        if (!light.CastShadows) continue;

        mSpotShadowSlotOwners.PushBack(static_cast<Int32>(i));
        mSpotShadowMatrices.PushBack(ComputeSpotLightMatrix(light.Location, light.Direction, light.Range, light.OuterConeAngle));
    }
}

void Plu::Renderer::RenderSpotShadowPass(Plu::RenderSnapshot* snapshot)
{
    if (mSpotShadowSlotOwners.IsEmpty()) return;

    PLU_PROFILE_SCOPE("Renderer::RenderSpotShadowPass");
    PLU_PROFILE_SCOPE_GPU("Renderer::RenderSpotShadowPass");

    // The atlas is about to become the render target, so it must not still be bound for sampling
    // from the previous frame's main pass — same feedback loop as the cascade array.
    UnbindSpotShadowTexture();

    // Same caster-side bias budget as the cascades: front-face culling moves the acne threshold
    // onto the geometry's hidden side, and the slope-scaled polygon offset works per triangle in
    // depth-buffer units, so it does not shift the shadow sideways.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    constexpr float kShadowPolygonOffsetFactor = 2.0f;
    constexpr float kShadowPolygonOffsetUnits  = 4.0f;
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(kShadowPolygonOffsetFactor, kShadowPolygonOffsetUnits);

    // Deliberately NO GL_DEPTH_CLAMP here, unlike the cascade pass. Pancaking works for an ortho
    // projection; under a perspective one it would flatten every caster in front of the near
    // plane onto it, inventing a shadow right at the cone apex. kSpotShadowNearClip (5 cm) sits
    // close enough to the apex that nothing real is lost by testing the near plane honestly.

    const UInt32 staticBatchCount = snapshot->StaticMeshBatches.Size();
    const UInt32 cascadeCount     = mCascades.Size();
    const UInt32 skeletalMeshCount = snapshot->SkeletalMeshRenderObjects.Size();

    for (UInt32 s = 0; s < mSpotShadowSlotOwners.Size(); s++) {
        PLU_PROFILE_SCOPE_GPU(SpotShadowGpuScopeNames()[s]);

        mSpotShadowFrameBuffers[s]->Clear(0.0f, 0.0f, 0.0f, 1.0f);
        mSpotShadowFrameBuffers[s]->Bind(); // sets the viewport to the slot resolution

        const Matrix4& lightMatrix = mSpotShadowMatrices[s];

        // Static meshes — the same instanced depth shader and the same visible-index SSBO the
        // cascades use. The spot frusta were culled in the same sweep (CullShadowCasters), so
        // their ranges sit right after the cascades' in mShadowDrawRanges.
        mDepthShader->SetMatrix4Uniform("lightSpaceMatrix", lightMatrix);
        for (UInt32 i = 0; i < staticBatchCount; i++) {
            const ShadowDrawRange& range = mShadowDrawRanges[(cascadeCount + s) * staticBatchCount + i];
            if (range.Count == 0) continue;
            const TUsePointer<StaticMesh>& staticMesh = mResolvedBatchMeshes[i];
            if (!staticMesh || !staticMesh->IsLoaded) continue;
            mDepthShader->SetIntUniform("instanceBaseIndex", static_cast<int>(range.Offset));
            DrawStaticMeshInstanced(staticMesh.GetRaw(), mApplicationInfo->AppRenderingManager.GetRaw(), range.Count);
            snapshot->StatDrawCalls++;
        }

        if (mSkeletalDepthReady) {
            mSkeletalDepthShader->SetMatrix4Uniform("lightSpaceMatrix", lightMatrix);
            // Full frustum test including the near plane, for the no-depth-clamp reason above.
            const Frustum spotFrustum = ExtractFrustumPlanes(lightMatrix);
            for (UInt32 i = 0; i < skeletalMeshCount; i++) {
                SkeletalMeshRenderObject* renderObject = &snapshot->SkeletalMeshRenderObjects[i];
                if (!renderObject->CastsShadow) continue;
                const TUsePointer<SkeletalMesh>& skeletalMesh = mResolvedSkeletalMeshes[i];
                if (!skeletalMesh || !skeletalMesh->IsLoaded) continue;
                if (!SphereInFrustum(spotFrustum, renderObject->BoundsCenter, renderObject->BoundsRadius)) {
                    snapshot->StatCulledCount++;
                    continue;
                }

                mSkeletalDepthShader->SetIntUniform("paletteBaseIndex", static_cast<int>(mSkeletalPaletteRanges[i].Offset));
                mSkeletalDepthShader->SetMatrix4Uniform("model", renderObject->ModelMatrix);
                DrawSkeletalMesh(skeletalMesh.GetRaw(), mApplicationInfo->AppRenderingManager.GetRaw());
                snapshot->StatDrawCalls++;
                mSpotShadowCasterCounts[s]++;
            }
        }

        mSpotShadowFrameBuffers[s]->Unbind();
        CheckShadowGLError("Renderer::RenderSpotShadowPass (slot draw)");
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.0f, 0.0f);
    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
}

void Plu::Renderer::UpdateSpotLightBuffers(Plu::RenderSnapshot* snapshot)
{
    PLU_PROFILE_SCOPE("Renderer::UpdateSpotLightBuffers");

    mSpotLightScratch.Clear();
    mSpotLightIndices.Clear();
    mSpotLightData = SpotLightDataGPU();

    // Defensive clamp: MAIN already trims to kMaxVisibleSpotLights, and the GPU buffers are
    // allocated at exactly that size, so anything beyond it would overrun the upload.
    const UInt32 snapshotLightCount = static_cast<UInt32>(snapshot->SpotLights.Size());
    const UInt32 lightCount = snapshotLightCount < static_cast<UInt32>(kMaxVisibleSpotLights)
                            ? snapshotLightCount
                            : static_cast<UInt32>(kMaxVisibleSpotLights);
    mSpotLightScratch.Reserve(lightCount);
    mSpotLightIndices.Reserve(lightCount);

    for (UInt32 i = 0; i < lightCount; i++) {
        const SpotLightRenderObject& light = snapshot->SpotLights[i];

        SpotLightGPU gpu{};
        // Identity, not garbage: a light without a slot never has its matrix read (the shader
        // checks shadowSlot first), but leaving it uninitialised makes any future bug read as a
        // wild transform instead of an obvious no-op.
        gpu.ShadowViewProj = Matrix4(1.0f);
        gpu.Position       = light.Location;
        gpu.Range          = light.Range;
        gpu.Direction      = light.Direction;
        gpu.InnerConeCos   = light.InnerConeCos;
        // Colour premultiplied by intensity on the CPU — the shader has no use for them apart.
        gpu.Color          = light.Color * light.Intensity;
        gpu.OuterConeCos   = light.OuterConeCos;
        gpu.ShadowSlot     = -1;
        gpu.ShadowDepthBias  = light.ShadowDepthBias;
        gpu.ShadowNormalBias = light.ShadowNormalBias;
        gpu.ShadowPcfRadius  = light.ShadowPcfRadius;
        // World size of one texel per metre of distance: the projection spans
        // 2*tan(outerHalfAngle) world units at 1 m, divided across the slot's resolution.
        gpu.ShadowTexelWorldPerMetre = mSpotShadowResolution > 0
                                     ? (2.0f * std::tan(light.OuterConeAngle)) / static_cast<float>(mSpotShadowResolution)
                                     : 0.0f;
        gpu.ShadowPcfTaps  = glm::clamp(light.ShadowPcfTaps, 1, kMaxShadowPcfTaps);
        // f*n/(f-n) of THIS light's shadow projection — the constant part of dz01/dd, which is
        // what turns a bias authored in metres into this frame's projected depth. Must use the
        // same near/far as ComputeSpotLightMatrix, hence the identical clamp on the far plane.
        const float farPlane = std::max(light.Range, kSpotShadowNearClip + 0.01f);
        gpu.ShadowDepthBiasScale = (farPlane * kSpotShadowNearClip) / (farPlane - kSpotShadowNearClip);

        mSpotLightScratch.PushBack(gpu);
    }

    // Slots second, so a light that lost the competition keeps ShadowSlot = -1 from above.
    for (UInt32 s = 0; s < mSpotShadowSlotOwners.Size(); s++) {
        const UInt32 lightIndex = static_cast<UInt32>(mSpotShadowSlotOwners[s]);
        if (lightIndex >= mSpotLightScratch.Size()) continue;
        mSpotLightScratch[lightIndex].ShadowSlot     = static_cast<Int32>(s);
        mSpotLightScratch[lightIndex].ShadowViewProj = mSpotShadowMatrices[s];
    }

    // One global index list — the whole clustered-forward investment. Today it is [0, count) and
    // the offset is 0; with clusters it becomes per-cluster sub-lists and PBR.frag does not change.
    for (UInt32 i = 0; i < lightCount; i++) {
        mSpotLightIndices.PushBack(i);
    }

    mSpotLightData.SpotLightOffset = 0;
    mSpotLightData.SpotLightCount  = static_cast<Int32>(lightCount);
    mSpotLightData.InvSpotShadowResolution = mSpotShadowResolution > 0
                                           ? 1.0f / static_cast<float>(mSpotShadowResolution)
                                           : 0.0f;
    mSpotLightData.SpotShadowSlotCount = static_cast<Int32>(mSpotShadowSlotOwners.Size());

    // Uploaded unconditionally, every frame — with SpotLightCount = 0 when there is nothing to
    // light. Same rule as ShadowData: there is no frame in which a shader reads the previous
    // frame's light list.
    if (lightCount > 0) {
        mSpotLightBuffer.Update(mSpotLightScratch.Data(), static_cast<Int32>(lightCount));
        mSpotLightIndexBuffer.Update(mSpotLightIndices.Data(), static_cast<Int32>(lightCount));
    }
    mSpotLightDataBuffer.Update(mSpotLightData);
}

void Plu::Renderer::UpdateShadowDataBuffer(Plu::RenderSnapshot* snapshot)
{
    PLU_PROFILE_SCOPE("Renderer::UpdateShadowDataBuffer");

    mShadowData = ShadowDataGPU();
    mShadowData.CascadeCount = static_cast<Int32>(mCascades.Size());

    const float atlasWidth  = static_cast<float>(std::max(mShadowAtlasWidth, 1));
    const float atlasHeight = static_cast<float>(std::max(mShadowAtlasHeight, 1));

    for (UInt32 c = 0; c < mCascades.Size() && c < static_cast<UInt32>(kMaxShadowCascades); c++) {
        const ShadowCascadeData& cascade = mCascades[c];
        ShadowCascadeGPU& gpu = mShadowData.Cascades[c];

        gpu.ViewProj = cascade.ViewProj;

        // Where this cascade lives in the atlas, as the scale/bias the shader applies to its
        // [0,1] projected coordinates. Computed here rather than in GLSL because it is per
        // cascade, not per fragment — and because it is the only thing that has to agree with
        // the glViewport the depth pass used.
        const ShadowAtlasRect& rect = mCascadeAtlasRects[c];
        gpu.AtlasScaleBias = Vec4(
            static_cast<float>(rect.Size) / atlasWidth,
            static_cast<float>(rect.Size) / atlasHeight,
            static_cast<float>(rect.X)    / atlasWidth,
            static_cast<float>(rect.Y)    / atlasHeight);

        gpu.Params.x = cascade.SplitDistance;
        gpu.Params.y = cascade.TexelWorldSize;
        // The depth bias is authored in METRES; converting it into each cascade's own [0,1]
        // depth range here is what makes "5 mm of bias" mean 5 mm in every cascade. Doing it on
        // the CPU also retires the per-fragment GLSL helper that used to recover the same scale
        // from the light matrix.
        gpu.Params.z = cascade.DepthRange > 0.0f
                     ? mShadowSettings.DepthBias / cascade.DepthRange
                     : 0.0f;
        gpu.Params.w = 0.0f;
    }

    // Fade out over the last quarter of the shadow distance instead of cutting off at the end
    // of the last cascade.
    mShadowData.ShadowFadeEnd        = mShadowSettings.ShadowDistance;
    mShadowData.ShadowFadeStart      = mShadowSettings.ShadowDistance * 0.85f;
    mShadowData.CascadeBlendFraction = mShadowSettings.CascadeBlend;
    mShadowData.NormalBiasScale      = mShadowSettings.NormalBias;
    mShadowData.PcfRadiusTexels      = mShadowSettings.PcfRadius;
    mShadowData.PcfTapCount          = mShadowSettings.PcfTapCount;
    mShadowData.PcfRotateSamples     = mShadowSettings.PcfRotate ? 1 : 0;
    mShadowData.DebugVisualizeCascades = snapshot->ShowShadowCascades ? 1 : 0;
    // One texel of the atlas is one texel of whichever cascade owns it, so a single inverse size
    // converts the PCF radius (authored in texels) to UV for every cascade — no per-cascade
    // resolution needed in the shader even though they now differ.
    mShadowData.InvAtlasSize = Vec2(1.0f / atlasWidth, 1.0f / atlasHeight);

    // Contact shadows. Steps = 0 is the single "off" switch the shader tests, so everything that
    // can disable them — the setting, a missing prepass texture, a degenerate length — collapses
    // into it here rather than being re-checked per fragment.
    const bool contactShadowsUsable = AreContactShadowsActive(snapshot) && mSceneDepthTexture;
    mShadowData.ContactShadowSteps     = contactShadowsUsable ? mShadowSettings.ContactShadowSteps : 0;
    mShadowData.ContactShadowLength    = mShadowSettings.ContactShadowLength;
    mShadowData.ContactShadowThickness = mShadowSettings.ContactShadowThickness;
    mShadowData.ContactShadowBias      = mShadowSettings.ContactShadowBias;

    mShadowDataBuffer.Update(mShadowData);
}

void Plu::Renderer::BuildSkeletalPalettes(Plu::RenderSnapshot* snapshot)
{
    mSkeletalPaletteScratch.Clear();
    mSkeletalPaletteRanges.Clear();
    const UInt32 skeletalMeshCount = static_cast<UInt32>(snapshot->SkeletalMeshRenderObjects.Size());
    mSkeletalPaletteRanges.Reserve(skeletalMeshCount);
    for (UInt32 i = 0; i < skeletalMeshCount; i++) {
        const SkeletalMeshRenderObject& renderObject = snapshot->SkeletalMeshRenderObjects[i];
        SkeletalPaletteRange range;
        range.Offset = static_cast<UInt32>(mSkeletalPaletteScratch.Size());
        for (const auto& bone : renderObject.Bones) {
            // {offset, global}: skin = global * offset. The reverse also yields identity in
            // bind pose (offset == global⁻¹), so a swap here only breaks animated poses.
            mSkeletalPaletteScratch.PushBack(bone.second * bone.first);
        }
        range.Count = static_cast<UInt32>(mSkeletalPaletteScratch.Size()) - range.Offset;
        mSkeletalPaletteRanges.PushBack(range);
    }
}

void Plu::Renderer::UploadSkeletalPalettes()
{
    PLU_PROFILE_SCOPE_GPU("Renderer::SkeletalPaletteUpload");

    const Int32 needed = static_cast<Int32>(mSkeletalPaletteScratch.Size());
    if (needed > 0) {
        // Grow with 2x headroom, like the instance buffer.
        if (mSkeletalMatricesBuffer.GetCount() < needed) {
            mSkeletalMatricesBuffer.Resize(needed * 2);
        }
        mSkeletalMatricesBuffer.Update(mSkeletalPaletteScratch.Data(), needed);
    }
    // BindBase AFTER any Resize — Resize creates a new buffer ID and the indexed binding point
    // would otherwise still hold the deleted buffer.
    mSkeletalMatricesBuffer.BindBase(0);
}

void Plu::Renderer::RenderSnapshot(Plu::RenderSnapshot *snapshot)
{
    PLU_PROFILE_SCOPE("Renderer::RenderSnapshot");
    if (!snapshot->IsSnapshotValid) return;

    // Backend ImGui (koniec poprzedniej klatki) bindował programy surowym glUseProgram —
    // cache deduplikacji Bind() startuje klatkę jako "nieznany".
    ShaderProgram::ResetBindCache();

    // Palety skinningu wszystkich skeletal meshy liczone RAZ i wysyłane na GPU RAZ; pass cieni
    // (per kaskada) i pass główny adresują swoje zakresy uniformem "paletteBaseIndex".
    BuildSkeletalPalettes(snapshot);
    UploadSkeletalPalettes();

    // Wskaźniki meshy rozwiązane RAZ na klatkę — konsumują je pass cieni (per kaskada)
    // i pass główny (patrz komentarz przy mResolvedBatchMeshes w Renderer.h).
    ResolveSnapshotMeshes(snapshot);

    // Upload danych instancji SSBO — RAZ na klatkę, PRZED RenderShadowPass (który w fazie 2
    // czyta te same dane). Bufor zostaje zbindowany (BindBase) na binding 1 na całą klatkę,
    // batche adresują swój zakres uniformem "instanceBaseIndex" (patrz komentarz przy
    // mInstanceBuffer w Renderer.h). Zapas 2x, bo SetData/Resize realokują ilekroć liczba
    // elementów się zmienia — bez zapasu scena o zmiennej liczbie widocznych obiektów
    // realokowałaby bufor co klatkę.
    {
        PLU_PROFILE_SCOPE_GPU("Renderer::InstanceUpload");
        const Int32 neededInstances = static_cast<Int32>(snapshot->StaticInstanceData.Size());
        if (neededInstances > 0) {
            if (mInstanceBuffer.GetCount() < neededInstances) {
                mInstanceBuffer.Resize(neededInstances * 2);
            }
            mInstanceBuffer.Update(snapshot->StaticInstanceData);
        }
        mInstanceBuffer.BindBase(1);
    }

    const Matrix4 view = glm::inverse(
        glm::translate(glm::mat4(1.0f), snapshot->CameraLocation) *
        glm::mat4_cast(glm::quat(glm::radians(snapshot->CameraRotation)))
    );

    // Depth shaders resolved once for both shadow passes (see ResolveDepthShaders). A frame
    // without them simply produces no shadow frusta, so everything downstream reads "no shadows".
    ResolveDepthShaders();

    // Both shadow passes are planned BEFORE either draws: the caster culling below covers every
    // frustum of the frame in one sweep, so the cascade matrices and the spot slot matrices both
    // have to exist first. That is what keeps the visible-index SSBO a single upload on binding 3.
    PrepareShadowCascades(snapshot, view);
    PrepareSpotShadowSlots(snapshot);
    snapshot->StatCulledCount += CullShadowCasters(snapshot);

    // Pass 1: mapy głębi kaskad dla światła kierunkowego.
    RenderShadowPass(snapshot);
    // Pass 1b: depth slots of the spot shadow atlas. Shares the visible-index SSBO with the
    // cascades, so it must run after them and before anything rebinds binding 3.
    RenderSpotShadowPass(snapshot);

    // Shadow parameter block — pushed every frame, shadows or not (see UpdateShadowDataBuffer).
    UpdateShadowDataBuffer(snapshot);
    // Spot light blocks — same unconditional rule (see UpdateSpotLightBuffers).
    UpdateSpotLightBuffers(snapshot);

#ifdef PLU_ENGINE_EDITOR_BUILD
    // Debug cascade / spot slot viewers — no-op unless a panel asked for one this frame.
    UpdateShadowLayerView();
    UpdateSpotShadowSlotView();
#endif

    // Pass 2: scena do głównego bufora.
    PLU_PROFILE_SCOPE("Renderer::MainPass");
    PLU_PROFILE_SCOPE_GPU("Renderer::MainPass");
    mMainBuffer->Clear();

    // Pass 0: scene depth for contact shadows. Renders into its OWN framebuffer, so it neither
    // touches nor is touched by the main buffer's clear above.
    EnsureDepthPrepassBuffer();
    RenderDepthPrepass(snapshot, snapshot->CameraProjectionMatrix * view);

    mMainBuffer->Bind();

    // Uniformy globalne (kamera, światło, kaskady, sloty teksturujące) ustawiane raz na klatkę
    // na liście aktywnych shaderów prowadzonej przez ShadersManager — Renderer nie trzyma już
    // własnej listy. Mapy cieni kaskad zajmują pierwsze kCascadeCount slotów (SetSlotsUsed PRZED
    // RenderFromMaterial, które startuje tekstury materiału za nimi). Uniformy nieobecne w danym
    // shaderze są no-opem (location == -1), więc ustawianie ich na wszystkich programach jest bezpieczne.
    DynamicArray<TUsePointer<ShaderProgram>>* activePrograms = mApplicationInfo->AppShaderManager->GetRenderableShaderPrograms();
    const UInt32 programCount = activePrograms ? activePrograms->Size() : 0;

    // Tablica map cieni bindowana RAZ na klatkę na stały slot 0 wraz z samplerem porównującym
    // (to on robi z texture() sprzętowe PCF). Jednostki teksturujące to stan globalny GL, nie
    // per program, a slot samplera jest wpisany w shader przez layout(binding = 0) — w pętli
    // programów nie zostaje już nic per kaskada.
    if (mShadowAtlas) {
        mShadowAtlas->Bind(kShadowTextureUnit);
        mShadowCompareSampler.Bind(kShadowTextureUnit);
        CheckShadowGLError("Renderer::RenderSnapshot (shadow atlas bind)");
    }
    // Spot shadow atlas on its own unit. The SAME comparison sampler object serves both units —
    // glBindSampler binds one sampler object to a unit, and one object may sit on many units.
    if (mSpotShadowArray) {
        mSpotShadowArray->Bind(kSpotShadowTextureUnit);
        mShadowCompareSampler.Bind(kSpotShadowTextureUnit);
        CheckShadowGLError("Renderer::RenderSnapshot (spot shadow atlas bind)");
    }
    // Scene depth for contact shadows. NO comparison sampler here — unlike the shadow maps this is
    // read as a plain depth value to be ray-marched against, not compared against a reference.
    if (mSceneDepthTexture) {
        mSceneDepthTexture->Bind(kSceneDepthTextureUnit);
        CheckShadowGLError("Renderer::RenderSnapshot (scene depth bind)");
    }
    for (UInt32 p = 0; p < programCount; p++) {
        ShaderProgram* program = activePrograms->At(p).GetRaw();
        if (!program) continue;
        // Hot reload: main-thread zgłosił zmianę źródła, tu (na render threadzie z kontekstem GL)
        // rekompilujemy. Recompile przy błędzie kompilacji zostawia stary program załadowany.
        if (program->ConsumeRecompileRequest()) {
            program->Recompile();
        }
        if (!program->IsLoaded()) continue;

        program->SetMatrix4Uniform("view", view);
        program->SetMatrix4Uniform("projection", snapshot->CameraProjectionMatrix);
        program->SetVec3Uniform("cameraPos", snapshot->CameraLocation);

        if (snapshot->HasDirLight) {
            program->SetVec3Uniform("dirLightDir", snapshot->DirLight.Direction);
            program->SetVec4Uniform("dirLightColor", Vec4(snapshot->DirLight.Color, snapshot->DirLight.Intensity));
        }

        // Material textures start at unit 0; the shadow array lives at kShadowTextureUnit, far
        // out of their way (see the comment there). Constant either way, so a frame without a
        // directional light does not silently renumber every material's samplers.
        program->SetSlotsUsed(0);
    }

    // Batche instancingu (grupowanie zrobione na main w RenderSnapshotBuilder::BatchStaticMeshes).
    // Materiały wchodzą w instancing OPT-IN: dopóki ich program nie ma bloku SSBO "InstanceMatrices"
    // (HasInstanceDataBlock), batch leci fallbackiem per-obiekt niżej — bajtowo zgodnym z dawną
    // pętlą po StaticMeshRenderObjects (ten shader MA `uniform mat4 model`).
    // Programy Z blokiem InstanceMatrices (BasicVertInstanced.vert) celowo NIE mają `uniform mat4
    // model` — transform idzie wyłącznie z SSBO. Dla takich programów instanced draw jest jedyną
    // poprawną ścieżką NIEZALEŻNIE od VisibleCount; SetMatrix4Uniform("model", ...) na nich byłby
    // cichym no-opem (lokacja -1), więc pojedyncza instancja renderowałaby się ze śmieciowym/starym
    // transformem z instances[instanceBaseIndex] zamiast własnego.
    const UInt32 staticBatchCount = snapshot->StaticMeshBatches.Size();
    for (UInt32 i = 0; i < staticBatchCount; i++) {
        StaticMeshBatch* batch = &snapshot->StaticMeshBatches[i];
        const TUsePointer<StaticMesh>& staticMesh = mResolvedBatchMeshes[i];
        TUsePointer<MaterialInfo> materialInfo = mApplicationInfo->AppAssetManager->GetAssetDataNoLoad(batch->MaterialUUID);
        if (!materialInfo || !staticMesh) continue;
        if (!staticMesh->IsLoaded) {
            mApplicationInfo->AppRenderingManager->RequestStaticMeshLoad(batch->MeshUUID);
        }
        TUsePointer<ShaderProgram> shaderProgram = mApplicationInfo->AppShaderManager->GetShaderProgram(materialInfo->shaderProgram);
        if (!shaderProgram || !shaderProgram->IsLoaded()) {
            // Leniwa kompilacja na render threadzie (analogicznie do RequestStaticMeshLoad dla meshy);
            // LoadShader rejestruje program w liście aktywnych ShadersManagera, więc w następnej
            // klatce dostanie uniformy globalne powyżej. Tego batcha ta klatka pomija.
            mApplicationInfo->AppShaderManager->LoadShader(materialInfo->shaderProgram);
            continue;
        }

        // Per-batch: tylko materiał (tekstury od slotu kCascadeCount), potem albo jeden
        // glDrawElementsInstanced, albo pętla po instancjach (fallback).
        shaderProgram->RenderFromMaterial(materialInfo.GetRaw(), mApplicationInfo->AppRenderingManager);

        const bool useInstancing = shaderProgram->HasInstanceDataBlock();
        if (useInstancing) {
            shaderProgram->SetIntUniform("instanceBaseIndex", static_cast<int>(batch->InstanceOffset));
            DrawStaticMeshInstanced(staticMesh.GetRaw(), mApplicationInfo->AppRenderingManager.GetRaw(), batch->VisibleCount);
            snapshot->StatDrawCalls++;
            snapshot->StatInstancesDrawn += batch->VisibleCount;
        } else {
            for (UInt32 v = 0; v < batch->VisibleCount; v++) {
                const InstanceGPUData& instance = snapshot->StaticInstanceData[batch->InstanceOffset + v];
                shaderProgram->SetMatrix4Uniform("model", instance.ModelMatrix);
                // Macierz normalnych z CPU (snapshot ma ją już policzoną dla batchingu) —
                // BasicVert.vert nie robi już transpose(inverse()) per wierzchołek.
                shaderProgram->SetMatrix4Uniform("normalMatrix", instance.NormalMatrix);
                DrawStaticMesh(staticMesh.GetRaw(), mApplicationInfo->AppRenderingManager.GetRaw());
            }
            snapshot->StatDrawCalls += batch->VisibleCount;
            snapshot->StatInstancesDrawn += batch->VisibleCount;
            if (batch->VisibleCount > 1 && !mWarnedNonInstancedPrograms.Contains(materialInfo->shaderProgram.getUUID())) {
                mWarnedNonInstancedPrograms.Insert(materialInfo->shaderProgram.getUUID());
                PLU_CORE_WARN("Batch of {} static mesh instances uses material {} whose shader program {} has no 'InstanceMatrices' SSBO block — "
                              "falling back to one draw call per instance. Use a program with an instanced vertex shader (e.g. BasicVertInstanced.vert) to enable instancing.",
                              batch->VisibleCount, batch->MaterialUUID.getUUID(), materialInfo->shaderProgram.getUUID());
            }
        }
    }

    UInt64 skeletalMeshCount = snapshot->SkeletalMeshRenderObjects.Size();
    for (UInt32 i = 0; i < skeletalMeshCount; i++) {
        SkeletalMeshRenderObject* renderObject = &snapshot->SkeletalMeshRenderObjects[i];
        const TUsePointer<SkeletalMesh>& skeletalMesh = mResolvedSkeletalMeshes[i];
        TUsePointer<MaterialInfo> materialInfo = mApplicationInfo->AppAssetManager->GetAssetDataNoLoad(renderObject->MaterialUUID);
        if (!materialInfo || !skeletalMesh) continue;
        if (!skeletalMesh->IsLoaded) {
            mApplicationInfo->AppRenderingManager->RequestSkeletalMeshLoad(renderObject->MeshUUID);
        }
        TUsePointer<ShaderProgram> shaderProgram = mApplicationInfo->AppShaderManager->GetShaderProgram(materialInfo->shaderProgram);
        if (!shaderProgram || !shaderProgram->IsLoaded()) {
            // Leniwa kompilacja na render threadzie (analogicznie do RequestStaticMeshLoad dla meshy);
            // LoadShader rejestruje program w liście aktywnych ShadersManagera, więc w następnej
            // klatce dostanie uniformy globalne powyżej. Tej klatki mesh jest pomijany.
            mApplicationInfo->AppShaderManager->LoadShader(materialInfo->shaderProgram);
            continue;
        }

        // Materiał na programie bez skinningu (brak bloku SSBO "BoneMatrices" w vertex shaderze)
        // rysuje skeletal mesh zamrożony w bind pose — po cichu. Ostrzegamy raz per program.
        if (!shaderProgram->HasBoneMatricesBlock() && !mWarnedNonSkeletalPrograms.Contains(materialInfo->shaderProgram.getUUID())) {
            mWarnedNonSkeletalPrograms.Insert(materialInfo->shaderProgram.getUUID());
            PLU_CORE_WARN("Skeletal mesh {} uses material {} whose shader program {} has no 'BoneMatrices' SSBO block — "
                          "no skinning, mesh will stay in bind pose. Use a program with a skeletal vertex shader (e.g. BasicVertSkeletal.vert).",
                          renderObject->MeshUUID.getUUID(), renderObject->MaterialUUID.getUUID(), materialInfo->shaderProgram.getUUID());
        }

        // Per-mesh: tylko materiał (tekstury od slotu 1) + offset palety w buforze wysłanym raz
        // na klatkę (UploadSkeletalPalettes) + model + rysowanie.
        shaderProgram->RenderFromMaterial(materialInfo.GetRaw(), mApplicationInfo->AppRenderingManager);

        shaderProgram->SetIntUniform("paletteBaseIndex", static_cast<int>(mSkeletalPaletteRanges[i].Offset));
        shaderProgram->SetMatrix4Uniform("model", renderObject->ModelMatrix);
        // Macierz normalnych z CPU — raz per obiekt zamiast transpose(inverse()) per
        // wierzchołek w BasicVertSkeletal.vert (jak dotąd z samego modelu, bez skinningu).
        shaderProgram->SetMatrix4Uniform("normalMatrix", glm::transpose(glm::inverse(renderObject->ModelMatrix)));
        DrawSkeletalMesh(skeletalMesh.GetRaw(), mApplicationInfo->AppRenderingManager.GetRaw());
    }

#ifdef PLU_ENGINE_EDITOR_BUILD
    // Pass 3: editor grid, blended over the scene. Before debug geometry, so physics
    // wireframes/points draw on top of the grid.
    RenderEditorGrid(snapshot, view);

    // Pass 4: debugowa geometria fizyki (linie + punkty) do tego samego bufora.
    RenderDebugGeometry(snapshot, snapshot->CameraProjectionMatrix * view);
#endif

    mMainBuffer->Unbind();

    // Hand texture unit 0 back as plain, unbound state: the ImGui backend binds its own textures
    // there right after this (a comparison sampler left over it would render the whole editor UI
    // black), and the next frame's depth pass renders INTO this array.
    UnbindShadowTexture();
    UnbindSpotShadowTexture();
    UnbindSceneDepthTexture();
    CheckShadowGLError("Renderer::RenderSnapshot (frame end)");

    // Publikacja liczników tej klatki dla panelu Render/GPU (main thread) — snapshot->Stat*
    // było tylko roboczym akumulatorem powyżej, ta klatka jest teraz skończona.
    SetRenderFrameStats(snapshot->StatDrawCalls, snapshot->StatInstancesDrawn, snapshot->StatCulledCount);
    SetShadowCascadeStats(mCascadeCasterCounts.Data(), mCascadeCasterCounts.Size());
    SetSpotLightStats(mSpotShadowCasterCounts.Data(), mSpotShadowCasterCounts.Size(), snapshot->SpotLights.Size());
}

void Plu::Renderer::RenderDebugGeometry(Plu::RenderSnapshot *snapshot, const Matrix4 &viewProj)
{
    if (snapshot->DebugLineVerts.IsEmpty() && snapshot->DebugPointVerts.IsEmpty()) return;

    PLU_PROFILE_SCOPE("Renderer::RenderDebugGeometry");
    PLU_PROFILE_SCOPE_GPU("Renderer::RenderDebugGeometry");

    TUsePointer<ShaderProgram> shader = mApplicationInfo->AppShaderManager->GetShaderProgram(EngineAssets::DebugLine);
    if (!shader) return;
    if (!shader->IsLoaded()) {
        // Leniwa kompilacja na render threadzie (parytet z passem materiałów/cieni); rysowanie
        // pojawi się w kolejnej klatce, gdy shader będzie gotowy.
        mApplicationInfo->AppShaderManager->LoadShader(shader->Uuid);
        return;
    }

    shader->SetMatrix4Uniform("uViewProj", viewProj);

    glBindVertexArray(mDebugVao);
    glBindBuffer(GL_ARRAY_BUFFER, mDebugVbo);

    if (!snapshot->DebugLineVerts.IsEmpty()) {
        glBufferData(GL_ARRAY_BUFFER, snapshot->DebugLineVerts.Size() * sizeof(float),
                     snapshot->DebugLineVerts.Data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(snapshot->DebugLineVerts.Size() / 6));
    }

    if (!snapshot->DebugPointVerts.IsEmpty()) {
        glPointSize(snapshot->DebugPointSize);
        glBufferData(GL_ARRAY_BUFFER, snapshot->DebugPointVerts.Size() * sizeof(float),
                     snapshot->DebugPointVerts.Data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(snapshot->DebugPointVerts.Size() / 6));
        glPointSize(1.0f);
    }

    glBindVertexArray(0);
}

void Plu::Renderer::RenderEditorGrid(Plu::RenderSnapshot *snapshot, const Matrix4 &view)
{
    if (!snapshot->ShowEditorGrid) return;

    PLU_PROFILE_SCOPE("Renderer::RenderEditorGrid");
    PLU_PROFILE_SCOPE_GPU("Renderer::RenderEditorGrid");

    TUsePointer<ShaderProgram> shader = mApplicationInfo->AppShaderManager->GetShaderProgram(EngineAssets::EditorGridProgram);
    if (!shader) return;
    if (!shader->IsLoaded()) {
        // Leniwa kompilacja na render threadzie (parytet z DebugLine); siatka pojawi się
        // w kolejnej klatce, gdy shader będzie gotowy.
        mApplicationInfo->AppShaderManager->LoadShader(shader->Uuid);
        return;
    }

    const Matrix4 viewProj = snapshot->CameraProjectionMatrix * view;
    shader->SetMatrix4Uniform("uViewProj", viewProj);
    shader->SetMatrix4Uniform("uInvViewProj", glm::inverse(viewProj));
    shader->SetVec3Uniform("uCameraPos", snapshot->CameraLocation);

    // Depth test stays on — EditorGrid.frag writes the plane point's real depth, so scene
    // geometry occludes the grid. Depth writes go off for the pass: the blended grid must
    // not occlude anything drawn after it (debug geometry).
    glDepthMask(GL_FALSE);
    glBindVertexArray(mGridVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}

void Plu::Renderer::Shutdown()
{
    DestroyShadowResources();
    DestroySpotShadowResources();
    // Observer first — the depth texture is owned by the framebuffer destroyed right after.
    mSceneDepthTexture = nullptr;
    if (mDepthPrepassBuffer) {
        mApplicationInfo->AppObjectManager->DestroyObject(mDepthPrepassBuffer->GetObjectHandle());
        mDepthPrepassBuffer->Destroy();
        mDepthPrepassBuffer = nullptr;
    }
    mShadowCompareSampler.Destroy();
    if (mShadowLayerView) {
        mApplicationInfo->AppObjectManager->DestroyObject(mShadowLayerView->GetObjectHandle());
        mShadowLayerView->Destroy();
        mShadowLayerView = nullptr;
    }
    if (mSpotShadowView) {
        mApplicationInfo->AppObjectManager->DestroyObject(mSpotShadowView->GetObjectHandle());
        mSpotShadowView->Destroy();
        mSpotShadowView = nullptr;
    }
    mVisibleInstanceBuffer.Destroy();
    mCascades.Clear();
    mCascadeSplits.Clear();
    mShadowDataBuffer.Destroy();

    mSpotShadowSlotOwners.Clear();
    mSpotShadowMatrices.Clear();
    mSpotShadowCasterCounts.Clear();
    mSpotLightBuffer.Destroy();
    mSpotLightIndexBuffer.Destroy();
    mSpotLightDataBuffer.Destroy();

    if (mDebugVao) { glDeleteVertexArrays(1, &mDebugVao); mDebugVao = 0; }
    if (mDebugVbo) { glDeleteBuffers(1, &mDebugVbo); mDebugVbo = 0; }
    if (mGridVao) { glDeleteVertexArrays(1, &mGridVao); mGridVao = 0; }

    mApplicationInfo->AppObjectManager->DestroyObject(mMainBuffer->GetObjectHandle());
    mMainBuffer->Destroy();
    mMainBuffer = nullptr;
}
