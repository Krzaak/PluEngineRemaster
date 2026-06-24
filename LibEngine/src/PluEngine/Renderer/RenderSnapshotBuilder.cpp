//
// Created by Plutex on 6/21/26.
//

#include "PluEngine/Renderer/RenderSnapshotBuilder.h"

#include "PluEngine/Application.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/BasicEngineClasses/Components/CameraComponent.h"
#include "PluEngine/BasicEngineClasses/Components/StaticMeshComponent.h"
#include "PluEngine/BasicEngineClasses/GameObjects/Lights/DirectionalLight.h"
#include "PluEngine/Renderer/RenderingInterfaces.h"
#include "PluEngine/Renderer/RenderThreading.h"
#include "PluEngine/Renderer/RenderUtils.h"
#include "PluEngine/Scenes/SceneManager.h"
#include "PluEngine/Scenes/SceneWorld.h"
#include "PluEngine/Window/Window.h"

Matrix4 Plu::RenderSnapshotBuilder::GetProjectionMatrix(IRendererCamera* camera) const
{
    TUsePointer<IWindow> window = mAppInfo->AppWindow;
    if (camera) {
        switch (camera->GetCameraOptions()->CameraPerspective) {
            case PerspectiveType::Perspective:
                return glm::perspective(
                glm::radians(camera->GetCameraOptions()->FieldOfView),
                static_cast<float>(window->GetWidth()) / static_cast<float>(window->GetHeight()),
                Plu::kCameraNearClip, Plu::kCameraFarClip);
                break;
            case PerspectiveType::Orthographic:
                return glm::ortho(0.0f - camera->GetCameraOptions()->OrthoWidth / 2,
                camera->GetCameraOptions()->OrthoWidth / 2,
                0.0f - camera->GetCameraOptions()->OrthoWidth / 2,
                camera->GetCameraOptions()->OrthoWidth / 2,
                Plu::kCameraNearClip, Plu::kCameraFarClip);
                break;
            default: ;
        }
    }
    return glm::perspective(
                glm::radians(45.0f),
                static_cast<float>(window->GetWidth()) / static_cast<float>(window->GetHeight()),
                Plu::kCameraNearClip, Plu::kCameraFarClip);
}

Matrix4 Plu::RenderSnapshotBuilder::GetViewMatrix(IRendererCamera* camera) const
{
    if (camera) {
        return glm::inverse(
        glm::translate(glm::mat4(1.0f), camera->GetCameraLocation()) *
        glm::mat4_cast(glm::quat(glm::radians(camera->GetCameraRotation())))
        );
    }
    glm::mat4 view = glm::inverse(
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f)) *
        glm::mat4_cast(glm::quat(glm::radians(glm::vec3(0.0f))))
        );
    return view;
}

Plu::RenderSnapshotBuilder::RenderSnapshotBuilder() : RenderSnapshotBuilder(nullptr, nullptr)
{
}

Plu::RenderSnapshotBuilder::RenderSnapshotBuilder(TripleBuffer<RenderSnapshot *> *tripleBuffer,
    ApplicationInfo *applicationInfo)
{
    this->mTripleBuffer = tripleBuffer;
    this->mAppInfo = applicationInfo;
    PLU_CORE_TRACE("Render Snapshot Builder Initialized");
}

Plu::RenderSnapshotBuilder::~RenderSnapshotBuilder()
{
}

void Plu::RenderSnapshotBuilder::BuildSnapshotAndPublish()
{
    if (!mTripleBuffer || !mAppInfo) return;

    RenderSnapshot*& snapshot = mTripleBuffer->GetWriteBuffer();
    if (snapshot == nullptr) {
        snapshot = new RenderSnapshot();
    } else {
        snapshot->Clear();
    }

    //Here will be building
    TUsePointer<SceneWorld> sceneWorld = mAppInfo->AppScenesManager->GetCurrentWorld();
    if (!sceneWorld) return;

    // Wybór aktywnej kamery: najpierw kamera pucharka z kontrolera (gra/PIE/runtime), a w
    // edytorze poza PIE fallback na kamerę edytora (EditorSceneCamera). Oba typy implementują
    // IRendererCamera, więc dalej jedna ścieżka.
    IRendererCamera* activeCamera = nullptr;
    if (sceneWorld->GetControllerByID(0)) {
        activeCamera = sceneWorld->GetControllerByID(0)->GetControlledPuppetCamera().GetRaw();
    }
#ifdef PLU_ENGINE_EDITOR_BUILD
    if (!activeCamera) {
        activeCamera = mAppInfo->AppScenesManager->GetEditorRenderCamera();
    }
#endif
    if (activeCamera) {
        snapshot->CameraProjectionMatrix = GetProjectionMatrix(activeCamera);
        snapshot->CameraLocation = activeCamera->GetCameraLocation();
        snapshot->CameraRotation = activeCamera->GetCameraRotation();
        if (activeCamera->GetCameraOptions()) {
            snapshot->CameraFOV = activeCamera->GetCameraOptions()->FieldOfView;
        }
    }

    if (sceneWorld->mDirectionalLight) {
        TUsePointer<DirectionalLight> dirLight = sceneWorld->mDirectionalLight;
        snapshot->HasDirLight = true;
        snapshot->DirLight.Color = dirLight->GetLightColor();
        snapshot->DirLight.Intensity = dirLight->GetLightIntensity();
        snapshot->DirLight.Direction = dirLight->GetObjectForwardVector();
        snapshot->DirLight.Location = dirLight->GetObjectLocation();
        snapshot->DirLight.Rotation = dirLight->GetObjectRotation();
        snapshot->DirLight.Scale = dirLight->GetObjectScale();
        snapshot->DirLight.Type = RenderObjectType::DIRECTIONAL_LIGHT;
    }

    for (auto gameObject : sceneWorld->mStaticMeshRenderables) {
        for (auto worldComponent : gameObject.second) {
            snapshot->StaticMeshRenderObjects.EmplaceBack(worldComponent->GetStaticMesh()->Uuid,
                                                          worldComponent->GetMaterial()->Uuid,
                                                          worldComponent->GetWorldLocation(),
                                                          glm::quat(glm::radians(worldComponent->GetWorldRotation())),
                                                          worldComponent->GetWorldScale(),
                                                          worldComponent->GetWorldMatrix(),
                                                          worldComponent->CastsShadow());

            if (!mAppInfo->AppAssetManager->IsAssetLoaded(worldComponent->GetMaterial()->Uuid) && worldComponent->GetMaterial()->Uuid.getUUID() != 0) {
                mAppInfo->AppAssetManager->LoadAssetData(mAppInfo->AppAssetManager->GetAssetDescriptor(worldComponent->GetMaterial()->Uuid));
            }
            if (!mAppInfo->AppAssetManager->IsAssetLoaded(worldComponent->GetStaticMesh()->Uuid) && worldComponent->GetStaticMesh()->Uuid.getUUID() != 0) {
                mAppInfo->AppAssetManager->LoadAssetData(mAppInfo->AppAssetManager->GetAssetDescriptor(worldComponent->GetStaticMesh()->Uuid));
            }
        }
    }


    mTripleBuffer->Publish();
}
