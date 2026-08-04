//
// Created by Plutex on 6/22/26.
//

#include "PluEngine/Renderer/Renderer.h"

#include <glad/glad.h>
#include "PluEngine/Application.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Renderer/RenderThreading.h"
#include "PluEngine/Renderer/RenderUtils.h"
#include "PluEngine/Shaders/ShaderProgram.h"
#include "PluEngine/Window/Window.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/Managers/RenderingManager.h"
#include "PluEngine/Managers/ShadersManager.h"
#include "EngineAssets.h"
#include "PluEngine/Renderer/GPUProfiler.h"
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
    GLint maxTextureUnits = 0;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
    if (maxTextureUnits <= static_cast<GLint>(kShadowTextureUnit)) {
        PLU_CORE_ERROR("Renderer::Initialize - GL_MAX_TEXTURE_IMAGE_UNITS is {}, but the shadow map array needs unit {}. "
                       "Directional shadows will not sample correctly.", maxTextureUnits, kShadowTextureUnit);
    }

    // Shadow depth array + one framebuffer per layer, created eagerly — the GL context is on
    // the render thread here, so the frame path never allocates GL objects.
    RecreateShadowResources(kDefaultShadowResolution, kCascadeCount);

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
}

void Plu::Renderer::RecreateShadowResources(Int32 Resolution, Int32 LayerCount)
{
    if (Resolution == mShadowResolution && LayerCount == mShadowLayerCount && mShadowDepthArray) {
        return;
    }

    DestroyShadowResources();

    // Anything still in the error queue would otherwise be blamed on the calls below.
    CheckShadowGLError("Renderer::RecreateShadowResources (entry)");

    EngineObjectHandle textureHandle = mApplicationInfo->AppObjectManager->CreateObject<Texture>();
    mShadowDepthArray = mApplicationInfo->AppObjectManager->GetObjectAsOwner<Texture>(textureHandle);
    // D32F, not D16: the depth range is no longer padded by a 50 m near margin (GL_DEPTH_CLAMP
    // replaced it), so the extra bits go straight into fighting acne instead of covering slack.
    if (!mShadowDepthArray->CreateDepthArray(Resolution, Resolution, LayerCount)
        || !CheckShadowGLError("Renderer::RecreateShadowResources (depth array)")) {
        PLU_CORE_ERROR("Renderer::RecreateShadowResources - Failed to create the shadow depth array ({}x{}, {} layers)",
                       Resolution, Resolution, LayerCount);
        DestroyShadowResources();
        return;
    }

    mCascadeFrameBuffers.Reserve(static_cast<UInt32>(LayerCount));
    for (Int32 layer = 0; layer < LayerCount; layer++) {
        EngineObjectHandle fbHandle = mApplicationInfo->AppObjectManager->CreateObject<FrameBuffer>();
        TOwningPointer<FrameBuffer> fb = mApplicationInfo->AppObjectManager->GetObjectAsOwner<FrameBuffer>(fbHandle);
        // A layer framebuffer that failed to create must NOT be kept: FrameBuffer::Clear() and
        // Bind() silently no-op / bind framebuffer 0 on an invalid object, so the cascade would
        // never be cleared nor rendered — and an uncleared depth array reads as "everything is
        // occluded", i.e. a fully black scene. Bail out of shadows entirely instead.
        if (!fb->CreateWithDepthTextureLayer(mShadowDepthArray, layer, mApplicationInfo->AppObjectManager)
            || !CheckShadowGLError("Renderer::RecreateShadowResources (layer framebuffer)")) {
            PLU_CORE_ERROR("Renderer::RecreateShadowResources - Failed to create the framebuffer for cascade layer {} — directional shadows disabled", layer);
            mApplicationInfo->AppObjectManager->DestroyObject(fb->GetObjectHandle());
            fb->Destroy();
            DestroyShadowResources();
            return;
        }
        mCascadeFrameBuffers.PushBack(fb);
    }

    mShadowResolution = Resolution;
    mShadowLayerCount = LayerCount;

    PLU_CORE_INFO("Shadow resources ready: {}x{} D32F array, {} cascade layers", Resolution, Resolution, LayerCount);
}

void Plu::Renderer::UnbindShadowTexture()
{
    glActiveTexture(GL_TEXTURE0 + kShadowTextureUnit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
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
    if (layer < 0 || !mShadowDepthArray || layer >= mShadowLayerCount) return;

    PLU_PROFILE_SCOPE("Renderer::UpdateShadowLayerView");

    // Destination is rebuilt whenever the geometry stops matching, so the viewer keeps working
    // across a resolution change instead of showing a stale, wrongly sized copy.
    if (!mShadowLayerView || mShadowLayerView->GetWidth() != mShadowResolution) {
        if (mShadowLayerView) {
            mApplicationInfo->AppObjectManager->DestroyObject(mShadowLayerView->GetObjectHandle());
            mShadowLayerView->Destroy();
            mShadowLayerView = nullptr;
        }
        EngineObjectHandle handle = mApplicationInfo->AppObjectManager->CreateObject<Texture>();
        mShadowLayerView = mApplicationInfo->AppObjectManager->GetObjectAsOwner<Texture>(handle);
        mShadowLayerView->CreateDepth(mShadowResolution, mShadowResolution);
    }

    // Straight image copy — no framebuffer, no shader, no format conversion. Both textures are
    // D32F, so the driver can move the whole layer in one go.
    glCopyImageSubData(mShadowDepthArray->GetID(), GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer,
                       mShadowLayerView->GetID(), GL_TEXTURE_2D, 0, 0, 0, 0,
                       mShadowResolution, mShadowResolution, 1);
}

void Plu::Renderer::DestroyShadowResources()
{
    // Framebuffers first: they hold a reference to the array texture and must not outlive it.
    for (UInt32 c = 0; c < mCascadeFrameBuffers.Size(); c++) {
        if (!mCascadeFrameBuffers[c]) continue;
        mApplicationInfo->AppObjectManager->DestroyObject(mCascadeFrameBuffers[c]->GetObjectHandle());
        mCascadeFrameBuffers[c]->Destroy();
        mCascadeFrameBuffers[c] = nullptr;
    }
    mCascadeFrameBuffers.Clear();

    if (mShadowDepthArray) {
        mApplicationInfo->AppObjectManager->DestroyObject(mShadowDepthArray->GetObjectHandle());
        mShadowDepthArray->Destroy();
        mShadowDepthArray = nullptr;
    }

    mShadowResolution = -1;
    mShadowLayerCount = -1;
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

void Plu::Renderer::RenderShadowPass(Plu::RenderSnapshot *snapshot, const Matrix4& cameraView)
{
    PLU_PROFILE_SCOPE("Renderer::RenderShadowPass");
    PLU_PROFILE_SCOPE_GPU("Renderer::RenderShadowPass");
    mCascades.Clear();
    // Cleared here, not in CullShadowCasters — a frame that bails out early (no light, shaders
    // not ready) must not publish last frame's caster counts to the stats panel.
    mCascadeCasterCounts.Clear();

    const DirectionalLightShadowSettings settings = ClampShadowSettings(snapshot->DirLight.Shadow);
    mShadowSettings = settings;

    if (!snapshot->HasDirLight || !settings.CastShadows) {
        // No directional shadows this frame. Clear the depth maps ONCE on the transition so
        // leftover content (e.g. shadows baked during a previous PIE session) doesn't linger,
        // then stop touching them — with CascadeCount = 0 nothing samples them anyway.
        if (!mShadowMapsCleared) {
            for (UInt32 c = 0; c < mCascadeFrameBuffers.Size(); c++) {
                mCascadeFrameBuffers[c]->Clear(0.0f, 0.0f, 0.0f, 1.0f);
            }
            mShadowMapsCleared = true;
        }
        return;
    }

    // Resolution / cascade count are settings, so the GL resources may need rebuilding. Safe
    // here: the render thread owns the GL context, and nothing samples the array until the
    // main pass below.
    RecreateShadowResources(settings.Resolution, settings.CascadeCount);
    if (mCascadeFrameBuffers.IsEmpty()) {
        return;
    }
    mShadowMapsCleared = false;

    // Shader głębi instancingu (tylko pozycja, SSBO InstanceMatrices) dla static meshy — leniwa
    // kompilacja na wątku renderu. Zastępuje dawny nieinstancowany OnlyPositionShader: depth pass
    // jest silnikowy (nie opt-in per materiał jak główny pass), a SSBO instancji jest już
    // wypełniony i zbindowany (Renderer::mInstanceBuffer, przed RenderShadowPass) dla wszystkich
    // batchy niezależnie od tego, czy materiał widocznego passu wspiera instancing.
    TUsePointer<ShaderProgram> depthShader = mApplicationInfo->AppShaderManager->GetShaderProgram(EngineAssets::OnlyPositionInstancedShader);
    if (!depthShader) return;
    if (!depthShader->IsLoaded()) {
        mApplicationInfo->AppShaderManager->LoadShader(EngineAssets::OnlyPositionInstancedShader);
        return; // gotowe w kolejnej klatce
    }

    // Skinowany wariant shadera głębi dla skeletal meshy — ładowany niezależnie: jeśli jeszcze nie
    // gotowy, pomijamy tylko cienie skeletalne (static-owe i tak lecą), a nie cały pass.
    TUsePointer<ShaderProgram> skeletalDepthShader = mApplicationInfo->AppShaderManager->GetShaderProgram(EngineAssets::OnlyPositionSkeletalShader);
    const bool skeletalDepthReady = skeletalDepthShader && skeletalDepthShader->IsLoaded();
    if (skeletalDepthShader && !skeletalDepthShader->IsLoaded()) {
        mApplicationInfo->AppShaderManager->LoadShader(EngineAssets::OnlyPositionSkeletalShader);
    }

    TUsePointer<IWindow> window = mApplicationInfo->AppWindow;
    const float aspect = static_cast<float>(window->GetWidth()) / static_cast<float>(window->GetHeight());
    const float fovRad = glm::radians(snapshot->CameraFOV);

    // A higher lambda packs the near cascades tighter against the camera — sharper close-up
    // shadows. At the defaults (4 cascades, 150 m, lambda 0.9) the splits land around
    // 1.3 / 7 / 33 / 150 m.
    CascadeConfig cascadeConfig;
    cascadeConfig.CascadeCount   = settings.CascadeCount;
    cascadeConfig.ShadowDistance = settings.ShadowDistance;
    cascadeConfig.SplitLambda    = settings.SplitLambda;
    cascadeConfig.Resolution     = settings.Resolution;

    ComputeCascadeSplits(cascadeConfig, kCameraNearClip, mCascadeSplits);
    ComputeCascadeMatrices(
        cameraView, fovRad, aspect,
        kCameraNearClip,
        snapshot->DirLight.Direction,
        cascadeConfig,
        mCascadeSplits,
        mCascades
    );

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

    // Per-cascade caster culling, done once for all cascades so the visible-index SSBO is a
    // single upload (see CullShadowCasters).
    snapshot->StatCulledCount += CullShadowCasters(snapshot);

    const UInt32 staticBatchCount = snapshot->StaticMeshBatches.Size();

    for (UInt32 c = 0; c < mCascades.Size(); c++) {
        PLU_PROFILE_SCOPE_GPU(CascadeGpuScopeNames()[c]);

        mCascadeFrameBuffers[c]->Clear(0.0f, 0.0f, 0.0f, 1.0f);
        mCascadeFrameBuffers[c]->Bind(); // ustawia glViewport na rozmiar mapy cienia

        // Static meshe — instanced shader głębi, batche z RenderSnapshotBuilder::BatchStaticMeshes.
        // Kamerowy culling z batchowania (VisibleCount) jest dla cieni bezużyteczny — caster poza
        // kadrem kamery może rzucać cień w kadr — więc pass cieni cullinguje sam, per kaskada, i
        // adresuje instancje przez skompaktowaną tablicę indeksów. Jeden glDrawElementsInstanced
        // na batch, a batch niewidoczny w tej kaskadzie odpada bez draw calla.
        depthShader->SetMatrix4Uniform("lightSpaceMatrix", mCascades[c].ViewProj);
        for (UInt32 i = 0; i < staticBatchCount; i++) {
            const ShadowDrawRange& range = mShadowDrawRanges[c * staticBatchCount + i];
            if (range.Count == 0) continue;
            const TUsePointer<StaticMesh>& staticMesh = mResolvedBatchMeshes[i];
            if (!staticMesh || !staticMesh->IsLoaded) continue;
            // For the depth shader instanceBaseIndex indexes the VISIBLE-INDEX buffer, not the
            // instance buffer — the extra indirection is what lets a cascade draw a subset.
            depthShader->SetIntUniform("instanceBaseIndex", static_cast<int>(range.Offset));
            DrawStaticMeshInstanced(staticMesh.GetRaw(), mApplicationInfo->AppRenderingManager.GetRaw(), range.Count);
            snapshot->StatDrawCalls++;
        }

        // Skeletal meshe — skinowany shader głębi z tą samą paletą kości co główny pass, dzięki
        // czemu cień podąża za animacją. Palety WSZYSTKICH meshy poszły na GPU raz na klatkę
        // (UploadSkeletalPalettes), więc tutaj zostaje tylko offset w tym buforze — dawniej każdy
        // obiekt nadpisywał wspólny bufor, per kaskada, czyli 5x ten sam upload co klatkę.
        if (skeletalDepthReady) {
            skeletalDepthShader->SetMatrix4Uniform("lightSpaceMatrix", mCascades[c].ViewProj);
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

                skeletalDepthShader->SetIntUniform("paletteBaseIndex", static_cast<int>(mSkeletalPaletteRanges[i].Offset));
                skeletalDepthShader->SetMatrix4Uniform("model", renderObject->ModelMatrix);
                DrawSkeletalMesh(skeletalMesh.GetRaw(), mApplicationInfo->AppRenderingManager.GetRaw());
                snapshot->StatDrawCalls++;
                mCascadeCasterCounts[c]++;
            }
        }

        mCascadeFrameBuffers[c]->Unbind();
        CheckShadowGLError("Renderer::RenderShadowPass (cascade draw)");
    }

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

    const UInt32 cascadeCount = mCascades.Size();
    const UInt32 batchCount   = snapshot->StaticMeshBatches.Size();

    mVisibleInstanceScratch.Clear();
    mShadowDrawRanges.Clear();
    mShadowDrawRanges.Resize(cascadeCount * batchCount);
    mCascadeCasterCounts.Clear();
    mCascadeCasterCounts.Resize(cascadeCount);

    UInt32 culledCount = 0;

    for (UInt32 c = 0; c < cascadeCount; c++) {
        // No near plane: depth-clamped casters legitimately sit in front of it (see
        // SphereInFrustumNoNear), and culling them would remove the very objects casting into
        // this cascade.
        const Frustum cascadeFrustum = ExtractFrustumPlanes(mCascades[c].ViewProj);

        for (UInt32 b = 0; b < batchCount; b++) {
            const StaticMeshBatch& batch = snapshot->StaticMeshBatches[b];
            ShadowDrawRange& range = mShadowDrawRanges[c * batchCount + b];
            range.Offset = static_cast<UInt32>(mVisibleInstanceScratch.Size());
            range.Count  = 0;

            if (!batch.CastsShadow || batch.TotalCount == 0) continue;

            const UInt32 end = batch.InstanceOffset + batch.TotalCount;
            for (UInt32 instance = batch.InstanceOffset; instance < end; instance++) {
                const InstanceCullData& bounds = snapshot->StaticInstanceBounds[instance];
                if (!SphereInFrustumNoNear(cascadeFrustum, bounds.BoundsCenter, bounds.BoundsRadius)) {
                    culledCount++;
                    continue;
                }
                mVisibleInstanceScratch.PushBack(instance);
                range.Count++;
            }
            mCascadeCasterCounts[c] += range.Count;
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

void Plu::Renderer::UpdateShadowDataBuffer(Plu::RenderSnapshot* snapshot)
{
    PLU_PROFILE_SCOPE("Renderer::UpdateShadowDataBuffer");

    mShadowData = ShadowDataGPU();
    mShadowData.CascadeCount = static_cast<Int32>(mCascades.Size());

    for (UInt32 c = 0; c < mCascades.Size() && c < static_cast<UInt32>(kMaxShadowCascades); c++) {
        const ShadowCascadeData& cascade = mCascades[c];
        mShadowData.CascadeViewProj[c] = cascade.ViewProj;
        mShadowData.CascadeSplits[c]     = cascade.SplitDistance;
        mShadowData.CascadeTexelSizes[c] = cascade.TexelWorldSize;
        // The depth bias is authored in METRES; converting it into each cascade's own [0,1]
        // depth range here is what makes "5 mm of bias" mean 5 mm in every cascade. Doing it on
        // the CPU also retires the per-fragment GLSL helper that used to recover the same scale
        // from the light matrix.
        mShadowData.CascadeDepthBias[c]  = cascade.DepthRange > 0.0f
                                         ? mShadowSettings.DepthBias / cascade.DepthRange
                                         : 0.0f;
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
    mShadowData.InvShadowMapResolution = mShadowResolution > 0
                                       ? 1.0f / static_cast<float>(mShadowResolution)
                                       : 0.0f;

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

    // Pass 1: mapy głębi kaskad dla światła kierunkowego.
    RenderShadowPass(snapshot, view);

    // Shadow parameter block — pushed every frame, shadows or not (see UpdateShadowDataBuffer).
    UpdateShadowDataBuffer(snapshot);

#ifdef PLU_ENGINE_EDITOR_BUILD
    // Debug cascade viewer — no-op unless a panel asked for a layer this frame.
    UpdateShadowLayerView();
#endif

    // Pass 2: scena do głównego bufora.
    PLU_PROFILE_SCOPE("Renderer::MainPass");
    PLU_PROFILE_SCOPE_GPU("Renderer::MainPass");
    mMainBuffer->Clear();
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
    if (mShadowDepthArray) {
        mShadowDepthArray->Bind(kShadowTextureUnit);
        mShadowCompareSampler.Bind(kShadowTextureUnit);
        CheckShadowGLError("Renderer::RenderSnapshot (shadow array bind)");
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
    CheckShadowGLError("Renderer::RenderSnapshot (frame end)");

    // Publikacja liczników tej klatki dla panelu Render/GPU (main thread) — snapshot->Stat*
    // było tylko roboczym akumulatorem powyżej, ta klatka jest teraz skończona.
    SetRenderFrameStats(snapshot->StatDrawCalls, snapshot->StatInstancesDrawn, snapshot->StatCulledCount);
    SetShadowCascadeStats(mCascadeCasterCounts.Data(), mCascadeCasterCounts.Size());
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
    mShadowCompareSampler.Destroy();
    if (mShadowLayerView) {
        mApplicationInfo->AppObjectManager->DestroyObject(mShadowLayerView->GetObjectHandle());
        mShadowLayerView->Destroy();
        mShadowLayerView = nullptr;
    }
    mVisibleInstanceBuffer.Destroy();
    mCascades.Clear();
    mCascadeSplits.Clear();
    mShadowDataBuffer.Destroy();

    if (mDebugVao) { glDeleteVertexArrays(1, &mDebugVao); mDebugVao = 0; }
    if (mDebugVbo) { glDeleteBuffers(1, &mDebugVbo); mDebugVbo = 0; }
    if (mGridVao) { glDeleteVertexArrays(1, &mGridVao); mGridVao = 0; }

    mApplicationInfo->AppObjectManager->DestroyObject(mMainBuffer->GetObjectHandle());
    mMainBuffer->Destroy();
    mMainBuffer = nullptr;
}
