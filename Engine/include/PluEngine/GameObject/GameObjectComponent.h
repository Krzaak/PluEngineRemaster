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
	public:
		GameObjectComponent() = default;
		virtual ~GameObjectComponent() override = default;

		TUsePointer<GameObject> GetParentGameObject();
	};
}

#endif //PLUENGINE_GAMEOBJECTCOMPONENT_H
