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
		TUsePointer<GameObjectComponent> mParentComponent;

		void Cleanup();
	public:
		WorldComponent() = default;
		virtual ~WorldComponent() override = default;

		PLU_FUNCTION()
		[[nodiscard]] TUsePointer<GameObjectComponent> GetParentComponent() const;
		PLU_FUNCTION()
		DynamicArray<TUsePointer<WorldComponent>> GetChildren();
		PLU_FUNCTION()
		void AttachTo(GameObjectComponent* newAttachPoint);

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
	};
}

#endif //PLUENGINE_WORLDCOMPONENT_H
