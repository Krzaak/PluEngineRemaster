//
// Created by Plutex on 2026-02-08.
//

#include "PluEngine/Managers/RenderingManager.h"

#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"
#include "PluEngine/Application.h"
#include "PluEngine/Engine.h"
#include "PluEngine/Assets/AssetDescriptor.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/AssetTypes/Texture/Texture.h"
#include "PluEngine/Managers/ShadersManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/Renderer/GLTexture.h"
#include "PluEngine/Renderer/ImGuiDrawSnapshot.h"
#include "PluEngine/Renderer/Renderer.h"
#include "PluEngine/Renderer/RenderThreading.h"
#include "PluEngine/Threading/ThreadAffinity.h"
#include "PluEngine/Window/Window.h"

static Plu::TripleBuffer<Plu::RenderSnapshot *>* gTripleBuffer = nullptr;
static Plu::Renderer* gRenderer = nullptr;
static std::atomic<bool> gIsRendererGut(false);

void Plu::RenderingManager::RenderThreadEnter()
{
	PLU_CORE_TRACE("Render Thread Started");
	if (!mApplicationInfo) return;
	mApplicationInfo->AppWindow->MakeGLContextCurrent();
	gRenderer = new Renderer();
	gRenderer->Initialize(mApplicationInfo);

	// The OpenGL ImGui backend lives on the render thread: ImGui_ImplOpenGL3_Init queries
	// glGetString(GL_VERSION) and every RenderDrawData call issues GL commands, so it must
	// run where the GL context is current. The SDL2 (platform/input) backend stays on Main.
	// ImGui's GImGui is a single global; both threads only ever set it to this one context,
	// so it stays effectively constant - the render thread touches the renderer backend data,
	// Main touches the platform backend data.
	if (ImGuiContext* ctx = mApplicationInfo->AppWindow->GetImGuiContext()) {
		ImGui::SetCurrentContext(ctx);
		ImGui_ImplOpenGL3_Init("#version 450");
	}

	gIsRendererGut = true;
	while (mIsRendererRunning) {
		RenderThreadLoop();
	}
	gIsRendererGut = false;
	RenderThreadExit();
}

void Plu::RenderingManager::RenderThreadLoop()
{
	// Render-thread frame delta. Only this thread calls RenderThreadLoop, so a function-local
	// static is race-free. Published for diagnostics (editor panels) via PluUtils.
	static std::chrono::high_resolution_clock::time_point lastRenderFrame = std::chrono::high_resolution_clock::now();
	const std::chrono::high_resolution_clock::time_point nowRenderFrame = std::chrono::high_resolution_clock::now();
	SetRenderThreadDeltaTime(std::chrono::duration<float>(nowRenderFrame - lastRenderFrame).count());
	lastRenderFrame = nowRenderFrame;

	// static double period = 0.000000003f;
	// double sineWave = (std::sin(period * std::chrono::high_resolution_clock::now().time_since_epoch().count()) + 1) / 2.0f;
	// glClearColor(sineWave, sineWave, sineWave, 1.0f);
	// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mApplicationInfo->AppRenderingManager->Tick();
	RenderSnapshot* snapshot = gTripleBuffer->AcquireReadBuffer();
	if (snapshot) {
		gRenderer->RenderSnapshot(snapshot);
	}

	bool wasImGuiRendered = false;

	// ImGui overlay on top of the scene. The draw data was deep-copied on the Main thread
	// and handed over through the triple buffer; here we just upload any pending textures
	// (handled inside RenderDrawData via DrawData.Textures) and submit.
	if (mApplicationInfo->AppWindow->GetImGuiContext() && !IsImGuiRenderingIgnored()) {
		// Lockstep gate: while the Main thread is rebuilding the font atlas it parks this ImGui
		// section so it can mutate the shared ImTextureData/ImFontAtlas exclusively. We announce
		// we're parked, then wait for Main to grant exactly one pass (or to leave lockstep). Outside
		// a rebuild this is a single atomic load and we render as usual. See mImGuiLockstep.
		bool consumedGrant = false;
		bool skipImGui = false;
		if (mImGuiLockstep.load(std::memory_order_acquire)) {
			std::unique_lock<std::mutex> lock(mImGuiLockstepMtx);
			mImGuiRenderParked = true;
			mImGuiLockstepCv.notify_all();
			mImGuiLockstepCv.wait(lock, [this] {
				return mImGuiPassGranted || !mImGuiLockstep.load(std::memory_order_acquire) || !mIsRendererRunning;
			});
			mImGuiRenderParked = false;
			if (!mIsRendererRunning) {
				skipImGui = true;
			} else if (mImGuiPassGranted) {
				mImGuiPassGranted = false;
				consumedGrant = true;
			}
			// else: Main left lockstep without a pending grant -> fall through and render normally.
		}

		if (!skipImGui) {
			ImGui_ImplOpenGL3_NewFrame(); // lazily (re)creates the backend's GL device objects
			ImGuiDrawSnapshot* guiSnapshot = mImguiTripleBuffer.AcquireReadBuffer();
			if (guiSnapshot && guiSnapshot->DrawData.Valid) {
				ImGui_ImplOpenGL3_RenderDrawData(&guiSnapshot->DrawData);
				wasImGuiRendered = true;
			}
		}

		if (consumedGrant) {
			std::lock_guard<std::mutex> lock(mImGuiLockstepMtx);
			mImGuiPassDone = true;
			mImGuiLockstepCv.notify_all();
		}
	}

	TUsePointer<IWindow> window = mApplicationInfo->AppWindow;
	if (!wasImGuiRendered) {
		gRenderer->GetMainFrameBuffer()->BlitToScreen(window->GetWidth(), window->GetHeight());
	}

	window->SwapBuffer();

	int windowWidth = window->GetWidth();
	int windowHeight = window->GetHeight();
	int bufferWidth = gRenderer->GetMainFrameBuffer()->GetWidth();
	int bufferHeight = gRenderer->GetMainFrameBuffer()->GetHeight();
	if (windowWidth != bufferWidth || windowHeight != bufferHeight) {
		gRenderer->GetMainFrameBuffer()->Resize(windowWidth, windowHeight);
	}
}

void Plu::RenderingManager::RenderThreadExit()
{
	// Tear down the GL ImGui backend on the render thread while its context is still current.
	if (mApplicationInfo->AppWindow->GetImGuiContext()) {
		ImGui_ImplOpenGL3_Shutdown();
	}
	gRenderer->Shutdown();
	delete gRenderer;

	DynamicArray<UInt64> textureIds;
	for (const auto& texture : mTextures) {
		textureIds.PushBack(texture.first);
	}
	for (const auto& txt : textureIds) {
		UnloadTextureForUUID(txt);
	}

	// Release render-owned shader programs on this (the render) thread, while the GL context is
	// still current — see IShaderManager::ReleaseRenderResources. Must run before ReleaseGLContext.
	if (mApplicationInfo->AppShaderManager) mApplicationInfo->AppShaderManager->ReleaseRenderResources();
	mApplicationInfo->AppWindow->ReleaseGLContext();
	PLU_CORE_TRACE("Render Thread Exit");
}

void Plu::RenderingManager::OnStaticMeshRender(StaticMesh *staticMesh)
{
	if (mStaticMeshUsePerFrame.Contains(staticMesh->Uuid)) {
		mStaticMeshUsePerFrame[staticMesh->Uuid]++;
	} else {
		mStaticMeshUsePerFrame.Insert(staticMesh->Uuid, 1);
	}
}

Plu::RenderingManager::RenderingManager(ApplicationInfo *applicationInfo)
{
	mApplicationInfo = applicationInfo;
	PLU_CORE_TRACE("Rendering Manager Init");
}

Plu::RenderingManager::~RenderingManager()
{
	PLU_CORE_TRACE("Rendering Manager Destroy");
}

void Plu::RenderingManager::RequestTextureFromInfo(const TUsePointer<TextureInfo>& textureInfo)
{
	if (!textureInfo) return;

	// GL creation and the texture maps belong to the render thread. A request coming from any other
	// thread (the editor texture-preview panel queries from Main) must not issue GL or mutate the
	// maps here — instead enqueue it and let the render thread do the load in Tick(). The render
	// thread's own callers (e.g. ShaderProgram during RenderSnapshot) load immediately.
	if (!IsOnMainThread()) {
		std::lock_guard<std::mutex> lock(mTextureMutex);
		LoadTextureFromInfo_NoLock(textureInfo);
		return;
	}

	std::lock_guard<std::mutex> lock(mTextureMutex);
	// Already loaded, or already queued — nothing to do. The panel re-requests every frame until the
	// texture appears, so dedupe keeps the pending queue from growing while the render thread catches up.
	if (mTextures.Contains(textureInfo->Uuid)) return;
	for (const TUsePointer<TextureInfo>& pending : mPendingTextureRequests) {
		if (pending && pending->Uuid == textureInfo->Uuid) return;
	}
	mPendingTextureRequests.PushBack(textureInfo);
}

void Plu::RenderingManager::LoadTextureFromInfo_NoLock(const TUsePointer<TextureInfo>& textureInfo)
{
	if (!textureInfo) return;
	if (mTextures.Contains(textureInfo->Uuid)) return;
	TUsePointer<Texture> texture = mApplicationInfo->AppObjectManager->CreateObject(Texture::GetStaticClass());
	texture->CreateFromInfo(textureInfo.GetRaw(), false);
	mTextures.Insert(textureInfo->Uuid, mApplicationInfo->AppObjectManager->GetObjectAsOwner<Texture>(*texture->GetEngineObjectHandle()));
	mTextureUsePerFrame[textureInfo->Uuid] = 0;
	PLU_CORE_INFO("Texture {} Loaded!", textureInfo->Uuid.getUUID());
}

void Plu::RenderingManager::ProcessPendingTextureRequests_NoLock()
{
	if (mPendingTextureRequests.IsEmpty()) return;
	for (const TUsePointer<TextureInfo>& pending : mPendingTextureRequests) {
		LoadTextureFromInfo_NoLock(pending);
	}
	mPendingTextureRequests.Clear();
}

void Plu::RenderingManager::RequestTextureSave(const TUsePointer<Texture>& texture, const Path& path)
{
	if (!texture) return;
	// GL readback belongs to the render thread. A render-thread caller can save immediately; anyone
	// else (e.g. the editor texture-preview panel on Main) enqueues and lets Tick() do it with the
	// GL context current. Same split as RequestTextureFromInfo.
	if (!IsOnMainThread()) {
		texture->SaveTexture(path);
		return;
	}
	std::lock_guard<std::mutex> lock(mTextureMutex);
	mPendingTextureSaves.PushBack({texture, path});
}

void Plu::RenderingManager::ProcessPendingTextureSaves_NoLock()
{
	if (mPendingTextureSaves.IsEmpty()) return;
	for (const PendingTextureSave& pending : mPendingTextureSaves) {
		if (pending.TargetTexture) {
			pending.TargetTexture->SaveTexture(pending.SavePath);
		}
	}
	mPendingTextureSaves.Clear();
}

Plu::TUsePointer<Plu::Texture> Plu::RenderingManager::GetTextureForInfo(const TUsePointer<TextureInfo>& textureInfo)
{
	if (!textureInfo) return nullptr;
	std::lock_guard<std::mutex> lock(mTextureMutex);
	if (mTextures.Contains(textureInfo->Uuid)) {
		mTextureUsePerFrame[textureInfo->Uuid]++;
		return mTextures[textureInfo->Uuid];
	}
	return nullptr;
}

void Plu::RenderingManager::UnloadTextureForUUID(UInt64 uuid)
{
	std::lock_guard<std::mutex> lock(mTextureMutex);
	UnloadTextureForUUID_NoLock(uuid);
}

void Plu::RenderingManager::UnloadTextureForUUID_NoLock(UInt64 uuid)
{
	if (!mTextures.Contains(uuid)) return;
	TOwningPointer<Texture> texture = mTextures[uuid];
	texture->Destroy();
	mApplicationInfo->AppObjectManager->DestroyObject(*texture->GetEngineObjectHandle());
	mTextures.Remove(uuid);
	mTextureUsePerFrame.Remove(uuid);
	mTextureFramesWithNoUse.Remove(uuid);
	PLU_CORE_INFO("Unloaded {}", uuid);
}

void Plu::RenderingManager::RequestStaticMeshLoad(PluUUID uuid)
{
	TUsePointer<EngineAssetManager> assetManager = mApplicationInfo->AppAssetManager;
	if (!assetManager) return;
	if (!assetManager->AssetExists(uuid)) return;
	TUsePointer<AssetDescriptor> assetDesc = assetManager->GetAssetDescriptor(uuid);
	if (!assetDesc->AssetType->IsDerivedOfOrSame(StaticMesh::GetStaticClass())) return;
	// Render thread: read cache only. If the CPU data isn't loaded yet, post a deferred load
	// request (drained on the main thread) and bail — we'll do the GL setup once it's available.
	TUsePointer<StaticMesh> staticMesh = assetManager->GetAssetDataNoLoad(uuid);
	if (!staticMesh) {
		assetManager->RequestAssetDataLoad(uuid);
		return;
	}
	if (staticMesh->IsLoaded) return;
	SetupStaticMeshGL(&staticMesh->StaticMeshData, staticMesh.GetRaw());
	mStaticMeshes.Insert(staticMesh->Uuid, staticMesh);
	PLU_CORE_INFO("Static Mesh {} Loaded!", staticMesh->Uuid.getUUID());
}

void Plu::RenderingManager::UnloadStaticMesh(PluUUID uuid)
{
	if (!mStaticMeshes.Contains(uuid)) return;
	CleanupStaticMeshGL(mStaticMeshes[uuid].GetRaw());
	mStaticMeshes.Remove(uuid);
	mStaticMeshFramesWithNoUse[uuid] = 0;
	PLU_CORE_INFO("Static Mesh {} Unloaded", uuid.getUUID());
}

Plu::TUsePointer<Plu::FrameBuffer> Plu::RenderingManager::RequestMainFrameBuffer()
{
	if (gIsRendererGut) {
		return gRenderer->GetMainFrameBuffer();
	}
	return nullptr;
}

void Plu::RenderingManager::SubmitImGuiDrawData(ImDrawData *drawData)
{
	if (!drawData) return;

	// Writer side of the triple buffer (Main thread). Reuse the slot's snapshot object
	// (its previous clones are freed in CopyFrom -> Clear), mirroring RenderSnapshotBuilder.
	ImGuiDrawSnapshot*& slot = mImguiTripleBuffer.GetWriteBuffer();
	if (slot == nullptr) {
		slot = new ImGuiDrawSnapshot();
	}
	slot->CopyFrom(drawData);
	mImguiTripleBuffer.Publish();
}

void Plu::RenderingManager::SetImGuiRenderingIgnorance(bool ignore)
{
	mSkipImGuiRendering.store(ignore);
}

bool Plu::RenderingManager::IsImGuiRenderingIgnored() const
{
	return mSkipImGuiRendering.load();
}

void Plu::RenderingManager::BeginImGuiLockstep()
{
	std::unique_lock<std::mutex> lock(mImGuiLockstepMtx);
	if (!mIsRendererRunning) return;
	mImGuiPassGranted = false;
	mImGuiPassDone = false;
	mImGuiLockstep.store(true, std::memory_order_release);
	// Wait until the render thread reaches its park point (finished any in-flight ImGui pass and is
	// no longer touching the atlas) before Main starts mutating it. The timeout is only a deadlock
	// safety net for the case where the render thread isn't rendering ImGui at all (no context /
	// rendering ignored) - then it never parks, but it also never touches the atlas, so proceeding
	// is safe. In the normal editor case it parks within one frame.
	mImGuiLockstepCv.wait_for(lock, std::chrono::milliseconds(250),
		[this] { return mImGuiRenderParked || !mIsRendererRunning; });
}

void Plu::RenderingManager::StepImGuiLockstep()
{
	std::unique_lock<std::mutex> lock(mImGuiLockstepMtx);
	if (!mIsRendererRunning || !mImGuiLockstep.load(std::memory_order_acquire)) return;
	mImGuiPassDone = false;
	mImGuiPassGranted = true;
	mImGuiLockstepCv.notify_all();
	// Block until the render thread completed exactly one ImGui pass (textures uploaded, draw data
	// submitted). Main must not mutate the atlas again until this returns. Timeout: deadlock safety
	// net (see BeginImGuiLockstep); harmless to proceed since a non-rendering render thread isn't
	// touching the atlas.
	mImGuiLockstepCv.wait_for(lock, std::chrono::milliseconds(250),
		[this] { return mImGuiPassDone || !mIsRendererRunning; });
}

void Plu::RenderingManager::EndImGuiLockstep()
{
	std::lock_guard<std::mutex> lock(mImGuiLockstepMtx);
	mImGuiLockstep.store(false, std::memory_order_release);
	mImGuiPassGranted = false;
	mImGuiLockstepCv.notify_all();
}

UInt32 Plu::RenderingManager::GetSnapshotDroppedCount() const
{
	return gTripleBuffer ? gTripleBuffer->GetDroppedSnapshotCount() : 0;
}

UInt32 Plu::RenderingManager::GetSnapshotReusedCount() const
{
	return gTripleBuffer ? gTripleBuffer->GetStaleFrameCount() : 0;
}

UInt32 Plu::RenderingManager::GetImGuiDroppedCount() const
{
	return mImguiTripleBuffer.GetDroppedSnapshotCount();
}

UInt32 Plu::RenderingManager::GetImGuiReusedCount() const
{
	return mImguiTripleBuffer.GetStaleFrameCount();
}

void Plu::RenderingManager::ResetTripleBufferTelemetry()
{
	if (gTripleBuffer) {
		gTripleBuffer->ResetTelemetry();
	}
	mImguiTripleBuffer.ResetTelemetry();
}

void Plu::RenderingManager::Initialize(TripleBuffer<RenderSnapshot *> *tripleBuffer)
{
	mIsRendererRunning = true;
	gTripleBuffer = tripleBuffer;
	// Keep the thread joinable: shutdown synchronizes by join()ing it, which guarantees
	// RenderThreadExit() has fully torn down GL and released the context before Main touches
	// the window or destroys the SDL/GL context. Do NOT detach.
	mRenderThread = CreateOwning<std::thread>(&RenderingManager::RenderThreadEnter, this);
}

void Plu::RenderingManager::Tick()
{
	// Render thread. One lock spans the whole texture section: drain the Main->Render request queue
	// (GL load), run the per-frame use/eviction bookkeeping, and evict — so off-thread Get/Request
	// calls never observe a half-mutated map. Unload via the _NoLock variant; the public one re-locks.
	{
		std::lock_guard<std::mutex> lock(mTextureMutex);
		ProcessPendingTextureRequests_NoLock();
		ProcessPendingTextureSaves_NoLock();

		for (const auto& textureId : mTextureUsePerFrame) {
			int uses = mTextureUsePerFrame[textureId.first];
			if (uses == 0) {
				if (mTextureFramesWithNoUse.Contains(textureId.first)) {
					mTextureFramesWithNoUse[textureId.first]++;
				} else {
					mTextureFramesWithNoUse.Insert(textureId.first, 0);
				}
			} else {
				// Used this tick — reset the idle counter so it measures *consecutive* idle render
				// ticks, not total. This loop runs once per render tick (faster than a Main-thread
				// consumer like the texture-preview panel bumps the use count), so without the reset a
				// panel texture accumulates idle ticks between bumps and gets evicted within a fraction
				// of a second, then reloaded — a churn that recycles the GL texture id (ImGui's font
				// atlas grabs it, so the preview flickers to the font cache) and re-runs glTexImage2D
				// every frame. Render-thread consumers (ShaderProgram) bump every tick and are unaffected.
				mTextureFramesWithNoUse[textureId.first] = 0;
			}
		}
		for (const auto& textureId : mTextureUsePerFrame) {
			mTextureUsePerFrame[textureId.first] = 0;
		}
		for (std::pair<unsigned long, int> textureIDp: mTextureFramesWithNoUse) {
			if (textureIDp.second > 100) {
				UnloadTextureForUUID_NoLock(textureIDp.first);
				mTextureFramesWithNoUse[textureIDp.first] = 0;
				break;
			}
		}
	}

	//Meshes
	for (const auto& mesh : mStaticMeshUsePerFrame) {
		int uses = mStaticMeshUsePerFrame[mesh.first];
		if (uses == 0) {
			if (mStaticMeshFramesWithNoUse.Contains(mesh.first)) {
				mStaticMeshFramesWithNoUse[mesh.first]++;
			} else {
				mStaticMeshFramesWithNoUse.Insert(mesh.first, 0);
			}
		} else {
			// Reset to count *consecutive* idle ticks (see the texture loop above). Meshes are
			// consumed on the render thread so they bump every tick and never hit this in practice,
			// but keep the semantics identical to avoid a churn footgun if that ever changes.
			mStaticMeshFramesWithNoUse[mesh.first] = 0;
		}
	}
	for (const auto& mesh : mStaticMeshUsePerFrame) {
		mStaticMeshUsePerFrame[mesh.first] = 0;
	}
	for (std::pair<unsigned long, int> mesh: mStaticMeshFramesWithNoUse) {
		if (mesh.second > 100) {
			UnloadStaticMesh(mesh.first);
			mStaticMeshFramesWithNoUse[mesh.first] = 0;
			break;
		}
	}
}

void Plu::RenderingManager::Shutdown()
{
	PLU_CORE_TRACE("Rendering Manager Shutdown");
	mIsRendererRunning = false;
	// Wake the render thread if it's parked in the ImGui lockstep wait, so it can observe the stop
	// flag and exit instead of hanging the join below.
	{
		std::lock_guard<std::mutex> lock(mImGuiLockstepMtx);
		mImGuiLockstepCv.notify_all();
	}
	// Block until the render thread has run RenderThreadExit() to completion (GL torn down,
	// shader resources released, ReleaseGLContext()). After join the context is free, so it is
	// safe for Main to make it current and for Run() to delete the SDL/GL context afterwards.
	if (mRenderThread && mRenderThread->joinable()) mRenderThread->join();
	mApplicationInfo->AppWindow->MakeGLContextCurrent();
}
