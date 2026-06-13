//
// Created by Plutex on 12/30/25.
//

#include "PluEngine/Renderer/Renderer.h"

#include <iostream>
#include <chrono>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "EngineAssets.h"
#include "glm/trigonometric.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "PluEngine/Application.h"
#include "PluEngine/Engine.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/BasicEngineClasses/GameObjects/Lights/DirectionalLight.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Managers/ShadersManager.h"
#include "PluEngine/Objects/EngineObjectHandle.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Physics/PhysicsWireframeRenderer.h"
#include "PluEngine/Physics/PhysicsPointRenderer.h"
#include "PluEngine/Physics/PhysicsWorld.h"
#include "PluEngine/Renderer/RenderingInterfaces.h"
#include "PluEngine/Renderer/RenderUtils.h"
#include "PluEngine/Scenes/SceneManager.h"
#include "PluEngine/Scenes/SceneWorld.h"
#include "PluEngine/Shaders/ShaderProgram.h"
#include "PluEngine/Window/WindowManager.h"

#ifdef PLU_PLATFORM_LINUX
#include "glad.h"
#include <imgui/backends/imgui_impl_sdl2.h>
#include <SDL_video.h>
#elif defined(PLU_PLATFORM_WINDOWS)
#include "Platforms/Windows/WindowsWindow.h"
#include "glad/glad_wgl.h"
#include "imgui/backends/imgui_impl_win32.h"
#endif

#include "PluEngine/Core.h"
#include "PluEngine/Log.h"

using namespace Plu;

void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity,
							GLsizei length, const char* message, const void* userParam) {
	// Ignoruj nieistotne kody błędów
	if(id == 131185 || id == 131218 || id == 131204) return;

	PLU_CORE_ERROR("---------------");
	PLU_CORE_ERROR("Debug message ( {} ): {}", id,  message);

	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:         PLU_CORE_ERROR("Severity: HIGH"); break;
		case GL_DEBUG_SEVERITY_MEDIUM:       PLU_CORE_ERROR("Severity: MEDIUM"); break;
		case GL_DEBUG_SEVERITY_LOW:          PLU_CORE_ERROR("Severity: LOW"); break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: PLU_CORE_ERROR("Severity: NOTIFICATION"); break;
	}
}

void Renderer::RenderImGui(int windowID)
{
	Engine::GetEngine()->InitImGui(mApplication->GetAppInfo()->AppWindowsManager->GetWindowAt(windowID)->GetImGuiContext());
	ImGui_ImplOpenGL3_NewFrame();
#ifdef PLU_PLATFORM_LINUX
	ImGui_ImplSDL2_NewFrame();
#elif defined(PLU_PLATFORM_WINDOWS)
	ImGui_ImplWin32_NewFrame();
#endif
	ImGui::NewFrame();

	if (windowID == 0) {
		mApplication->OnImGuiRender();
	} else {
		mApplication->OnImGuiRenderEX(windowID);
	}

	mApplication->GetAppInfo()->AppWindowsManager->GetWindowAt(windowID)->ImGuiItemHovered = ImGui::IsAnyItemHovered();

	try {
		ImGui::Render();
	} catch (...) {
		PLU_CORE_ERROR("Error during ImGui::Render()!");
	}
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


void Renderer::RenderGame(float deltaTime)
{
	glDepthMask(GL_TRUE);
	if (mRenderables.IsEmpty()) {
		mMainBuffer->Clear(0.0f,0.0f,0.0f,1.0f);
		mMainBuffer->Unbind();
	}
	if (!mApplication->GetAppInfo()->AppShaderManager) return;

	//First pass is for DirLight
	static DirectionalLight* dirLight = nullptr;
	if (!dirLight && mApplication->GetAppInfo()->AppScenesManager->IsAnySceneOpen()) {
		DynamicArray<TUsePointer<GameObject> > directionalLights = mApplication->GetAppInfo()->AppScenesManager->
				GetCurrentWorld()->GetAllGameObjectsOfClass(DirectionalLight::GetStaticClass());
#ifdef PLU_ENGINE_EDITOR_BUILD
		if (directionalLights.Size() > 1) {
			PLU_CORE_ERROR("More than 1 DirLight in the scene!");
		}
#endif
		if (directionalLights.Size() == 1) {
			dirLight = dynamic_cast<DirectionalLight *>(directionalLights[0].GetRaw());
		}
	}

	UInt32 numRenderables = mRenderables.Size();
	Matrix4 lightViewMatrix;
	Matrix4 lightProjectionMatrix;
	DynamicArray<ShadowCascadeData> cascades;

	if (dirLight && GetCamera()) {
		if (mCascadeFrameBuffers.IsEmpty()) {
			for (int c = 0; c < kCascadeCount; c++) {
				EngineObjectHandle hdl = mApplication->GetAppObjectManager()->CreateObject<FrameBuffer>();
				TOwningPointer<FrameBuffer> fb = mApplication->GetAppObjectManager()->GetObjectAsOwner<FrameBuffer>(hdl);
				fb->Create(1024 * 4, 1024 * 4, mApplication->GetAppObjectManager(), FrameBufferType::DepthOnly);
				mCascadeFrameBuffers.PushBack(fb);
			}
#ifdef PLU_ENGINE_EDITOR_BUILD
			EngineObjectHandle editorHdl = mApplication->GetAppObjectManager()->CreateObject<FrameBuffer>();
			mEditorDirLightFrameBuffer = mApplication->GetAppObjectManager()->GetObjectAsOwner<FrameBuffer>(editorHdl);
			mEditorDirLightFrameBuffer->Create(1024 * 2, 1024 * 2, mApplication->GetAppObjectManager(), FrameBufferType::ColorDepth);
#endif
		}

		float width  = static_cast<float>(mApplication->GetAppInfo()->AppWindow->GetWidth());
		float height = static_cast<float>(mApplication->GetAppInfo()->AppWindow->GetHeight());
		float fovRad = glm::radians(GetCamera()->GetCameraOptions()->FieldOfView);

		auto splits = Plu::GetCascadeSplits(kCascadeCount, Plu::kCameraNearClip, Plu::kShadowFarClip, 0.6f);
		cascades = Plu::GetCascadedLightMatrices(
			GetViewMatrix(), fovRad, width / height,
			Plu::kCameraNearClip, Plu::kShadowFarClip,
			dirLight->GetObjectForwardVector(),
			splits
		);

		// Editor debug: full-frustum light matrices for the light-space viewport
		DynamicArray<Vec3> corners = GetFrustumCornersWorldSpace(GetProjectionMatrix(), GetViewMatrix());
		lightViewMatrix       = GetLightViewMatrix(corners, dirLight->GetObjectForwardVector());
		lightProjectionMatrix = GetLightProjectionMatrix(corners, lightViewMatrix);

		TUsePointer<ShaderProgram> dirLightShader = mApplication->GetAppInfo()->AppShaderManager->GetShaderProgram(EngineAssets::OnlyPositionShader);
		if (!dirLightShader->IsLoaded()) {
			mApplication->GetAppInfo()->AppShaderManager->LoadShader(dirLightShader->Uuid);
		}

		glViewport(0, 0, mCascadeFrameBuffers[0]->GetWidth(), mCascadeFrameBuffers[0]->GetHeight());
		for (int c = 0; c < kCascadeCount; c++) {
			dirLightShader->Bind();
			dirLightShader->SetMatrix4Uniform("lightSpaceMatrix", cascades[c].viewProj);

			mCascadeFrameBuffers[c]->Clear(0.0f, 0.0f, 0.0f, 1.0f);
			mCascadeFrameBuffers[c]->Bind();

			for (UInt32 i = 0; i < numRenderables; i++) {
				IRenderable* renderable = mRenderables.At(i);
				StaticMesh* mesh = renderable->GetStaticMeshToRender();
				if (!mesh) continue;
				if (!mesh->IsLoaded) {
					SetupStaticMeshGL(&mesh->StaticMeshData, mesh);
					continue;
				}
				dirLightShader->SetMatrix4Uniform("model", renderable->GetRenderMatrix());
				DrawStaticMesh(mesh);
			}

			mCascadeFrameBuffers[c]->Unbind();
		}
		glViewport(0, 0,
			static_cast<int>(width),
			static_cast<int>(height));
	}

	mMainBuffer->Clear(0.0f,0.0f,0.0f,1.0f);
	mMainBuffer->Bind();

#ifdef PLU_ENGINE_EDITOR_BUILD
	if (mApplication->GetAppInfo()->AppScenesManager && mApplication->GetAppInfo()->AppScenesManager->GetCurrentWorld()) {
		mWireframeRenderer->BeginFrame();
		mPointRenderer->BeginFrame();

		bool inPIE = mApplication->GetAppInfo()->AppScenesManager->IsInPIE();
		PhysicsWorld* physicsWorld = mApplication->GetAppInfo()->AppScenesManager->GetCurrentWorld()->GetPhysicsWorld();

		if (!inPIE && physicsWorld) {
			JoltWireframeRenderer* wire = (PhysicsDebugRenderMode == PhysicsDebugRender::WIREFRAME || PhysicsDebugRenderMode == PhysicsDebugRender::BOTH)
				? mWireframeRenderer.GetRaw() : nullptr;
			JoltPointRenderer* pts = (PhysicsDebugRenderMode == PhysicsDebugRender::POINTS || PhysicsDebugRenderMode == PhysicsDebugRender::BOTH)
				? mPointRenderer.GetRaw() : nullptr;
			physicsWorld->DrawEditModeShapes(wire, pts, PhysicsDebugRenderColorWireframe, PhysicsDebugRenderColorPoints);
		} else if (inPIE && physicsWorld) {
			switch (PhysicsDebugRenderMode) {
				case PhysicsDebugRender::NONE:
					break;
				case PhysicsDebugRender::POINTS:
				{
					JPH::BodyIDVector bodies;
					JPH::PhysicsSystem& physicsSystem = physicsWorld->GetSystem();
					physicsSystem.GetBodies(bodies);
					for (JPH::BodyID body: bodies) {
						JPH::BodyLockRead lock(physicsSystem.GetBodyLockInterface(), body);
						if (lock.Succeeded())
						{
							mPointRenderer->AddBody(lock.GetBody(), PhysicsDebugRenderColorPoints);
						}
					}
					break;
				}
				case PhysicsDebugRender::WIREFRAME:
				{
					JPH::BodyIDVector bodies;
					JPH::PhysicsSystem& physicsSystem = physicsWorld->GetSystem();
					physicsSystem.GetBodies(bodies);
					for (JPH::BodyID body: bodies) {
						JPH::BodyLockRead lock(physicsSystem.GetBodyLockInterface(), body);
						if (lock.Succeeded())
						{
							mWireframeRenderer->AddBody(lock.GetBody(), PhysicsDebugRenderColorWireframe);
						}
					}
					break;
				}
				case PhysicsDebugRender::BOTH:
				{
					JPH::BodyIDVector bodies;
					JPH::PhysicsSystem& physicsSystem = physicsWorld->GetSystem();
					physicsSystem.GetBodies(bodies);
					for (JPH::BodyID body: bodies) {
						JPH::BodyLockRead lock(physicsSystem.GetBodyLockInterface(), body);
						if (lock.Succeeded())
						{
							mPointRenderer->AddBody(lock.GetBody(), PhysicsDebugRenderColorPoints);
							mWireframeRenderer->AddBody(lock.GetBody(), PhysicsDebugRenderColorWireframe);
						}
					}
					break;
				}
			}
		}
	}

	mWireframeRenderer->Render(GetProjectionMatrix() * GetViewMatrix());
	mPointRenderer->Render(GetProjectionMatrix() * GetViewMatrix(), 10);
	if (mApplication->GetAppInfo()->AppScenesManager->GetCurrentWorld()) {
		if (mApplication->GetAppInfo()->AppScenesManager->GetCurrentWorld()->GetPhysicsWorld()) {
			mApplication->GetAppInfo()->AppScenesManager->GetCurrentWorld()->GetPhysicsWorld()->DrawDebugRaycasts(deltaTime, GetProjectionMatrix()*GetViewMatrix(), mApplication->GetAppInfo()->AppShaderManager);
		}
	}
#endif

	DynamicArray<TUsePointer<ShaderProgram>>* shaderPrograms = mApplication->GetAppInfo()->AppShaderManager->GetRenderableShaderPrograms();
	UInt32 numShaderPrograms = shaderPrograms ? shaderPrograms->Size() : 0;

	for (UInt32 i = 0; i < numShaderPrograms; i++) {
		ShaderProgram* program = shaderPrograms->At(i).GetRaw();
		program->SetMatrix4Uniform("projection", GetProjectionMatrix());
		program->SetMatrix4Uniform("view", GetViewMatrix());
		if (mActiveCamera) {
			program->SetVec3Uniform("cameraPos", mActiveCamera->GetCameraLocation());
		}
		if (!mApplication->GetAppInfo()->AppScenesManager->IsAnySceneOpen()) continue;
		if (!dirLight) continue;
		program->SetVec3Uniform("dirLightDir", dirLight->GetObjectForwardVector());
		program->SetVec4Uniform("dirLightColor", Vec4(dirLight->GetLightColor(), dirLight->GetLightIntensity()));
	}

	for (UInt32 i = 0; i < numRenderables; i++) {
		IRenderable* renderable = mRenderables.At(i);
		MaterialInfo* material = renderable->GetMaterialInfoToRender();
		if (!material) continue;
		ShaderProgram* program = mApplication->GetAppInfo()->AppShaderManager->GetShaderProgram(material->shaderProgram).GetRaw();
		StaticMesh* mesh = renderable->GetStaticMeshToRender();
		if (!mesh || !program) {
			continue;
		}
		if (!program->IsLoaded()) {
			mApplication->GetAppInfo()->AppShaderManager->LoadShader(program->Uuid);
			continue;
		}
		if (!mesh->IsLoaded) {
			SetupStaticMeshGL(&mesh->StaticMeshData, mesh);
			continue;
		}

		Matrix4 model = renderable->GetRenderMatrix();
		//Placeholder Model Matrix
		program->RenderFromMaterial(material, mApplication->GetAppInfo()->AppRenderingManager);
		program->SetMatrix4Uniform("model", model);
		if (!mCascadeFrameBuffers.IsEmpty() && !cascades.IsEmpty()) {
			for (int c = 0; c < kCascadeCount; c++) {
				String ci = String::FromInt(c);
				String matName = "cascadeLightSpaceMatrices["; matName += ci; matName += "]";
				String texName = "cascadeShadowMaps[";         texName += ci; texName += "]";
				String sptName = "cascadeSplitDistances[";     sptName += ci; sptName += "]";
				program->SetMatrix4Uniform(matName, cascades[c].viewProj);
				program->SetTextureUniform(texName, mCascadeFrameBuffers[c]->GetDepthTexture(), c);
				program->SetFloatUniform(sptName, cascades[c].splitDistance);
			}
			program->SetIntUniform("cascadeCount", kCascadeCount);
		}
		DrawStaticMesh(mesh);
	}

	// static double period = 0.000000003f;
	// double sineWave = (std::sin(period * std::chrono::high_resolution_clock::now().time_since_epoch().count()) + 1) / 2.0f;
	// glClearColor(sineWave, sineWave, sineWave, 1.0f);
	mMainBuffer->Unbind();

#ifdef PLU_ENGINE_EDITOR_BUILD
	if (mEditorDirLightFrameBuffer) {
		mEditorDirLightFrameBuffer->Clear();
		mEditorDirLightFrameBuffer->Bind();

		for (UInt32 i = 0; i < numShaderPrograms; i++) {
			ShaderProgram* program = shaderPrograms->At(i).GetRaw();
			program->SetMatrix4Uniform("projection", lightProjectionMatrix);
			program->SetMatrix4Uniform("view", lightViewMatrix);
		}

		for (UInt32 i = 0; i < numRenderables; i++) {
			IRenderable* renderable = mRenderables.At(i);
			MaterialInfo* material = renderable->GetMaterialInfoToRender();
			if (!material) continue;
			ShaderProgram* program = mApplication->GetAppInfo()->AppShaderManager->GetShaderProgram(material->shaderProgram).GetRaw();
			StaticMesh* mesh = renderable->GetStaticMeshToRender();
			if (!mesh || !program) {
				continue;
			}
			if (!program->IsLoaded()) {
				mApplication->GetAppInfo()->AppShaderManager->LoadShader(program->Uuid);
				continue;
			}
			if (!mesh->IsLoaded) {
				SetupStaticMeshGL(&mesh->StaticMeshData, mesh);
				continue;
			}

			Matrix4 model = renderable->GetRenderMatrix();
			//Placeholder Model Matrix
			program->RenderFromMaterial(material, mApplication->GetAppInfo()->AppRenderingManager);
			program->SetMatrix4Uniform("model", model);
			DrawStaticMesh(mesh);
		}

		mEditorDirLightFrameBuffer->Unbind();
	}
#endif
}

void Renderer::RenderThread()
{
	PLU_CORE_INFO("Render Thread Init");
	while (mIsRenderThreadRunning) {
		RenderLoop();
	}
	RenderQuit();
}

void Renderer::RenderLoop()
{
}

void Renderer::RenderQuit()
{
	PLU_CORE_INFO("Render Thread Quit");
}

Renderer::Renderer() : mApplication(nullptr)
{
}

void Renderer::Init(Application *application)
{
	mApplication = application;
}

Renderer::~Renderer()
{
}

TUsePointer<FrameBuffer> Renderer::GetMainBuffer()
{
	return mMainBuffer;
}

void Renderer::AddRenderable(IRenderable *renderable)
{
	mRenderables.PushBack(renderable);
}

void Renderer::RemoveRenderable(IRenderable *renderable)
{
	mRenderables.Remove(renderable);
}

UInt64 Renderer::NumOfRenderables()
{
	return mRenderables.Size();
}

void Renderer::ClearRenderables()
{
	mRenderables.Clear();
}

void Renderer::SetCamera(IRendererCamera *newCamera)
{
	mActiveCamera = newCamera;
}

IRendererCamera * Renderer::GetCamera()
{
	return mActiveCamera;
}

Matrix4 Renderer::GetProjectionMatrix() const
{
	if (mActiveCamera) {
		switch (mActiveCamera->GetCameraOptions()->CameraPerspective) {
			case PerspectiveType::Perspective:
				return glm::perspective(
				glm::radians(mActiveCamera->GetCameraOptions()->FieldOfView),
				static_cast<float>(mMainBuffer->GetWidth()) / static_cast<float>(mMainBuffer->GetHeight()),
				Plu::kCameraNearClip, Plu::kCameraFarClip);
				break;
			case PerspectiveType::Orthographic:
				return glm::ortho(0.0f - mActiveCamera->GetCameraOptions()->OrthoWidth / 2,
				mActiveCamera->GetCameraOptions()->OrthoWidth / 2,
				0.0f - mActiveCamera->GetCameraOptions()->OrthoWidth / 2,
				mActiveCamera->GetCameraOptions()->OrthoWidth / 2,
				Plu::kCameraNearClip, Plu::kCameraFarClip);
				break;
			default: ;
		}
	}
	return glm::perspective(
				glm::radians(45.0f),
				static_cast<float>(mMainBuffer->GetWidth()) / static_cast<float>(mMainBuffer->GetHeight()),
				Plu::kCameraNearClip, Plu::kCameraFarClip);
}

Matrix4 Renderer::GetViewMatrix()
{
	if (mActiveCamera) {
		return glm::inverse(
		glm::translate(glm::mat4(1.0f), mActiveCamera->GetCameraLocation()) *
		glm::mat4_cast(glm::quat(glm::radians(mActiveCamera->GetCameraRotation())))
		);
	}
	glm::mat4 view = glm::inverse(
		glm::translate(glm::mat4(1.0f), glm::vec3(0.0f)) *
		glm::mat4_cast(glm::quat(glm::radians(glm::vec3(0.0f))))
		);
	return view;
}

void Renderer::Init(const TUsePointer<IWindow>& appWindow)
{
#ifdef PLU_PLATFORM_LINUX
	if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
		PLU_CORE_CRITICAL("Failed to load GLAD!");
		std::terminate();
	}
	PLU_CORE_ASSERT(SDL_GL_GetCurrentContext() != nullptr, "GL Context is null!");
#endif

	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Dzięki temu callback wykona się natychmiast
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}

	glViewport(0,0,1,1);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	IMGUI_CHECKVERSION();

	int height = mApplication->GetAppWindow()->GetHeight();
	int width = mApplication->GetAppWindow()->GetWidth();
	const EngineObjectHandle mainBufferHandle = mApplication->GetAppObjectManager()->CreateObject<FrameBuffer>();
	mMainBuffer = mApplication->GetAppObjectManager()->GetObjectAsOwner<FrameBuffer>(mainBufferHandle);
	mMainBuffer->Create(width, height, mApplication->GetAppObjectManager(), FrameBufferType::ColorDepth);

	mWireframeRenderer = CreateOwning<JoltWireframeRenderer>(mApplication->GetAppInfo()->AppShaderManager);
	mPointRenderer = CreateOwning<JoltPointRenderer>(mApplication->GetAppInfo()->AppShaderManager);

	mRenderThread = CreateOwning<std::thread>(&Renderer::RenderThread, this);
	mRenderThread->detach();
}

void Renderer::OnUpdate(float deltaTime)
{
	for (int i = 0; i < mApplication->GetAppInfo()->AppWindowsManager->GetWindowsAmount(); i++) {
		if (!mApplication->GetAppInfo()->AppWindowsManager->GetWindowAt(i)) continue;
		mApplication->GetAppInfo()->AppWindowsManager->GetWindowAt(i)->MakeGLContextCurrent();
		ImGui::SetCurrentContext(mApplication->GetAppInfo()->AppWindowsManager->GetWindowAt(i)->GetImGuiContext());
		glViewport(0,0,mApplication->GetAppInfo()->AppWindowsManager->GetWindowAt(i)->GetWidth(), mApplication->GetAppInfo()->AppWindowsManager->GetWindowAt(i)->GetHeight());
		if (i == 0) {
			RenderGame(deltaTime);
		}
		// glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		// glClear(GL_COLOR_BUFFER_BIT);
		RenderImGui(i);
		mApplication->GetAppInfo()->AppWindowsManager->GetWindowAt(i)->SwapBuffer();
	}

	int width = mApplication->GetAppWindow()->GetWidth();
	int height = mApplication->GetAppWindow()->GetHeight();

	if (mMainBuffer->GetHeight() != height || mMainBuffer->GetWidth() != width) {
		mMainBuffer->Resize(width, height);
	}
}

void Renderer::OnShutdown()
{
	mIsRenderThreadRunning = false;
#ifdef PLU_PLATFORM_LINUX
	ImGui_ImplSDL2_Shutdown();
#elif defined(PLU_PLATFORM_WINDOWS)
	ImGui_ImplWin32_Shutdown();
#endif
	ImGui_ImplOpenGL3_Shutdown();
	ImGui::DestroyContext();

	mMainBuffer->Destroy();
	mApplication->GetAppObjectManager()->DestroyObject(*mMainBuffer->GetEngineObjectHandle());
	mMainBuffer = nullptr;

	for (UInt32 c = 0; c < mCascadeFrameBuffers.Size(); c++) {
		if (mCascadeFrameBuffers[c]) {
			mCascadeFrameBuffers[c]->Destroy();
			mApplication->GetAppObjectManager()->DestroyObject(*mCascadeFrameBuffers[c]->GetEngineObjectHandle());
			mCascadeFrameBuffers[c] = nullptr;
		}
	}
	mCascadeFrameBuffers.Clear();

#ifdef PLU_ENGINE_EDITOR_BUILD
	if (mEditorDirLightFrameBuffer) {
		mEditorDirLightFrameBuffer->Destroy();
		mApplication->GetAppObjectManager()->DestroyObject(*mEditorDirLightFrameBuffer->GetEngineObjectHandle());
		mEditorDirLightFrameBuffer = nullptr;
	}
#endif
}
