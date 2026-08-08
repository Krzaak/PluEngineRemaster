//
// Created by Plutex on 6/22/26.
//

#ifndef PLUENGINE_RENDERER_H
#define PLUENGINE_RENDERER_H

#include <atomic>

#include "GLFrameBuffer.h"
#include "GLSamplerObject.h"
#include "GLShaderStorageBuffer.h"
#include "GLUniformBuffer.h"
#include "HashSet/HashSet.h"
#include "PluEngine/Core.h"
#include "PluEngine/PluTypes.h"
#include "PluEngine/Objects/EngineObject.h"
#include "RenderThreading.h"
#include "RenderUtils.h"

namespace Plu
{
    class ShaderProgram;
    struct RenderSnapshot;
    struct StaticMesh;
    struct SkeletalMesh;

    //This is a render thread only object
    class Renderer
    {
        // Number of directional shadow cascades. Bounded by kMaxShadowCascades, which also sizes
        // the GLSL arrays in the ShadowData uniform block — one place, no hand-synced #define.
        static constexpr int kCascadeCount = kMaxShadowCascades;

        // Default shadow map resolution, identical for every cascade. One 2048² D32F array of 4
        // layers is ~67 MB — less VRAM AND less fill than the old mixed 4096/2048/1024 set,
        // because the far cascades no longer waste texels on ground the camera barely resolves.
        static constexpr Int32 kDefaultShadowResolution = 2048;

        ApplicationInfo* mApplicationInfo;
        TOwningPointer<FrameBuffer> mMainBuffer;

        // All cascade depth maps in ONE GL_TEXTURE_2D_ARRAY. The lighting pass samples it with a
        // layer index instead of binding N samplers to N units (indexing a sampler array with a
        // non-uniform expression is formally UB, and it burned a texture unit per cascade).
        TOwningPointer<Texture> mShadowDepthArray;
        // One depth-only framebuffer per layer of mShadowDepthArray. They stay EngineObjects so
        // debug panels can still see them, and none of them owns the shared texture.
        // Created eagerly in Initialize (the render thread holds the GL context), so the frame
        // path never does a CreateObject.
        DynamicArray<TOwningPointer<FrameBuffer>> mCascadeFrameBuffers;
        // Comparison sampler bound over the shadow array during the lighting pass only — that is
        // what makes `texture()` do hardware PCF. It must be unbound before ImGui runs (a
        // comparison sampler on a colour texture renders it black), see the end of RenderSnapshot.
        SamplerObject mShadowCompareSampler;
        // Texture unit reserved for the shadow array. Deliberately the LAST unit guaranteed by
        // GL 4.5 (GL_MAX_TEXTURE_IMAGE_UNITS >= 16), not unit 0, and material textures keep
        // starting at 0 (SetSlotsUsed stays 0).
        //
        // Reason: RenderFromMaterial only calls SetTextureUniform for samplers that actually got
        // a texture, so a material sampler with nothing assigned keeps GLSL's default unit 0.
        // Two sampler uniforms of DIFFERENT types on the same unit inside one program is
        // INVALID_OPERATION at draw time — some drivers ignore it, Mesa rejects the draw and the
        // scene goes black. Parking the shadow array at the far end removes the whole class.
        // Must match `layout(binding = 15)` in PBR.frag.
        static constexpr GLuint kShadowTextureUnit = 15;

        // Releases the shadow unit back to plain, unbound state (texture AND sampler).
        //
        // Must be called before the depth pass renders INTO the array: leaving it bound for
        // sampling while it is the render target is a framebuffer feedback loop — undefined by
        // spec, tolerated by some drivers and rejected with INVALID_OPERATION by others (Mesa),
        // which then leaves the cascade maps unwritten and the whole scene reading as occluded.
        // Also called at the end of the frame, because a comparison sampler left on unit 0
        // renders every ImGui texture black.
        void UnbindShadowTexture();

        // Current GL-side geometry of the shadow resources, so a frame can detect that the
        // settings changed and rebuild. -1 = nothing created yet.
        Int32 mShadowResolution = -1;
        Int32 mShadowLayerCount = -1;

        // This frame's clamped shadow settings (from the DirectionalLight, via the snapshot).
        DirectionalLightShadowSettings mShadowSettings;
        // True while the depth maps are known to be blank. Turning shadows off clears them once
        // instead of every frame; nothing samples them at CascadeCount = 0 anyway.
        bool mShadowMapsCleared = false;

        // Brings user-authored settings into the range the renderer can actually honour
        // (cascade count, power-of-two resolution, sane biases). One place, so no consumer
        // downstream has to be defensive.
        static DirectionalLightShadowSettings ClampShadowSettings(const DirectionalLightShadowSettings& Settings);

        // (Re)builds the shadow depth array and its layer framebuffers at the given geometry.
        // No-op when the geometry already matches. Render thread only (it touches GL).
        void RecreateShadowResources(Int32 Resolution, Int32 LayerCount);
        void DestroyShadowResources();

        // Cascade state of the current frame, rebuilt by PrepareShadowCascades and consumed by
        // the main pass. Members (not locals) so the per-frame rebuild reuses their capacity —
        // Clear() keeps it, so the steady state allocates nothing. Empty = no shadows this frame.
        DynamicArray<ShadowCascadeData> mCascades;
        DynamicArray<float> mCascadeSplits;

        // --- Spot light shadows ---
        //
        // Texture unit for the spot shadow atlas, one below the cascade array. Same reasoning as
        // kShadowTextureUnit: engine samplers grow DOWN from the last guaranteed unit while
        // material textures grow up from 0, so an unassigned material sampler sitting on its
        // default unit 0 can never collide with a sampler of a different type (which is
        // INVALID_OPERATION at draw time and blanks the pass on Mesa).
        // Must match `layout(binding = 14)` in PBR.frag.
        static constexpr GLuint kSpotShadowTextureUnit = 14;

        // All spot shadow slots in ONE GL_TEXTURE_2D_ARRAY, exactly like the cascades — a light
        // indexes its layer with a per-light variable, which is legal for an array layer and is
        // NOT legal for an array of samplers.
        TOwningPointer<Texture> mSpotShadowArray;
        DynamicArray<TOwningPointer<FrameBuffer>> mSpotShadowFrameBuffers;  // one layer FBO per slot
        // Current GL-side geometry, so Recreate can no-op. -1 = nothing created yet. Unlike the
        // cascades these never change at runtime (they are engine constants, not light settings).
        Int32 mSpotShadowResolution = -1;
        Int32 mSpotShadowSlotCount  = -1;

        // Same feedback-loop and ImGui reasons as UnbindShadowTexture — see there.
        void UnbindSpotShadowTexture();
        // Builds the atlas and its layer framebuffers at kSpotShadowResolution /
        // kMaxSpotShadowSlots. No-op once they exist. Render thread only (touches GL).
        void RecreateSpotShadowResources();
        void DestroySpotShadowResources();

        // Slot assignment of the current frame. mSpotShadowSlotOwners[slot] is an index into
        // snapshot->SpotLights (which arrives sorted by importance, so the assignment is just
        // "walk the front of the list"), and mSpotShadowMatrices[slot] is that light's
        // projection. Both empty = no spot shadows this frame.
        DynamicArray<Int32>   mSpotShadowSlotOwners;
        DynamicArray<Matrix4> mSpotShadowMatrices;
        // Per-slot caster counts for the stats panel, mirroring mCascadeCasterCounts.
        DynamicArray<UInt32>  mSpotShadowCasterCounts;

        // CPU mirrors + GPU buffers of the per-frame light list. Allocated ONCE at
        // kMaxVisibleSpotLights and never resized: MAIN already trims the snapshot to that cap,
        // so a fixed 64-element buffer (9 KB) removes the grow-and-rebind dance the instance
        // buffers need. Members so the scratch arrays keep their capacity between frames.
        DynamicArray<SpotLightGPU> mSpotLightScratch;
        DynamicArray<UInt32>       mSpotLightIndices;
        ShaderStorageBuffer<SpotLightGPU> mSpotLightBuffer;       // binding 5
        ShaderStorageBuffer<UInt32>       mSpotLightIndexBuffer;  // binding 6
        SpotLightDataGPU mSpotLightData;
        UniformBuffer<SpotLightDataGPU> mSpotLightDataBuffer;     // binding 4

        // Picks this frame's shadow slots and their light matrices. CPU only — no draws — because
        // the caster culling below needs the spot frusta before anything is rendered.
        void PrepareSpotShadowSlots(RenderSnapshot* snapshot);
        // Pass 1b: renders caster depth into the atlas slots. Runs after the cascade pass and
        // shares its visible-index SSBO (binding 3).
        void RenderSpotShadowPass(RenderSnapshot* snapshot);
        // Uploads UBO 4 + SSBO 5 + SSBO 6. Called UNCONDITIONALLY every frame (with
        // SpotLightCount = 0 when there are no lights), for the same reason as
        // UpdateShadowDataBuffer: no frame may read the previous frame's light list.
        void UpdateSpotLightBuffers(RenderSnapshot* snapshot);

        // Debug: one atlas slot copied out for display, same mechanism as mShadowLayerViewRequest.
        std::atomic<Int32> mSpotShadowViewRequest{-1};
        TOwningPointer<Texture> mSpotShadowView;
        void UpdateSpotShadowSlotView();

        // The two engine depth shaders, resolved once per frame (ResolveDepthShaders) and shared
        // by the cascade and spot passes. Previously each pass looked them up itself.
        TUsePointer<ShaderProgram> mDepthShader;
        TUsePointer<ShaderProgram> mSkeletalDepthShader;
        bool mSkeletalDepthReady = false;
        // Resolves both, requesting a lazy compile on the render thread when needed. Returns
        // false when the static depth shader is not ready — without it there is no depth pass at
        // all, so every shadow of the frame is skipped (skeletal readiness only skips skinned
        // casters, which is why it is a separate flag).
        bool ResolveDepthShaders();

        // Per-frame shadow parameters shared by every shader that declares the `ShadowData`
        // block (std140, binding 2). CPU-side mirror + the GPU buffer. Uploaded and bound
        // UNCONDITIONALLY every frame — with no directional light it simply carries
        // CascadeCount = 0. That is what makes the old "stale cascade count" class of bug
        // structurally impossible: there is no frame in which shaders read last frame's value.
        ShadowDataGPU mShadowData;
        UniformBuffer<ShadowDataGPU> mShadowDataBuffer;

        // Fills mShadowData from the cascades built this frame (or zeroes the count) and
        // uploads it. Call once per frame, before the main pass.
        void UpdateShadowDataBuffer(RenderSnapshot* snapshot);

        // VAO/VBO debugowej geometrii fizyki (linie + punkty). Tworzone eager w Initialize na
        // wątku renderu; uploadowane per-klatkę z płaskich buforów snapshotu (pos(3)+color(3)).
        unsigned int mDebugVao = 0;
        unsigned int mDebugVbo = 0;

        // Empty VAO for the editor grid fullscreen pass — the draw is attribute-less
        // (EditorGrid.vert builds the triangle from gl_VertexID), but core profile still
        // requires a VAO to be bound. Created eager in Initialize on the render thread.
        unsigned int mGridVao = 0;

        // Meshe snapshotu rozwiązane RAZ na klatkę (ResolveSnapshotMeshes na początku
        // RenderSnapshot); indeks = indeks batcha w StaticMeshBatches / obiektu w
        // SkeletalMeshRenderObjects. GetAssetDataNoLoad bierze shared_lock na mutexie
        // AssetManagera — bez tego cache'u pass cieni brał go per batch PER KASKADA (x5),
        // kontendując z main threadem o ten sam mutex. Membery reużywane między klatkami
        // (Clear zachowuje capacity). Wpisy mogą być nullem (asset jeszcze nie w mapie).
        DynamicArray<TUsePointer<StaticMesh>> mResolvedBatchMeshes;
        DynamicArray<TUsePointer<SkeletalMesh>> mResolvedSkeletalMeshes;
        void ResolveSnapshotMeshes(RenderSnapshot* snapshot);

        // Fills mCascades with the matrices this frame's cascades (and the main pass) need, and
        // rebuilds the GL resources when the settings changed. Leaves mCascades empty when there
        // is no directional light or the depth shader is not ready yet.
        //
        // Split from the drawing below because the caster culling is now shared with the spot
        // shadow slots: every shadow frustum of the frame — cascades AND spot slots — has to be
        // known before a single one of them is culled, so that the visible-index SSBO stays one
        // upload on one binding point.
        void PrepareShadowCascades(RenderSnapshot* snapshot, const Matrix4& cameraView);

        // Pass 1: renders caster depth into the cascade maps. Assumes PrepareShadowCascades and
        // CullShadowCasters already ran this frame.
        void RenderShadowPass(RenderSnapshot* snapshot);

        // Per-cascade caster culling. Runs on the RENDER thread because the cascade frusta only
        // exist here — culling on main would mean duplicating the whole cascade math there.
        //
        // Covers EVERY shadow frustum of the frame in one sweep: the cascades first, then the
        // spot shadow slots, so a range lives at [frustum * batchCount + batch] with
        // frustum < mCascades.Size() meaning "cascade" and the rest meaning "spot slot". One
        // upload, one binding, and the depth shaders stay untouched.
        //
        // For every (frustum, batch) pair it walks that batch's instance bounds and compacts the
        // survivors' indices into one concatenated array, uploaded as a single SSBO (binding 3).
        // The depth vertex shader then reads instances[visibleIndices[base + gl_InstanceID]], so
        // a culled batch costs 4 B per surviving instance instead of re-uploading 128 B of
        // instance data — and a batch that survives nowhere is skipped entirely (the draw-call win).
        struct ShadowDrawRange { UInt32 Offset = 0; UInt32 Count = 0; };
        DynamicArray<UInt32> mVisibleInstanceScratch;      // concatenated instance indices
        DynamicArray<ShadowDrawRange> mShadowDrawRanges;   // indexed [frustum * batchCount + batch]
        ShaderStorageBuffer<UInt32> mVisibleInstanceBuffer;
        // Per-cascade caster counts (static instances + skeletal meshes), for the stats panel.
        DynamicArray<UInt32> mCascadeCasterCounts;

        // Fills mVisibleInstanceScratch / mShadowDrawRanges for this frame's cascades and uploads
        // the buffer. Returns the number of instance-cascade pairs culled away (for stats).
        UInt32 CullShadowCasters(RenderSnapshot* snapshot);

        // Debug: one cascade layer copied out for display. The ImGui backend can only bind
        // GL_TEXTURE_2D, so an array layer is not directly displayable — the render thread
        // blits the requested layer into a plain 2D depth texture with glCopyImageSubData.
        // The request is an atomic written by the UI thread (-1 = nobody is looking).
        std::atomic<Int32> mShadowLayerViewRequest{-1};
        TOwningPointer<Texture> mShadowLayerView;
        void UpdateShadowLayerView();

        // Rysuje debugową geometrię fizyki ze snapshotu (linie + punkty) shaderem DebugLine.
        void RenderDebugGeometry(RenderSnapshot* snapshot, const Matrix4& viewProj);

        // Draws the infinite procedural editor grid (EditorGrid shader, Y=0 plane) as a
        // fullscreen pass blended over the scene. Depth test stays on (the shader writes the
        // plane's real depth), depth writes stay off — the grid never occludes anything.
        void RenderEditorGrid(RenderSnapshot* snapshot, const Matrix4& view);

        //Skeletal sTUFF
        ShaderStorageBuffer<Matrix4> mSkeletalMatricesBuffer;

        // Palety skinningu (skin = global * offset) WSZYSTKICH skeletal meshy klatki, liczone
        // raz w RenderSnapshot (BuildSkeletalPalettes) — dawniej pass cieni przeliczał je per
        // kaskada × obiekt (z alokacją), a główny pass jeszcze raz. Płaski scratch + zakresy
        // per obiekt (indeks = indeks w snapshot->SkeletalMeshRenderObjects); tablice są
        // memberami reużywanymi między klatkami (Clear zachowuje capacity — zero alokacji
        // w ustalonym stanie).
        struct SkeletalPaletteRange { UInt32 Offset = 0; UInt32 Count = 0; };
        DynamicArray<Matrix4> mSkeletalPaletteScratch;
        DynamicArray<SkeletalPaletteRange> mSkeletalPaletteRanges;

        // Liczy palety do scratcha (patrz wyżej). Wołane raz na klatkę, przed RenderShadowPass.
        void BuildSkeletalPalettes(RenderSnapshot* snapshot);

        // Uploads EVERY palette of the frame to mSkeletalMatricesBuffer (binding 0) in one go and
        // binds it for the whole frame; draws then just set `paletteBaseIndex` to their range's
        // offset. Previously each object rewrote the whole shared buffer, once per cascade plus
        // once for the main pass — the same data uploaded N x (cascades + 1) times per frame.
        void UploadSkeletalPalettes();

        // Dane instancji static meshy (SSBO binding 1 — binding 0 to BoneMatrices skeletal, patrz
        // HELPERS.md). Bufor bindowany w CAŁOŚCI (BindBase) na całą klatkę w RenderSnapshot();
        // batche adresują swój zakres uniformem "instanceBaseIndex", nie BindRange per batch
        // (offsety BindRange muszą być wielokrotnością GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT,
        // które u części sterowników wynosi 256 — sizeof(InstanceGPUData)==128 by tego nie
        // gwarantowało). Skeletal używa osobnego indeksowanego punktu 0 (BoneMatrices), więc
        // bufor instancji przeżywa całą klatkę bez potrzeby rebindu.
        ShaderStorageBuffer<InstanceGPUData> mInstanceBuffer;

        // Programy shaderowe, dla których już ostrzegliśmy, że skeletal mesh jest rysowany bez
        // bloku SSBO "BoneMatrices" (mesh zamarza w bind pose) — warning raz, nie per klatkę.
        HashSet<UInt64> mWarnedNonSkeletalPrograms;

        // Programy shaderowe, dla których już ostrzegliśmy, że batch instancji (VisibleCount > 1)
        // renderuje się fallbackiem per-obiekt, bo shader nie ma bloku SSBO "InstanceMatrices" —
        // warning raz, nie per klatkę (analogicznie do mWarnedNonSkeletalPrograms).
        HashSet<UInt64> mWarnedNonInstancedPrograms;
    public:
        Renderer() = default;
        ~Renderer() = default;

        TUsePointer<FrameBuffer> GetMainFrameBuffer();

        // Debug cascade viewer (see mShadowLayerViewRequest). Call RequestShadowCascadeView every
        // frame the viewer is open; the copy lands one frame later and survives a resolution
        // change, because the destination texture is rebuilt alongside the array.
        void RequestShadowCascadeView(Int32 layer);
        TUsePointer<Texture> GetShadowCascadeView();
        [[nodiscard]] Int32 GetShadowCascadeLayerCount() const { return mShadowLayerCount; }

        // Same viewer, for a slot of the spot shadow atlas.
        void RequestSpotShadowView(Int32 slot);
        TUsePointer<Texture> GetSpotShadowView();
        [[nodiscard]] Int32 GetSpotShadowSlotCount() const { return mSpotShadowSlotCount; }

        void Initialize(ApplicationInfo* applicationInfo);
        void RenderSnapshot(RenderSnapshot* snapshot);
        void Shutdown();
    };
}

#endif //PLUENGINE_RENDERER_H
