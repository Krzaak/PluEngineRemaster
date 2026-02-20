//
// Created by Plutex on 1/18/26.
//

#ifndef PLUENGINE_GAMEOBJECTCOMPONENT_H
#define PLUENGINE_GAMEOBJECTCOMPONENT_H
#include "PluEngine/Objects/EngineObject.h"
#include "GameObjectComponent.generated.h"

namespace Plu
{
	class GameObject;
	PLU_CLASS(Abstract)
	class PLU_API GameObjectComponent : public EngineObject
	{
		REFLECTION_BODY_GAMEOBJECTCOMPONENT()
	private:
		TUsePointer<GameObject> mParentObject;

		friend class GameObject;
		void SetParentGameObject(TUsePointer<GameObject> newParent);

		bool mIsActivated = true;
	public:
		GameObjectComponent() = default;
		virtual ~GameObjectComponent() override = default;

		virtual void OnBeginPlay() {}
		virtual void OnUpdate(float deltaTime) {}
		virtual void OnEndPlay() {}

		[[nodiscard]] bool IsActivated() const;
		void Activate();
		void Deactivate();

		TUsePointer<GameObject> GetParentGameObject();
	};
}

#endif //PLUENGINE_GAMEOBJECTCOMPONENT_H
