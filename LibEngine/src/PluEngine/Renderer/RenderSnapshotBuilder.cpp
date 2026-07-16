//
// Created by Plutex on 6/21/26.
//

#include "PluEngine/Renderer/RenderSnapshotBuilder.h"

#include "PluEngine/Application.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/BasicEngineClasses/Components/CameraComponent.h"
#include "PluEngine/BasicEngineClasses/Components/StaticMeshComponent.h"
#include "PluEngine/BasicEngineClasses/Components/SkeletalMeshComponent.h"
#include "PluEngine/BasicEngineClasses/GameObjects/Lights/DirectionalLight.h"
#include "PluEngine/Renderer/RenderingInterfaces.h"
#include "PluEngine/Renderer/RenderThreading.h"
#include "PluEngine/Renderer/RenderUtils.h"
#include "PluEngine/Renderer/RenderUsageStats.h"
#include "PluEngine/AssetTypes/Texture/Texture.h"
#include "PluEngine/Shaders/ShaderCode.h"
#include "PluEngine/Scenes/SceneManager.h"
#include "PluEngine/Scenes/SceneWorld.h"
#include "PluEngine/Window/Window.h"
#include "PluEngine/Physics/PhysicsWorld.h"
#include "PluEngine/Physics/PhysicsWireframeRenderer.h"
#include "PluEngine/Physics/PhysicsPointRenderer.h"
#include <Jolt/Physics/Body/BodyLock.h>

#include "PluEngine/AssetTypes/Animation/SkeletalAnimation.h"
#include "PluEngine/AssetTypes/Skeleton/Skeleton.h"

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

static Matrix4 gLastProjectionMatrix;
static Matrix4 gLastViewMatrix;

Matrix4 Plu::RenderSnapshotBuilder::GetLastFrameProjectionMatrix()
{
    return gLastProjectionMatrix;
}

Matrix4 Plu::RenderSnapshotBuilder::GetLastFrameViewMatrix()
{
    return gLastViewMatrix;
}

void Plu::RenderSnapshotBuilder::BuildSnapshotAndPublish(float deltaTime)
{
    if (!mTripleBuffer || !mAppInfo) return;

    RenderSnapshot*& snapshot = mTripleBuffer->GetWriteBuffer();
    if (snapshot == nullptr) {
        snapshot = new RenderSnapshot();
    } else {
        snapshot->Clear();
    }

#ifdef PLU_ENGINE_EDITOR_BUILD
    // Nowa klatka dla liczników "hottest" assetów (panel Render/GPU). Rolujemy tu ZAWSZE,
    // także gdy brak sceny — wtedy wszystkie LastFrameUses spadają do 0 (nic nie renderowane).
    RenderUsageStats::GetInstance()->BeginFrame();
#endif

    //Here will be building
    TUsePointer<SceneWorld> sceneWorld = mAppInfo->AppScenesManager->GetCurrentWorld();
    if (!sceneWorld) {
        // Brak aktywnej sceny (np. po jej zamknięciu): publikujemy PUSTY (już wyczyszczony)
        // snapshot, zamiast wychodzić bez Publish(). Inaczej TripleBuffer::AcquireReadBuffer
        // oddawałby wątkowi renderu w kółko ostatni niepusty snapshot i scena „zamarzałaby"
        // na ostatniej klatce. Pusty snapshot ma HasDirLight=false i zero renderable'ów, więc
        // render thread jedynie czyści główny bufor (czarny ekran).
        mTripleBuffer->Publish();
        return;
    }

    // Wybór aktywnej kamery: najpierw kamera pucharka z kontrolera (gra/PIE/runtime), a w
    // edytorze poza PIE fallback na kamerę edytora (EditorSceneCamera). Oba typy implementują
    // IRendererCamera, więc dalej jedna ścieżka.
    IRendererCamera* activeCamera = nullptr;
    if (sceneWorld->GetControllerByID(0)) {
        activeCamera = sceneWorld->GetControllerByID(0)->GetControlledPuppetCamera().GetRaw();
    }
#ifdef PLU_ENGINE_EDITOR_BUILD
    if (mAppInfo->AppScenesManager->GetEditorRenderCamera()) {
        activeCamera = mAppInfo->AppScenesManager->GetEditorRenderCamera();
    }
#endif
    if (activeCamera) {
        snapshot->CameraProjectionMatrix = GetProjectionMatrix(activeCamera);
        snapshot->CameraLocation = activeCamera->GetCameraLocation();
        snapshot->CameraRotation = activeCamera->GetCameraRotation();
        const Matrix4 view = glm::inverse(
        glm::translate(glm::mat4(1.0f), snapshot->CameraLocation) *
        glm::mat4_cast(glm::quat(glm::radians(snapshot->CameraRotation)))
        );
        gLastProjectionMatrix = snapshot->CameraProjectionMatrix;
        gLastViewMatrix = view;
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
            snapshot->StaticMeshRenderObjects.EmplaceBack(worldComponent->GetStaticMesh().IsValid() ? worldComponent->GetStaticMesh()->Uuid : PluUUID(0),
                                                          worldComponent->GetMaterial().IsValid() ? worldComponent->GetMaterial()->Uuid : PluUUID(0),
                                                          worldComponent->GetWorldLocation(),
                                                          glm::quat(glm::radians(worldComponent->GetWorldRotation())),
                                                          worldComponent->GetWorldScale(),
                                                          worldComponent->GetWorldMatrix(),
                                                          worldComponent->CastsShadow);

            if (worldComponent->GetMaterial().IsValid()) {
                if (!mAppInfo->AppAssetManager->IsAssetLoaded(worldComponent->GetMaterial()->Uuid) && worldComponent->GetMaterial()->Uuid.getUUID() != 0) {
                    mAppInfo->AppAssetManager->LoadAssetData(mAppInfo->AppAssetManager->GetAssetDescriptor(worldComponent->GetMaterial()->Uuid));
                }
            }
            if (worldComponent->GetStaticMesh().IsValid()) {
                if (!mAppInfo->AppAssetManager->IsAssetLoaded(worldComponent->GetStaticMesh()->Uuid) && worldComponent->GetStaticMesh()->Uuid.getUUID() != 0) {
                    mAppInfo->AppAssetManager->LoadAssetData(mAppInfo->AppAssetManager->GetAssetDescriptor(worldComponent->GetStaticMesh()->Uuid));
                }
            }

#ifdef PLU_ENGINE_EDITOR_BUILD
            // Liczniki "hottest" assetów: mesh + tekstury materiału tego renderable'a. Tekstury
            // czytamy z uniformów sampler2D materiału — mapy cieni (CSM) są silnikowe i tu nieobecne,
            // więc są naturalnie pominięte, zgodnie z wymogiem "nie licz tekstur cieni".
            if (worldComponent->GetStaticMesh().IsValid()) {
                RenderUsageStats::GetInstance()->RecordMesh(worldComponent->GetStaticMesh()->Uuid.getUUID());
            }
            if (worldComponent->GetMaterial().IsValid()) {
                TUsePointer<MaterialInfo> material = worldComponent->GetMaterial();
                const UInt32 paramCount = material->MaterialParameters.Size();
                for (UInt32 u = 0; u < paramCount; u++) {
                    TUsePointer<IShaderUniform> uniform = material->MaterialParameters.At(u);
                    if (!uniform || uniform->ArraySize != 0 || uniform->Type != "sampler2D") continue;
                    ShaderUniform<TUsePointer<TextureInfo>>* texUniform =
                        static_cast<ShaderUniform<TUsePointer<TextureInfo>>*>(uniform.GetRaw());
                    TUsePointer<TextureInfo> texInfo = texUniform->Data;
                    if (texInfo.IsValid()) {
                        RenderUsageStats::GetInstance()->RecordTexture(texInfo->Uuid.getUUID());
                    }
                }
            }
#endif
        }
    }

    {
        PLU_PROFILE_SCOPE("Skeletal Mesh Calculations");
        for (auto gameObject : sceneWorld->mSkeletalMeshRenderables) {
            for (auto worldComponent : gameObject.second) {
                //Offset, World
                DynamicArray<std::pair<Matrix4, Matrix4>> bones;
                DynamicArray<TOwningPointer<SkeletonNode>>* nodes = worldComponent->GetNodes();

                GameHashMap<String, Matrix4> globalTransforms;

                // AnimationFrameToShow is a tick index (FramesAmount == duration in ticks);
                // clamp so scrubbing past either end holds the boundary pose. During playback
                // the fractional AnimationTimeTicks head wins, so poses interpolate between frames.
                TUsePointer<Animation> animation = worldComponent->AnimationToShow;
                double animTimeTicks = 0.0;
                if (animation) {
                    animTimeTicks = worldComponent->IsPlaying
                        ? glm::clamp(static_cast<double>(worldComponent->AnimationTimeTicks), 0.0, static_cast<double>(animation->FramesAmount))
                        : static_cast<double>(glm::clamp(worldComponent->AnimationFrameToShow, 0, animation->FramesAmount));
                }

                std::function<void(TUsePointer<SkeletonNode>, SkeletonNode*)> calculateMatrices = [&](TUsePointer<SkeletonNode> node, SkeletonNode* parent) {
                    if (!node) return;

                    Matrix4 localMatrix = node->LocalMatrix;
                    if (animation) {
                        if (const AnimationTrack* track = animation->Tracks.Find(node->NodeName)) {
                            const Vec3 loc = track->GetLocationAtTime(animTimeTicks);
                            const Quaternion rotation = track->GetRotationAtTime(animTimeTicks);
                            const Vec3 scale = track->GetScaleAtTime(animTimeTicks);

                            localMatrix = glm::translate(glm::mat4(1.0f), loc) *
                                glm::mat4_cast(rotation) *
                                glm::scale(glm::mat4(1.0f), scale);
                        }
                    }

                    // Temporary editor posing: parent-space delta on this bone (drags subtree).
                    if (!worldComponent->BoneLocalOverrides.IsEmpty()) {
                        if (const Matrix4* ov = worldComponent->BoneLocalOverrides.Find(node->NodeName))
                            localMatrix = (*ov) * localMatrix;
                    }

                    if (!parent) {
                        globalTransforms.Insert(node->NodeName, localMatrix);
                    } else {
                        globalTransforms.Insert(node->NodeName, globalTransforms[parent->NodeName] * localMatrix);
                    }

                    if (const auto* bone = dynamic_cast<const SkeletonBone*>(node.GetRaw()))
                    {
                        bones.PushBack({bone->OffsetMatrix, globalTransforms[bone->NodeName]});
                    }
                    for (UInt64 i = 0; i < node->Children.Size(); ++i)
                        calculateMatrices(node->Children[i], node.GetRaw());
                };

                if (worldComponent->GetSkeletalMesh()) {
                    calculateMatrices(worldComponent->GetSkeletalMesh()->MeshSkeleton->RootNode, nullptr);
                }

                snapshot->SkeletalMeshRenderObjects.EmplaceBack(worldComponent->GetSkeletalMesh().IsValid() ? worldComponent->GetSkeletalMesh()->Uuid : PluUUID(0),
                                                                worldComponent->GetMaterial().IsValid() ? worldComponent->GetMaterial()->Uuid : PluUUID(0),
                                                                worldComponent->GetWorldLocation(),
                                                                glm::quat(glm::radians(worldComponent->GetWorldRotation())),
                                                                worldComponent->GetWorldScale(),
                                                                worldComponent->GetWorldMatrix(),
                                                                worldComponent->CastsShadow,
                                                                &bones);

                if (worldComponent->GetMaterial().IsValid()) {
                    if (!mAppInfo->AppAssetManager->IsAssetLoaded(worldComponent->GetMaterial()->Uuid) && worldComponent->GetMaterial()->Uuid.getUUID() != 0) {
                        mAppInfo->AppAssetManager->LoadAssetData(mAppInfo->AppAssetManager->GetAssetDescriptor(worldComponent->GetMaterial()->Uuid));
                    }
                }
                if (worldComponent->GetSkeletalMesh().IsValid()) {
                    if (!mAppInfo->AppAssetManager->IsAssetLoaded(worldComponent->GetSkeletalMesh()->Uuid) && worldComponent->GetSkeletalMesh()->Uuid.getUUID() != 0) {
                        mAppInfo->AppAssetManager->LoadAssetData(mAppInfo->AppAssetManager->GetAssetDescriptor(worldComponent->GetSkeletalMesh()->Uuid));
                    }
                }

#ifdef PLU_ENGINE_EDITOR_BUILD
                // Liczniki "hottest" assetów: mesh + tekstury materiału tego renderable'a (analogicznie
                // do static-mesh gałęzi wyżej; mapy cieni CSM są silnikowe i naturalnie pominięte).
                if (worldComponent->GetSkeletalMesh().IsValid()) {
                    RenderUsageStats::GetInstance()->RecordSkeletalMesh(worldComponent->GetSkeletalMesh()->Uuid.getUUID());
                }
                if (worldComponent->GetMaterial().IsValid()) {
                    TUsePointer<MaterialInfo> material = worldComponent->GetMaterial();
                    const UInt32 paramCount = material->MaterialParameters.Size();
                    for (UInt32 u = 0; u < paramCount; u++) {
                        TUsePointer<IShaderUniform> uniform = material->MaterialParameters.At(u);
                        if (!uniform || uniform->ArraySize != 0 || uniform->Type != "sampler2D") continue;
                        ShaderUniform<TUsePointer<TextureInfo>>* texUniform =
                            static_cast<ShaderUniform<TUsePointer<TextureInfo>>*>(uniform.GetRaw());
                        TUsePointer<TextureInfo> texInfo = texUniform->Data;
                        if (texInfo.IsValid()) {
                            RenderUsageStats::GetInstance()->RecordTexture(texInfo->Uuid.getUUID());
                        }
                    }
                }
#endif
            }
        }
    }

    // --- Debugowa wizualizacja fizyki ---
    // Ekstrakcja geometrii Jolta i obchodzenie GameObjectów odbywa się TUTAJ, na MAIN
    // (oba są main-only pod thread confinement). Wynik ląduje jako płaskie bufory POD
    // w snapshotcie; wątek renderu tylko je uploaduje do VBO i rysuje (Renderer::RenderDebugGeometry).
    PhysicsWorld* physicsWorld = sceneWorld->GetPhysicsWorld();
    if (physicsWorld)
    {
        PLU_PROFILE_SCOPE("Physics Debug Building");
        const PhysicsDebugRender mode = physicsWorld->PhysicsDebugRenderMode;
        if (mode != PhysicsDebugRender::NONE)
        {
            const Vec3 wireColor  = physicsWorld->PhysicsDebugRenderColorWireframe;
            const Vec3 pointColor = physicsWorld->PhysicsDebugRenderColorPoints;

            JoltWireframeRenderer wire;
            JoltPointRenderer     pts;
            wire.BeginFrame();
            pts.BeginFrame();

            JoltWireframeRenderer* wirePtr = (mode == PhysicsDebugRender::WIREFRAME || mode == PhysicsDebugRender::BOTH) ? &wire : nullptr;
            JoltPointRenderer*     ptsPtr  = (mode == PhysicsDebugRender::POINTS    || mode == PhysicsDebugRender::BOTH) ? &pts  : nullptr;

            // Edytor poza PIE: rysuj kształty kolizji z komponentów (ciała mogą nie istnieć).
            // W PIE / runtime: ekstrahuj aktywne ciała Jolta. Locki ciał są bezpieczne — to main.
            bool playing = true;
#ifdef PLU_ENGINE_EDITOR_BUILD
            playing = mAppInfo->AppScenesManager->IsInPIE();
            if (!playing)
            {
                physicsWorld->DrawEditModeShapes(wirePtr, ptsPtr, wireColor, pointColor);
            }
#endif
            if (playing)
            {
                JPH::BodyIDVector bodies;
                JPH::PhysicsSystem& physicsSystem = physicsWorld->GetSystem();
                physicsSystem.GetBodies(bodies);
                for (JPH::BodyID body : bodies)
                {
                    JPH::BodyLockRead lock(physicsSystem.GetBodyLockInterface(), body);
                    if (!lock.Succeeded()) continue;
                    if (wirePtr) wirePtr->AddBody(lock.GetBody(), wireColor);
                    if (ptsPtr)  ptsPtr->AddBody(lock.GetBody(), pointColor);
                }
            }

            wire.PackInto(snapshot->DebugLineVerts);
            pts.PackInto(snapshot->DebugPointVerts);
        }

        // Raycasty debugowe są niezależne od trybu wizualizacji (włączane flagą DrawDebug
        // w samym Raycast). Decay timerów + pakowanie segmentów do wspólnego bufora linii.
        physicsWorld->CollectDebugRaycasts(deltaTime, snapshot->DebugLineVerts);
    }


    mTripleBuffer->Publish();
}
