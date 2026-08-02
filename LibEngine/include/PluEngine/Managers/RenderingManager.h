//
// Created by Plutex on 2026-02-08.
//

#ifndef PLUENGINE_RENDERINGMANAGER_H
#define PLUENGINE_RENDERINGMANAGER_H

#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "RenderingManager.generated.h"
#include "PluEngine/PluUUID.h"
#include "PluEngine/Renderer/ImGuiRenderState.h"
#include "PluEngine/Renderer/OpenGLRenderState.h"
#include "PluEngine/Threading/TripleBuffer.h"

#include <mutex>
#include <condition_variable>

struct ImDrawData;

#define MAX_TEXTURES_LOAD_PER_FRAME 1

namespace Plu
{
	class FrameBuffer;
	struct RenderSnapshot;
	struct ImGuiDrawSnapshot;
	struct StaticMesh;
	struct SkeletalMesh;
	class Texture;
	struct TextureInfo;
	//One of the few managers that isn't Editor or Runtime.
	//This manager keeps track of all textures, shaders and loaded meshes
	//Mostly textures are cared about right now
	PLU_CLASS(Abstract)
	class PLU_API RenderingManager final : public EngineObject
	{
		REFLECTION_BODY_RENDERINGMANAGER()
	private:
		ApplicationInfo* mApplicationInfo;

		GameHashMap<UInt64, TOwningPointer<Texture>> mTextures;
		GameHashMap<UInt64, int> mTextureFramesWithNoUse;
		GameHashMap<UInt64, int> mTextureUsePerFrame;

		// The texture maps above (and the GL texture objects they own) belong to the render thread:
		// they are created/uploaded and destroyed with the GL context current, and the per-frame
		// use/eviction bookkeeping runs in Tick() on the render thread. Off-thread callers (e.g. the
		// editor texture-preview panel, which queries textures from the Main thread) must NOT touch
		// GL or these maps directly. mTextureMutex guards every access to the texture maps and the
		// pending queue; a Main-thread RequestTextureFromInfo() only enqueues a TextureInfo, and the
		// render thread drains the queue + does the GL load in Tick(). See RequestTextureFromInfo.
		std::mutex mTextureMutex;
		DynamicArray<TUsePointer<TextureInfo>> mPendingTextureRequests;

		// A read-back-to-disk request. The GL readback (glGetTexImage) needs the context current, so
		// like the load queue this is filled on any thread and drained by the render thread in Tick().
		struct PendingTextureSave
		{
			TUsePointer<Texture> TargetTexture;
			Path SavePath;
		};
		DynamicArray<PendingTextureSave> mPendingTextureSaves;

		// Render-thread only. Performs the actual GL upload + map insert. Caller must hold mTextureMutex.
		void LoadTextureFromInfo_NoLock(const TUsePointer<TextureInfo>& textureInfo);
		// Render-thread only. GL delete + map removal. Caller must hold mTextureMutex.
		void UnloadTextureForUUID_NoLock(UInt64 uuid);
		// Render-thread only. Drains mPendingTextureRequests, loading each texture. Caller must hold mTextureMutex.
		void ProcessPendingTextureRequests_NoLock();
		// Render-thread only. Drains mPendingTextureSaves, writing each texture to disk. Caller must hold mTextureMutex.
		void ProcessPendingTextureSaves_NoLock();

		GameHashMap<UInt64, TUsePointer<StaticMesh>> mStaticMeshes;
		GameHashMap<UInt64, int> mStaticMeshUsePerFrame;
		GameHashMap<UInt64, int> mStaticMeshFramesWithNoUse;

		GameHashMap<UInt64, TUsePointer<SkeletalMesh>> mSkeletalMeshes;
		GameHashMap<UInt64, int> mSkeletalMeshUsePerFrame;
		GameHashMap<UInt64, int> mSkeletalMeshFramesWithNoUse;

		TOwningPointer<std::thread> mRenderThread;
		std::atomic<bool> mIsRendererRunning = false;
		std::atomic<bool> mSkipImGuiRendering = false;

		// Stan kontekstów renderowania, którym zarządza ten manager:
		// - mImGuiState: kontekst ImGui + styl (Main, InitializeImGuiContext) i backend OpenGL3
		//   (render thread, RenderThreadEnter/Exit).
		// - mGLState: domyślny stan GL (depth test, blending) — per-kontekst, więc odpalany na
		//   render threadzie zaraz po MakeGLContextCurrent().
		ImGuiRenderState mImGuiState;
		OpenGLRenderState mGLState;

		// Main->Render handoff of one ImGui frame (deep-copied draw data). Writer is the
		// Main thread via SubmitImGuiDrawData(), reader is the render thread loop. Same
		// lock-free triple-buffer pattern as the RenderSnapshot path.
		TripleBuffer<ImGuiDrawSnapshot*> mImguiTripleBuffer;

		// ImGui font-atlas rebuild lockstep. The lock-free ImGui handoff only deep-copies draw
		// lists, not texture state - the snapshot just references the live ImTextureData/ImFontAtlas
		// owned by the Main thread. As long as the atlas is static every texture sits at
		// ImTextureStatus_OK and neither thread mutates it, so the render thread can read it freely.
		// A font-size change rebuilds the atlas (creates a new texture, destroys the old one), and
		// then both threads touch the same texture objects at once -> ImGui asserts in the
		// create/destroy paths (imgui_draw.cpp). While such a rebuild is in flight the Main thread
		// drives the handoff in lockstep via Begin/Step/EndImGuiLockstep() so that only one thread
		// ever touches the atlas at a time. Steady state cost is a single atomic load per render loop.
		std::atomic<bool> mImGuiLockstep = false;
		std::mutex mImGuiLockstepMtx;
		std::condition_variable mImGuiLockstepCv;
		bool mImGuiPassGranted = false;   // Main -> Render: you may run exactly one ImGui pass.
		bool mImGuiPassDone = false;      // Render -> Main: that pass is complete.
		bool mImGuiRenderParked = false;  // Render -> Main: parked, not touching the atlas.

		void RenderThreadEnter();
		void RenderThreadLoop();
		void RenderThreadExit();

		friend PLU_API void DrawStaticMesh(const StaticMesh* staticMesh, RenderingManager* renderingManager);
		friend PLU_API void DrawStaticMeshInstanced(const StaticMesh* staticMesh, RenderingManager* renderingManager, UInt32 instanceCount);
		void OnStaticMeshRender(StaticMesh *staticMesh);

		friend PLU_API void DrawSkeletalMesh(const SkeletalMesh* skeletalMesh, RenderingManager* renderingManager);
		void OnSkeletalMeshRender(SkeletalMesh *skeletalMesh);

		//FUN
		std::atomic<bool> mLimitTextureLoadPerFrame = false;
	public:
		RenderingManager(ApplicationInfo* applicationInfo);
		virtual ~RenderingManager() override;

		//Requesting Resources
		void RequestTextureFromInfo(const TUsePointer<TextureInfo>& textureInfo);
		TUsePointer<Texture> GetTextureForInfo(const TUsePointer<TextureInfo>& textureInfo);
		void UnloadTextureForUUID(UInt64 uuid);

		// Enqueue a texture read-back to a PNG on disk. Safe to call from any thread: the actual
		// glGetTexImage + write happens on the render thread (with the GL context current) in Tick().
		void RequestTextureSave(const TUsePointer<Texture>& texture, const Path& path);

		void RequestStaticMeshLoad(PluUUID uuid);
		void UnloadStaticMesh(PluUUID uuid);

		void RequestSkeletalMeshLoad(PluUUID uuid);
		void UnloadSkeletalMesh(PluUUID uuid);

		TUsePointer<FrameBuffer> RequestMainFrameBuffer();

		// Debug view of one directional shadow cascade layer (RenderGpuStatsPanel). The array
		// texture itself is not displayable by the ImGui backend (it only binds GL_TEXTURE_2D), so
		// the render thread copies the requested layer into a plain 2D depth texture. Call Request
		// every frame the viewer is open (-1 to stop); Get returns last frame's copy, or null.
		void RequestShadowCascadeView(Int32 layer);
		TUsePointer<Texture> GetShadowCascadeView();
		[[nodiscard]] Int32 GetShadowCascadeLayerCount() const;

		// Called from the Main thread (the app that builds the UI) after ImGui::Render().
		// Deep-copies the live draw data into the triple buffer and publishes it for the
		// render thread. The engine does not drive ImGui::NewFrame()/Render() itself - the
		// app owns its ImGui frame and simply hands the result over through this API.
		void SubmitImGuiDrawData(ImDrawData* drawData);

		void SetImGuiRenderingIgnorance(bool ignore);
		bool IsImGuiRenderingIgnored() const;

		// ImGui font-atlas rebuild lockstep (call from the Main thread, around the ImGui frame that
		// rebuilds the atlas - e.g. a font-size change). Begin parks the render thread's ImGui
		// section so Main can mutate the atlas exclusively; Step grants exactly one render-thread
		// ImGui pass (texture upload + draw) and blocks until it finishes; End resumes the normal
		// lock-free free-running handoff. Stay in lockstep (Begin/build/Step each frame) until the
		// atlas has settled back to all-OK, then call End. No-ops are safe if the render thread is
		// not currently rendering ImGui. See mImGuiLockstep.
		void BeginImGuiLockstep();
		void StepImGuiLockstep();
		void EndImGuiLockstep();

		// TripleBuffer telemetry (thread-safe, callable from any thread). Cumulative since start
		// (or last reset). "Dropped" = published frames the consumer never picked up (producer
		// outran consumer). "Reused" = the consumer re-used the previous frame because nothing
		// new was published (consumer outran producer). Scene = Main->Render RenderSnapshot path;
		// ImGui = Main->Render draw-data path.
		[[nodiscard]] UInt32 GetSnapshotDroppedCount() const;
		[[nodiscard]] UInt32 GetSnapshotReusedCount() const;
		[[nodiscard]] UInt32 GetImGuiDroppedCount() const;
		[[nodiscard]] UInt32 GetImGuiReusedCount() const;
		void ResetTripleBufferTelemetry();

		// Main thread, po IWindow::Init(): tworzy kontekst ImGui (IO/DPI/styl + backend
		// platformowy) i wpina go w okno. Backend OpenGL3 dochodzi później na render
		// threadzie — patrz RenderThreadEnter().
		void InitializeImGuiContext();

		void Initialize(TripleBuffer<RenderSnapshot*>* tripleBuffer);
		// runUseBookkeeping = false, gdy ta klatka NIE renderuje sceny (stale snapshot — patrz
		// RenderThreadLoop): liczniki użyć assetów nie dostają wtedy bumpów z draw calli, więc
		// liczenie im "bezczynnych ticków" eksmitowałoby assety podczas dłuższego stalla maina
		// (breakpoint, modalny dialog). Kolejki pending (tekstury/zapisy) są drenowane zawsze.
		void Tick(bool runUseBookkeeping = true);
		void Shutdown();

		//FUN
		bool IsLimitTextureLoadPerFrame() const;
		void SetLimitTextureLoadPerFrame(bool limit);
	};
}

#endif //PLUENGINE_RENDERINGMANAGER_H
