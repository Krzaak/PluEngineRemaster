//
// Created by Plutex on 1/18/26.
//

#ifndef PLUENGINE_WORLDCOMPONENT_H
#define PLUENGINE_WORLDCOMPONENT_H
#include "GameObjectComponent.h"
#include "PluEngine/PluTypes.h"
#include "WorldComponent.generated.h"

namespace Plu
{
	PLU_CLASS(PyExport, PyDerive)
	class PLU_API WorldComponent : public GameObjectComponent
	{
		REFLECTION_BODY_WORLDCOMPONENT()
	private:
		friend class GameObject;
		DynamicArray<TOwningPointer<WorldComponent>> mWorldComponents;
		TUsePointer<WorldComponent> mParentComponent;

		void Cleanup();

		Vec3 mRelativeLocation = Vec3(0.0f);
		Vec3 mRelativeRotation = Vec3(0.0f);
		Vec3 mRelativeScale = Vec3(1.0f);

		Matrix4 mWorldMatrix;
		bool mRegenerateWorldMatrix = true;
		// Cache transpose(inverse(world)) — liczone leniwie w GetNormalMatrix(), unieważniane
		// razem z world matrix. inverse() 4x4 per komponent per klatka jest zbyt drogie przy
		// tysiącach komponentów (RenderSnapshotBuilder pobiera to co klatkę do InstanceGPUData).
		Matrix4 mNormalMatrix;
		bool mRegenerateNormalMatrix = true;
		void MarkWorldMatrixForRegeneration();
		// translate(loc) * rotate(rot) * scale(scale) — this component's own transform, relative to
		// whatever it is attached to.
		Matrix4 BuildLocalMatrix();
	protected:
		// Called after any of the relative transform setters changed a value. Components that
		// contribute collision geometry override this to keep the physics body in sync — sub-shape
		// offsets are baked into the compound shape when the body is built, so a moved component is
		// invisible to physics until the body is rebuilt.
		virtual void OnRelativeTransformChanged() {}
		// Queues a rebuild of the owning object's physics body. No-op outside of play, or when the
		// object has no body yet.
		void MarkOwnerCollisionDirty();
	public:
		WorldComponent() = default;
		virtual ~WorldComponent() override = default;

		PLU_FUNCTION()
		[[nodiscard]] TUsePointer<GameObjectComponent> GetParentComponent() const;
		PLU_FUNCTION()
		DynamicArray<TUsePointer<WorldComponent>> GetChildren();
		PLU_FUNCTION()
		void AttachTo(GameObjectComponent* newAttachPoint);

		Matrix4 GetWorldMatrix();
		Matrix4 GetNormalMatrix();

		// The full chain of relative transforms up to (but excluding) the owning game object, i.e.
		// this component's transform in object space. Differs from BuildLocalMatrix() only for
		// components attached to another component via AttachTo, whose relative transform is
		// parent-component-relative. Not cached — used at physics body build time, not per frame.
		Matrix4 GetMatrixRelativeToGameObject();

		PLU_FUNCTION()
		Vec3 GetRelativeLocation();
		PLU_FUNCTION()
		Vec3 GetRelativeRotation();
		PLU_FUNCTION()
		Vec3 GetRelativeScale();

		PLU_FUNCTION()
		void SetRelativeLocation(Vec3 newLoc);
		PLU_FUNCTION()
		void SetRelativeRotation(Vec3 newRot);
		PLU_FUNCTION()
		void SetRelativeScale(Vec3 newScale);

		PLU_FUNCTION()
		Vec3 GetWorldLocation();
		PLU_FUNCTION()
		Vec3 GetWorldRotation();
		PLU_FUNCTION()
		Vec3 GetWorldScale();

		PLU_FUNCTION()
		void SetWorldLocation(Vec3 newLoc);
		PLU_FUNCTION()
		void SetWorldRotation(Vec3 newRot);
		PLU_FUNCTION()
		void SetWorldScale(Vec3 newScale);

		JSON Serialize();
	};
}

#endif //PLUENGINE_WORLDCOMPONENT_H
