//
// Created by Plutex on 1/11/26.
//

#ifndef PLUENGINE_GAMEOBJECT_H
#define PLUENGINE_GAMEOBJECT_H

#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "GameObject.generated.h"
#include "PluEngine/PluTypes.h"

namespace Plu
{
	class GameObjectComponent;

	PLU_CLASS()
	class PLU_API GameObject : public EngineObject
	{
		REFLECTION_BODY_GAMEOBJECT()
	private:
		Vec3 mLocation = Vec3(0);
		Vec3 mRotation = Vec3(0);
		Vec3 mScale = Vec3(0);

		DynamicArray<TOwningPointer<GameObjectComponent>> mComponents;

		TUsePointer<class SceneWorld> mWorld;
		TUsePointer<class EngineObjectManager> mObjectManager;

		friend class SceneWorld;
		void InitGameObject(TUsePointer<class SceneWorld> sceneWorld, TUsePointer<class EngineObjectManager> objectManager);
	public:
		GameObject() = default;
		virtual ~GameObject() override = default;

		virtual void OnSetupComponents() {}
		virtual void OnBeginPlay() {}
		virtual void OnUpdate(float deltaTime) {}
		virtual void OnEndPlay() {}

		TUsePointer<GameObjectComponent> AddComponent(TypeInfo* componentClass);

		[[nodiscard]] Vec3 GetObjectLocation() const;
		[[nodiscard]] Vec3 GetObjectRotation() const;
		[[nodiscard]] Vec3 GetObjectScale() const;

		void SetObjectLocation(const Vec3& location);
		void SetObjectRotation(const Vec3& rotation);
		void SetObjectScale(const Vec3& scale);

	};
}

#endif //PLUENGINE_GAMEOBJECT_H
