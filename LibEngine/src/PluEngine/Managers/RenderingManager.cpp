//
// Created by Plutex on 2026-02-08.
//

#include "PluEngine/Managers/RenderingManager.h"

#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
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
#include "PluEngine/Window/Window.h"
#include "PluEngine/Window/WindowManager.h"

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
	if (mApplicationInfo->AppWindow->GetImGuiContext()) {
		ImGui_ImplOpenGL3_NewFrame(); // lazily (re)creates the backend's GL device objects
		ImGuiDrawSnapshot* guiSnapshot = mImguiTripleBuffer.AcquireReadBuffer();
		if (guiSnapshot && guiSnapshot->DrawData.Valid) {
			ImGui_ImplOpenGL3_RenderDrawData(&guiSnapshot->DrawData);
			wasImGuiRendered = true;
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
	//Just straight up load the texture and forget. FOR NOW! :(
	if (!textureInfo) return;
	if (mTextures.Contains(textureInfo->Uuid)) return;
	TUsePointer<Texture> texture = mApplicationInfo->AppObjectManager->CreateObject(Texture::GetStaticClass());
	texture->CreateFromInfo(textureInfo.GetRaw(), false);
	mTextures.Insert(textureInfo->Uuid, mApplicationInfo->AppObjectManager->GetObjectAsOwner<Texture>(*texture->GetEngineObjectHandle()));
	mTextureUsePerFrame[textureInfo->Uuid] = 0;
	PLU_CORE_INFO("Texture {} Loaded!", textureInfo->Uuid.getUUID());
}

Plu::TUsePointer<Plu::Texture> Plu::RenderingManager::GetTextureForInfo(const TUsePointer<TextureInfo>& textureInfo)
{
	if (!textureInfo) return nullptr;
	if (mTextures.Contains(textureInfo->Uuid)) {
		mTextureUsePerFrame[textureInfo->Uuid]++;
		return mTextures[textureInfo->Uuid];
	}
	return nullptr;
}

void Plu::RenderingManager::UnloadTextureForUUID(UInt64 uuid)
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
	for (const auto& textureId : mTextureUsePerFrame) {
		int uses = mTextureUsePerFrame[textureId.first];
		if (uses == 0) {
			if (mTextureFramesWithNoUse.Contains(textureId.first)) {
				mTextureFramesWithNoUse[textureId.first]++;
			} else {
				mTextureFramesWithNoUse.Insert(textureId.first, 0);
			}
		}
	}
	for (const auto& textureId : mTextureUsePerFrame) {
		mTextureUsePerFrame[textureId.first] = 0;
	}
	for (std::pair<unsigned long, int> textureIDp: mTextureFramesWithNoUse) {
		if (textureIDp.second > 100) {
			UnloadTextureForUUID(textureIDp.first);
			mTextureFramesWithNoUse[textureIDp.first] = 0;
			break;
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
	// Block until the render thread has run RenderThreadExit() to completion (GL torn down,
	// shader resources released, ReleaseGLContext()). After join the context is free, so it is
	// safe for Main to make it current and for Run() to delete the SDL/GL context afterwards.
	if (mRenderThread && mRenderThread->joinable()) mRenderThread->join();
	mApplicationInfo->AppWindow->MakeGLContextCurrent();
	DynamicArray<UInt64> textureIds;
	for (const auto& texture : mTextures) {
		textureIds.PushBack(texture.first);
	}
	for (const auto& txt : textureIds) {
		UnloadTextureForUUID(txt);
	}	
}
