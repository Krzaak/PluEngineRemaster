//
// Created by Plutex on 6/21/26.
//

#include "PluEngine/Renderer/RenderSnapshotBuilder.h"

#include <cstdint>

#include "PluEngine/Application.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/BasicEngineClasses/Components/CameraComponent.h"
#include "PluEngine/BasicEngineClasses/Components/StaticMeshComponent.h"
#include "PluEngine/BasicEngineClasses/Components/InstancedStaticMeshComponent.h"
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

namespace
{
    // Hash klucza batcha instancingu (MeshUUID, MaterialUUID, CastsShadow) na potrzeby
    // Plu::RenderSnapshotBuilder::mBatchLookup. Kolizje rozwiązuje pełne porównanie klucza
    // w kubełku (patrz BuildSnapshotAndPublish), więc to nie musi być kryptograficznie mocne.
    UInt64 HashBatchKey(UInt64 meshUuid, UInt64 materialUuid, bool castsShadow)
    {
        UInt64 h = meshUuid;
        h ^= materialUuid + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        h ^= (castsShadow ? 1ULL : 0ULL) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        return h;
    }
}

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

    {
        PLU_PROFILE_SCOPE("RenderSnapshotBuilder::BatchStaticMeshes");

        // Bucketing na hashu klucza, NIE sortowanie listy instancji (patrz pułapka QuickSorta
        // przy DynamicArray::Sort - 10000 obiektów o identycznym kluczu to O(n^2)/przepełnienie
        // stosu). mBatchLookup i mBatchScratch to membery buildera, reużywane między klatkami.
        mBatchLookup.Clear();
        mBatchScratch.Clear();

        // Wspólny dodawacz instancji do batcha — używany zarówno przez auto-batching luźnych
        // StaticMeshComponent poniżej, jak i przez InstancedStaticMeshComponent (faza 3): ISMC
        // o 500 instancjach i 500 luźnych komponentów na tym samym meshu+materiale trafiają do
        // tego samego batcha, bo klucz (MeshUUID, MaterialUUID, CastsShadow) nie rozróżnia źródła.
        auto AddInstanceToBatch = [&](PluUUID meshUuid, PluUUID materialUuid, bool castsShadow,
                                       const Matrix4& modelMatrix, const Vec3& boundsCenter, float boundsRadius) {
            const UInt64 keyHash = HashBatchKey(meshUuid.getUUID(), materialUuid.getUUID(), castsShadow);
            DynamicArray<UInt32>* bucket = mBatchLookup.Find(keyHash);
            if (!bucket) {
                mBatchLookup.Insert(keyHash, DynamicArray<UInt32>());
                bucket = mBatchLookup.Find(keyHash);
            }
            UInt32 batchIndex = UINT32_MAX;
            for (UInt32 candidate : *bucket) {
                const StaticMeshBatch& existing = snapshot->StaticMeshBatches[candidate];
                if (existing.MeshUUID == meshUuid && existing.MaterialUUID == materialUuid && existing.CastsShadow == castsShadow) {
                    batchIndex = candidate;
                    break;
                }
            }
            if (batchIndex == UINT32_MAX) {
                batchIndex = snapshot->StaticMeshBatches.Size();
                StaticMeshBatch newBatch;
                newBatch.MeshUUID = meshUuid;
                newBatch.MaterialUUID = materialUuid;
                newBatch.CastsShadow = castsShadow;
                snapshot->StaticMeshBatches.PushBack(newBatch);
                bucket->PushBack(batchIndex);
            }

            StaticMeshBatch& batch = snapshot->StaticMeshBatches[batchIndex];
            batch.TotalCount++;
            batch.VisibleCount++; // Fazy 1-3: brak cullingu, culling przyjdzie w fazie 4

            StaticMeshBatchScratchEntry& scratch = mBatchScratch.EmplaceBack();
            scratch.BatchIndex = batchIndex;
            scratch.Instance.ModelMatrix = modelMatrix;
            scratch.Instance.NormalMatrix = glm::transpose(glm::inverse(modelMatrix));
            scratch.Bounds.BoundsCenter = boundsCenter;
            scratch.Bounds.BoundsRadius = boundsRadius;
        };

        for (auto gameObject : sceneWorld->mStaticMeshRenderables) {
            for (auto worldComponent : gameObject.second) {
                const PluUUID meshUuid = worldComponent->GetStaticMesh().IsValid() ? worldComponent->GetStaticMesh()->Uuid : PluUUID(0);
                const PluUUID materialUuid = worldComponent->GetMaterial().IsValid() ? worldComponent->GetMaterial()->Uuid : PluUUID(0);
                const bool castsShadow = worldComponent->CastsShadow;

                snapshot->StaticMeshRenderObjects.EmplaceBack(meshUuid,
                                                              materialUuid,
                                                              worldComponent->GetWorldLocation(),
                                                              glm::quat(glm::radians(worldComponent->GetWorldRotation())),
                                                              worldComponent->GetWorldScale(),
                                                              worldComponent->GetWorldMatrix(),
                                                              castsShadow);

                if (worldComponent->GetMaterial().IsValid()) {
                    if (!mAppInfo->AppAssetManager->IsAssetLoaded(worldComponent->GetMaterial()->Uuid) && worldComponent->GetMaterial()->Uuid.getUUID() != 0) {
                        mAppInfo->AppAssetManager->LoadAssetData(mAppInfo->AppAssetManager->GetAssetDescriptor(worldComponent->GetMaterial()->Uuid));
                    }
                }
                if (worldComponent->GetStaticMesh().IsValid()) {
                    if (!mAppInfo->AppAssetManager->IsAssetLoaded(worldComponent->GetStaticMesh()->Uuid) && worldComponent->GetStaticMesh()->Uuid.getUUID() != 0) {
                        mAppInfo->AppAssetManager->LoadAssetData(mAppInfo->AppAssetManager->GetAssetDescriptor(worldComponent->GetStaticMesh()->Uuid));
                    }
                    // Mesh dojeżdża asynchronicznie (LoadAssetData powyżej); jeśli SetStaticMesh nie
                    // zdążył policzyć MeshBoundingBox (mesh był jeszcze niezaładowany), dogoń to tutaj —
                    // raz, gdy mesh jest już gotowy (guard: MeshBoundingBoxComputed).
                    if (!worldComponent->MeshBoundingBoxComputed && worldComponent->GetStaticMesh()->IsLoaded) {
                        worldComponent->MeshBoundingBox = Plu::CreateBoundingBoxForStaticMesh(worldComponent->GetStaticMesh().GetRaw());
                        worldComponent->MeshBoundingBoxComputed = true;
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

                // --- Batching instancingu: klucz = (MeshUUID, MaterialUUID, CastsShadow) ---
                const Matrix4 modelMatrix = worldComponent->GetWorldMatrix();
                const Vec3 boundsCenter = Vec3(modelMatrix * Vec4(worldComponent->MeshBoundingBox.GetCenter(), 1.0f));
                const float boundsRadius = glm::length(worldComponent->MeshBoundingBox.GetExtent() * glm::abs(worldComponent->GetWorldScale()));
                AddInstanceToBatch(meshUuid, materialUuid, castsShadow, modelMatrix, boundsCenter, boundsRadius);
            }
        }

        {
            PLU_PROFILE_SCOPE("RenderSnapshotBuilder::BatchInstancedStaticMeshes");

            // InstancedStaticMeshComponent (faza 3): każda instancja w Instances staje się jedną
            // pozycją w tym samym batchu co auto-batching powyżej. Klucz identyczny (MeshUUID,
            // MaterialUUID, CastsShadow), więc ISMC i luźne StaticMeshComponent na tym samym
            // meshu+materiale scalają się w jeden draw call.
            for (auto gameObject : sceneWorld->mInstancedMeshRenderables) {
                for (auto ismc : gameObject.second) {
                    const PluUUID meshUuid = ismc->GetStaticMesh().IsValid() ? ismc->GetStaticMesh()->Uuid : PluUUID(0);
                    const PluUUID materialUuid = ismc->GetMaterial().IsValid() ? ismc->GetMaterial()->Uuid : PluUUID(0);
                    const bool castsShadow = ismc->CastsShadow;

                    if (ismc->GetMaterial().IsValid()) {
                        if (!mAppInfo->AppAssetManager->IsAssetLoaded(ismc->GetMaterial()->Uuid) && ismc->GetMaterial()->Uuid.getUUID() != 0) {
                            mAppInfo->AppAssetManager->LoadAssetData(mAppInfo->AppAssetManager->GetAssetDescriptor(ismc->GetMaterial()->Uuid));
                        }
                    }
                    if (ismc->GetStaticMesh().IsValid()) {
                        if (!mAppInfo->AppAssetManager->IsAssetLoaded(ismc->GetStaticMesh()->Uuid) && ismc->GetStaticMesh()->Uuid.getUUID() != 0) {
                            mAppInfo->AppAssetManager->LoadAssetData(mAppInfo->AppAssetManager->GetAssetDescriptor(ismc->GetStaticMesh()->Uuid));
                        }
                        // Patrz komentarz analogiczny przy StaticMeshComponent powyżej: mesh dojeżdża
                        // asynchronicznie, dogoń MeshBoundingBox raz, gdy jest już załadowany.
                        if (!ismc->MeshBoundingBoxComputed && ismc->GetStaticMesh()->IsLoaded) {
                            ismc->MeshBoundingBox = Plu::CreateBoundingBoxForStaticMesh(ismc->GetStaticMesh().GetRaw());
                            ismc->MeshBoundingBoxComputed = true;
                        }
                    }

#ifdef PLU_ENGINE_EDITOR_BUILD
                    if (ismc->GetStaticMesh().IsValid()) {
                        RenderUsageStats::GetInstance()->RecordMesh(ismc->GetStaticMesh()->Uuid.getUUID());
                    }
                    if (ismc->GetMaterial().IsValid()) {
                        TUsePointer<MaterialInfo> material = ismc->GetMaterial();
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

                    const DynamicArray<Matrix4>* instanceMatrices = ismc->GetInstanceWorldMatrices();
                    const Vec3 localCenter = ismc->MeshBoundingBox.GetCenter();
                    const Vec3 localExtent = ismc->MeshBoundingBox.GetExtent();
                    for (const Matrix4& instanceMatrix : *instanceMatrices) {
                        // Skala per-instancja nie jest przechowywana osobno (tylko finalna macierz),
                        // więc odtwarzamy ją z długości kolumn bazowych — odpowiednik GetWorldScale()
                        // dla zwykłych komponentów, poprawne dopóki nie ma shearu (translate*rotate*scale).
                        const Vec3 approxScale = Vec3(glm::length(Vec3(instanceMatrix[0])),
                                                       glm::length(Vec3(instanceMatrix[1])),
                                                       glm::length(Vec3(instanceMatrix[2])));
                        const Vec3 boundsCenter = Vec3(instanceMatrix * Vec4(localCenter, 1.0f));
                        const float boundsRadius = glm::length(localExtent * approxScale);
                        AddInstanceToBatch(meshUuid, materialUuid, castsShadow, instanceMatrix, boundsCenter, boundsRadius);
                    }
                }
            }
        }

        // Prefix-sum InstanceOffset + sortowanie (małej) tablicy StaticMeshBatches po (MeshUUID,
        // MaterialUUID) — mStaticMeshRenderables to GameHashMap, kolejność iteracji niestabilna
        // między klatkami, bez tego batche skaczą w kolejności i profilowanie jest nieczytelne.
        // Batchy jest dziesiątki, więc słabość QuickSorta na identycznych kluczach tu nie gryzie.
        const UInt32 batchCount = snapshot->StaticMeshBatches.Size();
        if (batchCount > 0) {
            DynamicArray<UInt32> sortOrder;
            sortOrder.Reserve(batchCount);
            for (UInt32 i = 0; i < batchCount; i++) sortOrder.PushBack(i);

            sortOrder.Sort([&](UInt32 a, UInt32 b) {
                const StaticMeshBatch& ba = snapshot->StaticMeshBatches[a];
                const StaticMeshBatch& bb = snapshot->StaticMeshBatches[b];
                if (ba.MeshUUID != bb.MeshUUID) return ba.MeshUUID.getUUID() < bb.MeshUUID.getUUID();
                return ba.MaterialUUID.getUUID() < bb.MaterialUUID.getUUID();
            });

            DynamicArray<StaticMeshBatch> sortedBatches;
            sortedBatches.Reserve(batchCount);
            DynamicArray<UInt32> oldToNew;
            oldToNew.Resize(batchCount);
            UInt32 runningOffset = 0;
            for (UInt32 newIndex = 0; newIndex < batchCount; newIndex++) {
                const UInt32 oldIndex = sortOrder[newIndex];
                StaticMeshBatch batch = snapshot->StaticMeshBatches[oldIndex];
                batch.InstanceOffset = runningOffset;
                runningOffset += batch.TotalCount;
                oldToNew[oldIndex] = newIndex;
                sortedBatches.PushBack(batch);
            }
            snapshot->StaticMeshBatches = std::move(sortedBatches);

            // Scatter scratcha do StaticInstanceData/StaticInstanceBounds, fixując batchIndex
            // przez oldToNew. Faza 1: brak partycji visible/hidden (VisibleCount == TotalCount),
            // więc każda instancja idzie po prostu na kolejny wolny slot swojego batcha.
            snapshot->StaticInstanceData.Resize(runningOffset);
            snapshot->StaticInstanceBounds.Resize(runningOffset);
            DynamicArray<UInt32> runningCount;
            runningCount.Resize(batchCount);
            for (const StaticMeshBatchScratchEntry& entry : mBatchScratch) {
                const UInt32 newBatchIndex = oldToNew[entry.BatchIndex];
                const StaticMeshBatch& batch = snapshot->StaticMeshBatches[newBatchIndex];
                const UInt32 slot = batch.InstanceOffset + runningCount[newBatchIndex]++;
                snapshot->StaticInstanceData[slot] = entry.Instance;
                snapshot->StaticInstanceBounds[slot] = entry.Bounds;
            }
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

                worldComponent->InvalidateGlobalTransforms();

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

                    worldComponent->InsertGlobalTransform(node->NodeName, globalTransforms[node->NodeName]);

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
