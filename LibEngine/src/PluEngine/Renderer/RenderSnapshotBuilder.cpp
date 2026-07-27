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
#include "PluEngine/AssetTypes/AnimationGraph/AnimationGraph.h"
#include "HashSet/HashSet.h"

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

    // A graph bound to the wrong skeleton is a data/authoring error that would otherwise repeat
    // every frame, so the message is emitted once per (graph, mesh) pair. Main thread only
    // (BuildSnapshotAndPublish), so the static needs no synchronisation. Deliberately never
    // cleared: reassigning the offending field stops the call site from reaching here at all.
    void WarnGraphSkeletonMismatch(const Plu::AnimationGraph& graph, const Plu::SkeletalMesh& mesh)
    {
        static Plu::HashSet<UInt64> warned;
        const UInt64 key = HashBatchKey(graph.Uuid.getUUID(), mesh.Uuid.getUUID(), false);
        if (!warned.Insert(key)) return; // already reported

        PLU_CORE_WARN("AnimationGraph {} targets skeleton {} but SkeletalMesh {} uses skeleton {} — "
                      "graph ignored for this mesh.",
                      graph.Uuid.getUUID(),
                      graph.TargetSkeleton ? graph.TargetSkeleton->Uuid.getUUID() : 0,
                      mesh.Uuid.getUUID(),
                      mesh.MeshSkeleton ? mesh.MeshSkeleton->Uuid.getUUID() : 0);
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

// Frame phase 1: every skeletal pose for this frame, plus the GameObjects riding those poses.
//
// Runs before ANY renderable is collected into the snapshot, and that ordering is the whole
// point. A GameObject on an attach point derives its transform from
// SkeletalMeshComponent::PosedGlobalTransforms, which only exists once the pose has been
// evaluated — resolving it from GameObject::TickObject (where it used to live) therefore
// always read the PREVIOUS frame's pose and left every attached object exactly one frame
// behind the bone it rides, most visible during fast movement. Static-mesh batching reads
// those same transforms, so it has to run after this too.
//
// Objects are walked in attachment-depth order, so a chain (weapon on a hand, scope on the
// weapon's own skeletal mesh) resolves parent-first: at depth d every parent pose is already
// final, because it was produced at depth d-1.
void Plu::RenderSnapshotBuilder::EvaluateSkeletalPosesAndAttachments(const TUsePointer<SceneWorld>& sceneWorld, float deltaTime)
{
    PLU_PROFILE_SCOPE("Skeletal Poses And Attachments");

    // Almost every scene is entirely depth 0 (nothing attached) or depth 1 (props on bones), so
    // this stays a single pass in practice; the loop only repeats when chains actually exist.
    UInt32 maxDepth = 0;
    for (const auto& entry : sceneWorld->mGameObjects) {
        if (const GameObject* object = entry.second.GetRaw()) {
            if (object->IsAttachedToSkeletalMesh()) {
                maxDepth = glm::max(maxDepth, GetAttachmentDepth(object));
            }
        }
    }

    // Attachments only move objects while the scene is actually ticking. Outside play the builder
    // still runs every frame, and snapping attached objects onto their bones there would drag them
    // away from their authored transforms behind the user's back — this used to be resolved from
    // GameObject::TickObject, so it was implicitly play-only. Poses below are evaluated either way,
    // since the editor viewport has to render them.
    const bool resolveAttachments = sceneWorld->mIsPlaying;

    for (UInt32 depth = 0; depth <= maxDepth; ++depth) {
        // Depth 0 is by definition unattached, so the resolve pass is pure overhead there.
        if (depth > 0 && resolveAttachments) {
            for (const auto& entry : sceneWorld->mGameObjects) {
                GameObject* object = entry.second.GetRaw();
                if (object && object->IsAttachedToSkeletalMesh() && GetAttachmentDepth(object) == depth) {
                    object->UpdateSkeletalAttachment();
                }
            }
        }

        for (const auto& entry : sceneWorld->mSkeletalMeshRenderables) {
            for (const auto& component : entry.second) {
                if (!component) continue;
                const GameObject* owner = component->GetParentGameObject().GetRaw();
                if (GetAttachmentDepth(owner) != depth) continue;
                EvaluateSkeletalPose(component.GetRaw(), deltaTime);
            }
        }
    }
}

// Number of attach points between `object` and an unattached root. The chain is one pointer
// per object and realistically one or two links long; the counter bound keeps a miswired
// attachment cycle from hanging the frame instead of merely looking wrong.
UInt32 Plu::RenderSnapshotBuilder::GetAttachmentDepth(const GameObject* object)
{
    constexpr UInt32 maxChain = 64;
    UInt32 depth = 0;
    const GameObject* current = object;
    while (current && current->IsAttachedToSkeletalMesh() && depth < maxChain) {
        TUsePointer<SkeletalMeshComponent> parent = current->GetSkeletalAttachmentParent();
        if (!parent) break;
        current = parent->GetParentGameObject().GetRaw();
        ++depth;
    }
    return depth;
}

// Pose for one component: picks the source (AnimGraph or raw Animation), runs the pre-evaluate
// hook, and rebuilds PosedGlobalTransforms + CachedBonePalette unless the pose cache still
// covers this frame. Extracted from the collection loop so it can run in the phase above.
void Plu::RenderSnapshotBuilder::EvaluateSkeletalPose(SkeletalMeshComponent* component, float deltaTime)
{
    if (!component) return;
    TUsePointer<SkeletalMesh> skeletalMesh = component->GetSkeletalMesh();
    // AnimGraph takes priority over the raw AnimationToShow path when assigned
    // (SkeletalMeshComponent keeps both fields — unassigning the graph falls straight
    // back to direct sampling). Either way we need a (source uuid, time key) pair to
    // key the pose cache below.
    TUsePointer<AnimationGraph> animGraph = component->AnimGraph;
    TUsePointer<Animation> animation = component->AnimationToShow;

    // A graph is authored against one skeleton (AnimationGraph::TargetSkeleton) and its
    // poses are indexed by that skeleton's SkeletonPoseLayout. Driving a different mesh
    // with it would silently produce a scrambled pose, so drop the graph here and let
    // the component behave as if none were assigned (AnimationToShow / bind pose).
    if (animGraph && skeletalMesh && !animGraph->IsCompatibleWith(skeletalMesh->MeshSkeleton)) {
        WarnGraphSkeletonMismatch(*animGraph, *skeletalMesh);
        animGraph = nullptr;
    }

    // The AnimBlueprint-EventGraph hook: the graph pulls its own inputs here, one frame
    // fresh. Deliberately OUTSIDE the pose cache check below — the hook writes the very
    // graph variables whose revision is part of that cache key, so running it inside the
    // miss branch would be circular and would drop the hook exactly on the frames a
    // cached pose was about to be reused. It stays cheap even so: AnimGraphVariableStore
    // ::Set bumps the revision only on a real value change, so a hook that re-pushes
    // identical values leaves the pose cache hitting as before.
    if (animGraph) {
        PLU_PROFILE_SCOPE("AnimGraph PreEvaluate");
        component->OnPreEvaluateAnimGraph(deltaTime);
    }

    UInt64 poseSourceUuid = 0;
    double poseTimeKey = 0.0;
    if (animGraph) {
        poseSourceUuid = animGraph->Uuid.getUUID();
        poseTimeKey = static_cast<double>(component->GraphTimeSeconds);
    } else if (animation) {
        // AnimationFrameToShow is a tick index (FramesAmount == duration in ticks);
        // clamp so scrubbing past either end holds the boundary pose. During playback
        // the fractional AnimationTimeTicks head wins, so poses interpolate between frames.
        poseSourceUuid = animation->Uuid.getUUID();
        poseTimeKey = component->IsPlaying
            ? glm::clamp(static_cast<double>(component->AnimationTimeTicks), 0.0, static_cast<double>(animation->FramesAmount))
            : static_cast<double>(glm::clamp(component->AnimationFrameToShow, 0, animation->FramesAmount));
    }

    // Cache pozy (patrz komentarz przy CachedBonePalette w SkeletalMeshComponent.h):
    // poza zależy od (mesh, źródło animacji, czas, overrides, world matrix — patrz
    // CachedPoseWorldMatrix) — więc przy niezmienionym kluczu pomijamy cały traversal
    // szkieletu. Overrides (live posing w edytorze) wymuszają przeliczenie co klatkę;
    // przy pominięciu NIE ruszamy też globalnych transformów komponentu (attach pointy
    // itd.) — trzymają wartości z ostatniego przeliczenia, wciąż aktualne.
    const UInt64 poseMeshUuid = skeletalMesh.IsValid() ? skeletalMesh->Uuid.getUUID() : 0;
    const bool overridesActive = !component->BoneLocalOverrides.IsEmpty();
    // Per-user AnimGraph values (see AnimGraphInstance) — evaluated below only when a
    // graph is assigned, but resolved here so its value revision can enter the cache key.
    AnimGraphInstance* animGraphInstance = animGraph ? component->EnsureAnimGraphInstance() : nullptr;
    const UInt32 graphValueRevision = animGraphInstance ? animGraphInstance->GetVariables().GetValueRevision() : 0;
    // World-space graph nodes fold the component's world matrix into the pose itself
    // (AnimTransformBoneNode), so it joins the cache key alongside (mesh, source, time,
    // graph values) — see the comment on CachedPoseWorldMatrix.
    const Matrix4 poseWorldMatrix = component->GetWorldMatrix();
    const bool poseCacheHit = !overridesActive
        && component->CachedPoseValid
        && component->CachedPoseMeshUuid == poseMeshUuid
        && component->CachedPoseAnimUuid == poseSourceUuid
        && component->CachedPoseTicks == poseTimeKey
        && component->CachedPoseGraphValueRevision == graphValueRevision
        && component->CachedPoseWorldMatrix == poseWorldMatrix;

    if (!poseCacheHit) {
        DynamicArray<std::pair<Matrix4, Matrix4>>& bones = component->CachedBonePalette;
        bones.Clear();

        if (skeletalMesh && skeletalMesh->MeshSkeleton) {
            // Flat evaluation view of the skeleton (built once per asset). The node tree
            // is still the source of truth — this is its derived, index-addressed form.
            const Skeleton& skeleton = *skeletalMesh->MeshSkeleton;
            const SkeletonPoseLayout& layout = skeleton.GetPoseLayout();
            const UInt64 nodeCount = layout.NodeCount();

            Pose& globals = component->PosedGlobalTransforms;
            globals.Resize(nodeCount);

            if (animGraph) {
                AnimEvalContext context;
                context.TimeSeconds = component->GraphTimeSeconds;
                context.Loop = component->LoopAnimation;
                context.TargetSkeleton = skeletalMesh->MeshSkeleton;
                context.Instance = animGraphInstance;
                context.ComponentToWorld = poseWorldMatrix;

                Pose local = animGraph->Evaluate(context);
                if (local.Size() != nodeCount) {
                    // No output node / empty graph: fall back to the bind pose instead
                    // of leaving a mismatched or stale local pose.
                    layout.MakeBindPose(local);
                }

                if (overridesActive) {
                    for (UInt64 i = 0; i < nodeCount; ++i) {
                        if (const Matrix4* ov = component->BoneLocalOverrides.Find(layout.NodeName[i]))
                            local[i] = BoneTransform::FromMatrix(*ov).Compose(local[i]);
                    }
                }

                // Graph output is local (parent-space); one forward pass to skeleton-space.
                layout.ComposeGlobals(local, globals);
            } else {
                // Tracks resolved to node indices once per (animation, skeleton) pair, so
                // the loop below never hashes a bone name.
                const DynamicArray<const AnimationTrack*>* binding =
                    animation ? &animation->GetTrackBinding(skeleton) : nullptr;

                // Sample and compose entirely in Vec3/Quaternion. DFS pre-order guarantees
                // ParentIndex[i] < i, so a single forward pass resolves every global — each
                // parent is already final when its children are reached.
                for (UInt64 i = 0; i < nodeCount; ++i)
                {
                    BoneTransform local = layout.LocalBindTransform[i];

                    if (binding) {
                        if (const AnimationTrack* track = (*binding)[i]) {
                            // Default fallbacks on purpose: a track with an empty channel
                            // means "identity for that component" (FBX pivot-split nodes),
                            // which is what the importer assumes — NOT the bind value.
                            local.Location = track->GetLocationAtTime(poseTimeKey);
                            local.Rotation = track->GetRotationAtTime(poseTimeKey);
                            local.Scale    = track->GetScaleAtTime(poseTimeKey);
                        }
                    }

                    // Temporary editor posing: parent-space delta on this bone (drags
                    // subtree). Name-keyed and matrix-valued on purpose — it is an editor
                    // scratchpad, empty in normal play, so the decompose here never runs
                    // in a game.
                    if (overridesActive) {
                        if (const Matrix4* ov = component->BoneLocalOverrides.Find(layout.NodeName[i]))
                            local = BoneTransform::FromMatrix(*ov).Compose(local);
                    }

                    const Int32 parent = layout.ParentIndex[i];
                    globals[i] = (parent < 0)
                        ? local
                        : globals[static_cast<UInt64>(parent)].Compose(local);
                }
            }

            // The one and only conversion to matrices, at the very end of the chain.
            layout.BuildBonePalette(globals, bones);
        } else {
            component->PosedGlobalTransforms.Clear();
        }

        component->CachedPoseMeshUuid = poseMeshUuid;
        component->CachedPoseAnimUuid = poseSourceUuid;
        component->CachedPoseTicks = poseTimeKey;
        component->CachedPoseGraphValueRevision = graphValueRevision;
        component->CachedPoseWorldMatrix = poseWorldMatrix;
        component->CachedPoseValid = !overridesActive;
    }
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

    // Poses and attachments first, everything that reads a transform after — a prop riding a bone
    // is collected by the static-mesh batching below, so its transform has to be settled by then.
    EvaluateSkeletalPosesAndAttachments(sceneWorld, deltaTime);

    {
        PLU_PROFILE_SCOPE("RenderSnapshotBuilder::BatchStaticMeshes");

        // Bucketing na hashu klucza, NIE sortowanie listy instancji (patrz pułapka QuickSorta
        // przy DynamicArray::Sort - 10000 obiektów o identycznym kluczu to O(n^2)/przepełnienie
        // stosu). mBatchLookup i mBatchScratch to membery buildera, reużywane między klatkami.
        mBatchLookup.Clear();
        mBatchScratch.Clear();
        mEnsuredAssets.Clear();
#ifdef PLU_ENGINE_EDITOR_BUILD
        mFrameMeshUses.Clear();
        mFrameMaterialUses.Clear();
#endif

        // Ensure-load raz per unikalny UUID na klatkę (patrz komentarz przy mEnsuredAssets w .h) —
        // IsAssetLoaded bierze shared_lock, a przy tysiącach komponentów współdzielących kilka
        // assetów wynik jest identyczny dla każdego użycia tego samego UUID.
        auto EnsureAssetDataLoaded = [&](PluUUID uuid) {
            if (uuid.getUUID() == 0) return;
            if (!mEnsuredAssets.Insert(uuid.getUUID())) return; // już sprawdzony w tej klatce
            if (!mAppInfo->AppAssetManager->IsAssetLoaded(uuid)) {
                mAppInfo->AppAssetManager->LoadAssetData(mAppInfo->AppAssetManager->GetAssetDescriptor(uuid));
            }
        };

#ifdef PLU_ENGINE_EDITOR_BUILD
        // Akumulacja liczników "hottest" per unikalny UUID; flush do RenderUsageStats raz,
        // po pętlach batchowania (patrz blok za BatchInstancedStaticMeshes).
        auto BumpFrameUse = [](GameHashMap<UInt64, UInt32>& uses, UInt64 uuid) {
            if (uuid == 0) return;
            if (UInt32* count = uses.Find(uuid)) {
                (*count)++;
            } else {
                uses.Insert(uuid, 1);
            }
        };
#endif

        // Wspólny dodawacz instancji do batcha — używany zarówno przez auto-batching luźnych
        // StaticMeshComponent poniżej, jak i przez InstancedStaticMeshComponent (faza 3): ISMC
        // o 500 instancjach i 500 luźnych komponentów na tym samym meshu+materiale trafiają do
        // tego samego batcha, bo klucz (MeshUUID, MaterialUUID, CastsShadow) nie rozróżnia źródła.
        auto AddInstanceToBatch = [&](PluUUID meshUuid, PluUUID materialUuid, bool castsShadow,
                                       const Matrix4& modelMatrix, const Matrix4& normalMatrix,
                                       const Vec3& boundsCenter, float boundsRadius) {
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
            // Nie licz transpose(inverse()) tutaj — caller podaje macierz normalnych z cache'u
            // (WorldComponent::GetNormalMatrix / ISMC::GetInstanceNormalMatrices), inverse 4x4
            // per instancja per klatka było widoczne w profilu przy tysiącach instancji.
            scratch.Instance.NormalMatrix = normalMatrix;
            scratch.Bounds.BoundsCenter = boundsCenter;
            scratch.Bounds.BoundsRadius = boundsRadius;
        };

        for (auto gameObject : sceneWorld->mStaticMeshRenderables) {
            for (auto worldComponent : gameObject.second) {
                // Hoist: każde GetStaticMesh()/GetMaterial() kopiuje TUsePointer — raz per komponent.
                TUsePointer<StaticMesh> staticMesh = worldComponent->GetStaticMesh();
                TUsePointer<MaterialInfo> material = worldComponent->GetMaterial();
                const PluUUID meshUuid = staticMesh.IsValid() ? staticMesh->Uuid : PluUUID(0);
                const PluUUID materialUuid = material.IsValid() ? material->Uuid : PluUUID(0);
                const bool castsShadow = worldComponent->CastsShadow;

                EnsureAssetDataLoaded(materialUuid);
                EnsureAssetDataLoaded(meshUuid);
                if (staticMesh.IsValid()) {
                    // Mesh dojeżdża asynchronicznie (EnsureAssetDataLoaded powyżej); jeśli SetStaticMesh
                    // nie zdążył policzyć MeshBoundingBox (mesh był jeszcze niezaładowany), dogoń to tutaj —
                    // raz, gdy mesh jest już gotowy (guard: MeshBoundingBoxComputed).
                    if (!worldComponent->MeshBoundingBoxComputed && staticMesh->IsLoaded) {
                        worldComponent->MeshBoundingBox = Plu::CreateBoundingBoxForStaticMesh(staticMesh.GetRaw());
                        worldComponent->MeshBoundingBoxComputed = true;
                    }
                }

#ifdef PLU_ENGINE_EDITOR_BUILD
                BumpFrameUse(mFrameMeshUses, meshUuid.getUUID());
                BumpFrameUse(mFrameMaterialUses, materialUuid.getUUID());
#endif

                // --- Batching instancingu: klucz = (MeshUUID, MaterialUUID, CastsShadow) ---
                const Matrix4 modelMatrix = worldComponent->GetWorldMatrix();
                const Vec3 boundsCenter = Vec3(modelMatrix * Vec4(worldComponent->MeshBoundingBox.GetCenter(), 1.0f));
                const float boundsRadius = glm::length(worldComponent->MeshBoundingBox.GetExtent() * glm::abs(worldComponent->GetWorldScale()));
                AddInstanceToBatch(meshUuid, materialUuid, castsShadow, modelMatrix,
                                   worldComponent->GetNormalMatrix(), boundsCenter, boundsRadius);
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
                    TUsePointer<StaticMesh> staticMesh = ismc->GetStaticMesh();
                    TUsePointer<MaterialInfo> material = ismc->GetMaterial();
                    const PluUUID meshUuid = staticMesh.IsValid() ? staticMesh->Uuid : PluUUID(0);
                    const PluUUID materialUuid = material.IsValid() ? material->Uuid : PluUUID(0);
                    const bool castsShadow = ismc->CastsShadow;

                    EnsureAssetDataLoaded(materialUuid);
                    EnsureAssetDataLoaded(meshUuid);
                    if (staticMesh.IsValid()) {
                        // Patrz komentarz analogiczny przy StaticMeshComponent powyżej: mesh dojeżdża
                        // asynchronicznie, dogoń MeshBoundingBox raz, gdy jest już załadowany.
                        if (!ismc->MeshBoundingBoxComputed && staticMesh->IsLoaded) {
                            ismc->MeshBoundingBox = Plu::CreateBoundingBoxForStaticMesh(staticMesh.GetRaw());
                            ismc->MeshBoundingBoxComputed = true;
                        }
                    }

#ifdef PLU_ENGINE_EDITOR_BUILD
                    // Jak w oryginale: jedno "użycie" mesha/materiału per KOMPONENT, nie per instancja.
                    BumpFrameUse(mFrameMeshUses, meshUuid.getUUID());
                    BumpFrameUse(mFrameMaterialUses, materialUuid.getUUID());
#endif

                    const DynamicArray<Matrix4>* instanceMatrices = ismc->GetInstanceWorldMatrices();
                    const DynamicArray<Matrix4>* instanceNormalMatrices = ismc->GetInstanceNormalMatrices();
                    const Vec3 localCenter = ismc->MeshBoundingBox.GetCenter();
                    const Vec3 localExtent = ismc->MeshBoundingBox.GetExtent();
                    const UInt64 instanceCount = instanceMatrices->Size();
                    for (UInt64 i = 0; i < instanceCount; i++) {
                        const Matrix4& instanceMatrix = (*instanceMatrices)[i];
                        // Skala per-instancja nie jest przechowywana osobno (tylko finalna macierz),
                        // więc odtwarzamy ją z długości kolumn bazowych — odpowiednik GetWorldScale()
                        // dla zwykłych komponentów, poprawne dopóki nie ma shearu (translate*rotate*scale).
                        const Vec3 approxScale = Vec3(glm::length(Vec3(instanceMatrix[0])),
                                                       glm::length(Vec3(instanceMatrix[1])),
                                                       glm::length(Vec3(instanceMatrix[2])));
                        const Vec3 boundsCenter = Vec3(instanceMatrix * Vec4(localCenter, 1.0f));
                        const float boundsRadius = glm::length(localExtent * approxScale);
                        AddInstanceToBatch(meshUuid, materialUuid, castsShadow, instanceMatrix,
                                           (*instanceNormalMatrices)[i], boundsCenter, boundsRadius);
                    }
                }
            }
        }

#ifdef PLU_ENGINE_EDITOR_BUILD
        {
            // Flush liczników "hottest" assetów, zakumulowanych wyżej per unikalny UUID. Tekstury:
            // uniformy sampler2D materiału są identyczne dla wszystkich jego użyć, więc chodzimy po
            // nich RAZ per unikalny materiał (nie per komponent). Mapy cieni (CSM) są ustawiane
            // silnikowo poza materiałem, więc pozostają naturalnie pominięte.
            RenderUsageStats* usageStats = RenderUsageStats::GetInstance();
            for (const auto& meshUse : mFrameMeshUses) {
                usageStats->RecordMesh(meshUse.first, meshUse.second);
            }
            for (const auto& materialUse : mFrameMaterialUses) {
                TUsePointer<MaterialInfo> material = mAppInfo->AppAssetManager->GetAssetDataNoLoad(materialUse.first);
                if (!material) continue;
                const UInt32 paramCount = material->MaterialParameters.Size();
                for (UInt32 u = 0; u < paramCount; u++) {
                    TUsePointer<IShaderUniform> uniform = material->MaterialParameters.At(u);
                    if (!uniform || uniform->ArraySize != 0 || uniform->Type != "sampler2D") continue;
                    ShaderUniform<TUsePointer<TextureInfo>>* texUniform =
                        static_cast<ShaderUniform<TUsePointer<TextureInfo>>*>(uniform.GetRaw());
                    TUsePointer<TextureInfo> texInfo = texUniform->Data;
                    if (texInfo.IsValid()) {
                        usageStats->RecordTexture(texInfo->Uuid.getUUID(), materialUse.second);
                    }
                }
            }
        }
#endif

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
                TUsePointer<SkeletalMesh> skeletalMesh = worldComponent->GetSkeletalMesh();
                TUsePointer<MaterialInfo> material = worldComponent->GetMaterial();

                // The pose itself was built by EvaluateSkeletalPosesAndAttachments, before anything
                // was collected into this snapshot; from here on we only read the result.
                const UInt64 poseMeshUuid = skeletalMesh.IsValid() ? skeletalMesh->Uuid.getUUID() : 0;

                snapshot->SkeletalMeshRenderObjects.EmplaceBack(poseMeshUuid,
                                                                material.IsValid() ? material->Uuid : PluUUID(0),
                                                                worldComponent->GetWorldLocation(),
                                                                glm::quat(glm::radians(worldComponent->GetWorldRotation())),
                                                                worldComponent->GetWorldScale(),
                                                                worldComponent->GetWorldMatrix(),
                                                                worldComponent->CastsShadow,
                                                                &worldComponent->CachedBonePalette);

                if (material.IsValid()) {
                    if (!mAppInfo->AppAssetManager->IsAssetLoaded(material->Uuid) && material->Uuid.getUUID() != 0) {
                        mAppInfo->AppAssetManager->LoadAssetData(mAppInfo->AppAssetManager->GetAssetDescriptor(material->Uuid));
                    }
                }
                if (skeletalMesh.IsValid()) {
                    if (!mAppInfo->AppAssetManager->IsAssetLoaded(skeletalMesh->Uuid) && skeletalMesh->Uuid.getUUID() != 0) {
                        mAppInfo->AppAssetManager->LoadAssetData(mAppInfo->AppAssetManager->GetAssetDescriptor(skeletalMesh->Uuid));
                    }
                }

#ifdef PLU_ENGINE_EDITOR_BUILD
                // Liczniki "hottest" assetów: mesh + tekstury materiału tego renderable'a (analogicznie
                // do static-mesh gałęzi wyżej; mapy cieni CSM są silnikowe i naturalnie pominięte).
                if (skeletalMesh.IsValid()) {
                    RenderUsageStats::GetInstance()->RecordSkeletalMesh(skeletalMesh->Uuid.getUUID());
                }
                if (material.IsValid()) {
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
