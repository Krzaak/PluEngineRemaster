//
// Created by Plutex on 1/11/26.
//

#ifndef PLUENGINE_GAMEOBJECT_H
#define PLUENGINE_GAMEOBJECT_H

#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "GameObject.generated.h"
#include "WorldComponent.h"
#include "PluEngine/Reflection/TypeTraits.h"
#include "PluEngine/PluTypes.h"

namespace Plu
{
	class WorldComponent;
	class ShaderProgram;
}

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
		Vec3 mScale = Vec3(1);

		PluUUID mUuid;

		DynamicArray<TOwningPointer<GameObjectComponent>> mComponents;
		DynamicArray<TOwningPointer<WorldComponent>> mWorldComponents;

		TUsePointer<class SceneWorld> mWorld;
		TUsePointer<class EngineObjectManager> mObjectManager;

		friend class SceneWorld;
		void InitGameObject(const TUsePointer<class SceneWorld>& sceneWorld, const TUsePointer<class EngineObjectManager>& objectManager);
	protected:
		TUsePointer<GameObject> This();
		TUsePointer<SceneWorld> GetWorld();
	public:
		GameObject() = default;
		virtual ~GameObject() override;

		virtual void OnSetupComponents() {}
		virtual void OnBeginPlay() {}
		virtual void OnUpdate(float deltaTime) {}
		virtual void OnEndPlay() {}

		void Cleanup();

		TUsePointer<GameObjectComponent> AddComponent(TypeInfo* componentClass);

		DynamicArray<TOwningPointer<GameObjectComponent>>* GetObjectComponents();
		DynamicArray<TOwningPointer<WorldComponent>>* GetObjectWorldComponents();

		[[nodiscard]] Vec3 GetObjectLocation() const;
		[[nodiscard]] Vec3 GetObjectRotation() const;
		[[nodiscard]] Vec3 GetObjectScale() const;

		void SetObjectLocation(const Vec3& location);
		void SetObjectRotation(const Vec3& rotation);
		void SetObjectScale(const Vec3& scale);

		PluUUID& GetObjectUUID();
	};

	template<>
	struct TypeSerializer<GameObject>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			GameObject* obj = static_cast<GameObject *>(dataToSerialize);
			JSON j;
			Vec3 loc = obj->GetObjectLocation();
			j["location"] = TypeSerializer<glm::vec3>::Serialize(&loc);
			Vec3 rot = obj->GetObjectRotation();
			j["rotation"] = TypeSerializer<glm::vec3>::Serialize(&rot);
			Vec3 scl = obj->GetObjectScale();
			j["scale"] = TypeSerializer<glm::vec3>::Serialize(&scl);
			j["uuid"] = TypeSerializer<PluUUID>::Serialize(&obj->GetObjectUUID());
			j["typeName"] = obj->GetClass()->TypeName.CStr();
			j["worldComponents"] = nlohmann::json::array();
			j["components"] = nlohmann::json::array();
			for (const auto& worldComp : *obj->GetObjectWorldComponents()) {
				j["worldComponents"].push_back(TypeSerializer<TypeInfo*>::Serialize(worldComp->GetClass(), worldComp.GetRaw()));
			}
			for (const auto& comp : *obj->GetObjectComponents()) {
				j["components"].push_back(TypeSerializer<TypeInfo*>::Serialize(comp->GetClass(), comp.GetRaw()));
			}
			return j;
		}

		static void Deserialize(DeserializationContext* dc, const nlohmann::json& json, void* outValue)
		{
		}

		static void EditorControl(void* value, const String& name)
		{

		}
	};
}

#endif //PLUENGINE_GAMEOBJECT_H
