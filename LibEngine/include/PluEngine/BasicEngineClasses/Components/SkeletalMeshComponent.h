//
// Created by Plutex on 7/6/26.
//

#ifndef PLUENGINE_SKELETALMESHCOMPONENT_H
#define PLUENGINE_SKELETALMESHCOMPONENT_H
#include "PluEngine/GameObject/WorldComponent.h"
#include "SkeletalMeshComponent.generated.h"
#include "PluEngine/Renderer/RenderingInterfaces.h"
#include "PluEngine/AssetTypes/SkeletalMesh/SkeletalMesh.h"
#include "HashMap/HashMapV2.h"

namespace Plu
{
	struct SkeletonAttachPoint;
	struct Animation;
	struct SkeletalMesh;
	PLU_CLASS(PyExport)
	class PLU_API SkeletalMeshComponent : public WorldComponent
	{
		REFLECTION_BODY_SKELETALMESHCOMPONENT()
	private:
		DynamicArray<TOwningPointer<SkeletonNode>> mNodes;
		// Edge detector so playback (re)starts from the scrubbed AnimationFrameToShow.
		bool mWasPlaying = false;

		GameHashMap<String, TUsePointer<SkeletonNode>> mNodesCache;
		GameHashMap<String, Matrix4> mLastBoneGlobalTransforms;
	public:
		SkeletalMeshComponent() = default;
		~SkeletalMeshComponent() override = default;

		PLU_PROPERTY(Setter=SetSkeletalMesh, Getter=GetSkeletalMesh)
		TUsePointer<SkeletalMesh> SkeletalMeshToDisplay;

		PLU_PROPERTY()
		TUsePointer<MaterialInfo> Material;

		PLU_PROPERTY()
		TUsePointer<Animation> AnimationToShow;

		PLU_PROPERTY()
		int AnimationFrameToShow = 0;

		PLU_PROPERTY(PyExport)
		bool IsPlaying = false;

		PLU_PROPERTY(PyExport)
		bool LoopAnimation = true;

		// Playback head in animation ticks; runtime-only (advanced by OnUpdate during play,
		// read by RenderSnapshotBuilder the same frame — main thread both ways).
		float AnimationTimeTicks = 0.0f;

		// Per-bone temporary pose overrides, keyed by node name. Each is a parent-space delta
		// pre-multiplied onto the bind/animated local matrix (localMatrix = override * base), so it
		// can translate, rotate and scale a bone and drag its subtree. Applied identically by
		// RenderSnapshotBuilder and the editor bone overlay. Deliberately NOT a PLU_PROPERTY: this
		// is a live posing scratchpad that is never serialized. Empty in normal play (zero overhead).
		GameHashMap<String, Matrix4> BoneLocalOverrides;

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
		PLU_FUNCTION(PyExport)
		Vec3 GetAttachPointLocationInWorld(String attachPointName);
		PLU_FUNCTION(PyExport)
		Vec3 GetAttachPointRotationInWorld(String attachPointName);

		//IDK
		void InvalidateGlobalTransforms();
		void InsertGlobalTransform(const String &node, const Matrix4& globalTransform);
	};
}

#endif //PLUENGINE_SKELETALMESHCOMPONENT_H
