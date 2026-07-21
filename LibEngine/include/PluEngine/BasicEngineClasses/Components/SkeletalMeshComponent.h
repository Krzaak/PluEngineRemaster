//
// Created by Plutex on 7/6/26.
//

#ifndef PLUENGINE_SKELETALMESHCOMPONENT_H
#define PLUENGINE_SKELETALMESHCOMPONENT_H
#include "PluEngine/GameObject/WorldComponent.h"
#include "SkeletalMeshComponent.generated.h"
#include "PluEngine/Renderer/RenderingInterfaces.h"
#include "PluEngine/AssetTypes/SkeletalMesh/SkeletalMesh.h"
#include "PluEngine/Animation/BoneTransform.h"
#include "HashMap/HashMapV2.h"
#include <utility>

namespace Plu
{
	struct SkeletonAttachPoint;
	struct Animation;
	struct SkeletalMesh;
	struct AnimationGraph;
	PLU_CLASS(PyExport)
	class PLU_API SkeletalMeshComponent : public WorldComponent
	{
		REFLECTION_BODY_SKELETALMESHCOMPONENT()
	private:
		DynamicArray<TOwningPointer<SkeletonNode>> mNodes;
		// Edge detector so playback (re)starts from the scrubbed AnimationFrameToShow.
		bool mWasPlaying = false;

		// Attach point name -> node index in the skeleton's SkeletonPoseLayout, resolved on first
		// use so repeated attach point queries cost no name lookup. Cleared with the mesh.
		GameHashMap<String, Int32> mAttachPointNodeCache;
	public:
		SkeletalMeshComponent() = default;
		~SkeletalMeshComponent() override = default;

		PLU_PROPERTY(Setter=SetSkeletalMesh, Getter=GetSkeletalMesh)
		TUsePointer<SkeletalMesh> SkeletalMeshToDisplay;

		PLU_PROPERTY()
		TUsePointer<MaterialInfo> Material;

		// Raw (graph-less) animation: sampled directly with no blending/state-machine logic.
		// Ignored while AnimGraph is assigned — the graph takes priority — but never cleared, so
		// unassigning AnimGraph falls straight back to this path.
		PLU_PROPERTY()
		TUsePointer<Animation> AnimationToShow;

		PLU_PROPERTY()
		int AnimationFrameToShow = 0;

		// Drives the pose instead of AnimationToShow when assigned (checked first by
		// RenderSnapshotBuilder). Null by default — components that never touch the graph system
		// pay nothing extra and behave exactly as before.
		PLU_PROPERTY()
		TUsePointer<AnimationGraph> AnimGraph;

		PLU_PROPERTY(PyExport)
		bool IsPlaying = false;

		PLU_PROPERTY(PyExport)
		bool LoopAnimation = true;

		// Playback head in animation ticks; runtime-only (advanced by OnUpdate during play,
		// read by RenderSnapshotBuilder the same frame — main thread both ways).
		float AnimationTimeTicks = 0.0f;

		// Playback head in seconds for AnimGraph evaluation (AnimEvalContext::TimeSeconds is
		// seconds, not ticks — each AnimSampleNode converts using its own animation's FPS).
		// Separate from AnimationTimeTicks because the graph has no single FPS of its own.
		// Runtime-only, advanced by OnUpdate while IsPlaying and AnimGraph is assigned.
		float GraphTimeSeconds = 0.0f;

		// Per-bone temporary pose overrides, keyed by node name. Each is a parent-space delta
		// pre-multiplied onto the bind/animated local matrix (localMatrix = override * base), so it
		// can translate, rotate and scale a bone and drag its subtree. Applied identically by
		// RenderSnapshotBuilder and the editor bone overlay. Deliberately NOT a PLU_PROPERTY: this
		// is a live posing scratchpad that is never serialized. Empty in normal play (zero overhead).
		GameHashMap<String, Matrix4> BoneLocalOverrides;

		// Cache palety kości (pary OffsetMatrix / global transform) z ostatniego builda snapshotu.
		// Poza jest funkcją (mesh, animacja, tick, overrides) — NIE transformu komponentu — więc gdy
		// klucz się nie zmienił, RenderSnapshotBuilder pomija cały traversal szkieletu (mapy stringowe,
		// Tracks.Find i dynamic_cast per węzeł) i reużywa tej tablicy. Aktywne BoneLocalOverrides
		// wyłączają cache (CachedPoseValid zostaje false), więc live posing zawsze przelicza.
		// Kod mutujący Animation w miejscu (te same UUID i tick, inne klucze) musi zrzucić
		// CachedPoseValid ręcznie. Producent: RenderSnapshotBuilder::BuildSnapshotAndPublish.
		DynamicArray<std::pair<Matrix4, Matrix4>> CachedBonePalette;

		// Skeleton-space (root-relative) pose per node, indexed by SkeletonPoseLayout node index —
		// i.e. every node, not just bones (CachedBonePalette holds the bones-only, offset-multiplied
		// matrix form the skinning shader wants). Kept as BoneTransform so nothing on the per-frame
		// path builds a matrix; the attach point queries convert the single node they need.
		// Empty until the first pose evaluation; survives a pose cache hit, since a skipped
		// evaluation means the values are still current.
		// Producent: RenderSnapshotBuilder::BuildSnapshotAndPublish.
		Pose PosedGlobalTransforms;
		UInt64 CachedPoseMeshUuid = 0;
		UInt64 CachedPoseAnimUuid = 0;
		double CachedPoseTicks = -1.0;
		bool CachedPoseValid = false;

		void OnUpdate(float deltaTime) override;

		BoundingBox MeshBoundingBox;

		PLU_PROPERTY(PyExport)
		bool CastsShadow = true;

		PLU_FUNCTION(PyExport)
		TUsePointer<SkeletalMesh> GetSkeletalMesh();
		PLU_FUNCTION(PyExport)
		void SetSkeletalMesh(TUsePointer<SkeletalMesh> skeletalMesh);

		PLU_FUNCTION(PyExport)
		TUsePointer<MaterialInfo> GetMaterial();
		PLU_FUNCTION(PyExport)
		void SetMaterial(TUsePointer<MaterialInfo> material);

		//Rendering
		Matrix4 GetRenderMatrix();
		DynamicArray<TOwningPointer<SkeletonNode>>* GetNodes();

		//AttachPoints
		// Full world frame of an attach point: componentWorld * parentNodeGlobal * attachPointLocal.
		// False when the mesh, the attach point, its parent node or a posed transform for that node
		// is missing (i.e. before the first snapshot build). Prefer this over the location/rotation
		// pair below when the whole frame is wanted — it keeps the bone's basis exactly.
		bool TryGetAttachPointWorldMatrix(const String& attachPointName, Matrix4& outMatrix);

		PLU_FUNCTION(PyExport)
		Vec3 GetAttachPointLocationInWorld(String attachPointName);
		PLU_FUNCTION(PyExport)
		Vec3 GetAttachPointRotationInWorld(String attachPointName);

	};
}

#endif //PLUENGINE_SKELETALMESHCOMPONENT_H
