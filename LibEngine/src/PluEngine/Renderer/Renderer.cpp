//
// Created by Plutex on 12/30/25.
//

#include "PluEngine/Renderer/Renderer.h"

#include <iostream>
#include <chrono>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_opengl3.h>

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
	if (mRenderables.IsEmpty()) {
		mMainBuffer->Clear(0.0f,0.0f,0.0f,1.0f);
		mMainBuffer->Unbind();
	}
	if (!mApplication->GetAppInfo()->AppShaderManager) return;
	mMainBuffer->Clear(0.0f,0.0f,0.0f,1.0f);
	mMainBuffer->Bind();

#ifdef PLU_ENGINE_EDITOR_BUILD
	if (mApplication->GetAppInfo()->AppScenesManager && mApplication->GetAppInfo()->AppScenesManager->GetCurrentWorld()) {
		mWireframeRenderer->BeginFrame();
		mPointRenderer->BeginFrame();
		switch (PhysicsDebugRenderMode) {
			case PhysicsDebugRender::NONE:
				break;
			case PhysicsDebugRender::POINTS:
			{
				JPH::BodyIDVector bodies;
				JPH::PhysicsSystem& physicsSystem = mApplication->GetAppInfo()->AppScenesManager->GetCurrentWorld()->GetPhysicsWorld()->GetSystem();
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
				JPH::PhysicsSystem& physicsSystem = mApplication->GetAppInfo()->AppScenesManager->GetCurrentWorld()->GetPhysicsWorld()->GetSystem();
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
				JPH::PhysicsSystem& physicsSystem = mApplication->GetAppInfo()->AppScenesManager->GetCurrentWorld()->GetPhysicsWorld()->GetSystem();
				physicsSystem.GetBodies(bodies);
				for (JPH::BodyID body: bodies) {
					JPH::BodyLockRead lock(physicsSystem.GetBodyLockInterface(), body);
					if (lock.Succeeded())
					{
						mPointRenderer->AddBody(lock.GetBody(), PhysicsDebugRenderColorPoints);
					}
				}
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
		DynamicArray<TUsePointer<GameObject> > directionalLights = mApplication->GetAppInfo()->AppScenesManager->
				GetCurrentWorld()->GetAllGameObjectsOfClass(DirectionalLight::GetStaticClass());
		if (directionalLights.Size() > 1) {
			PLU_CORE_ERROR("More than one directional light in the Scene!");
		} else if (directionalLights.Size() == 1) {
			TUsePointer<DirectionalLight> directionalLight = directionalLights.At(0);
			program->SetVec3Uniform("dirLightDir", directionalLight->GetObjectForwardVector());
			program->SetVec4Uniform("dirLightColor", Vec4(directionalLight->GetLightColor(), directionalLight->GetLightIntensity()));
		}
	}

	UInt32 numRenderables = mRenderables.Size();

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

	// static double period = 0.000000003f;
	// double sineWave = (std::sin(period * std::chrono::high_resolution_clock::now().time_since_epoch().count()) + 1) / 2.0f;
	// glClearColor(sineWave, sineWave, sineWave, 1.0f);
	mMainBuffer->Unbind();
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
	constexpr float cameraFarPlane = 100000.0f;
	if (mActiveCamera) {
		switch (mActiveCamera->GetCameraOptions()->CameraPerspective) {
			case PerspectiveType::Perspective:
				return glm::perspective(
				glm::radians(mActiveCamera->GetCameraOptions()->FieldOfView),
				static_cast<float>(mMainBuffer->GetWidth()) / static_cast<float>(mMainBuffer->GetHeight()),
				0.1f, cameraFarPlane);
				break;
			case PerspectiveType::Orthographic:
				return glm::ortho(0.0f - mActiveCamera->GetCameraOptions()->OrthoWidth / 2,
				mActiveCamera->GetCameraOptions()->OrthoWidth / 2,
				0.0f - mActiveCamera->GetCameraOptions()->OrthoWidth / 2,
				mActiveCamera->GetCameraOptions()->OrthoWidth / 2,
				0.1f, cameraFarPlane);
				break;
			default: ;
		}
	}
	return glm::perspective(
				glm::radians(45.0f),
				static_cast<float>(mMainBuffer->GetWidth()) / static_cast<float>(mMainBuffer->GetHeight()),
				0.1f, cameraFarPlane);
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
}
