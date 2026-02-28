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
#include "PluEngine/GameCore/GameLocalPlayer.h"

namespace Plu
{
	class InputHandler;
	class WorldComponent;
	class ShaderProgram;
}

namespace Plu
{
	class GameObjectComponent;

	PLU_CLASS(PyExport, PyDerive)
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

		friend TUsePointer<EngineObjectManager> GameObjectComponent::GetObjectManagerFromParent();
	public:
		GameObject() = default;
		virtual ~GameObject() override;

		PLU_FUNCTION(PyOverride)
		virtual void OnSetupComponents() {}

		PLU_FUNCTION(PyOverride)
		virtual void OnBeginPlay() {}

		PLU_FUNCTION(PyOverride)
		virtual void OnUpdate(float deltaTime) {}

		PLU_FUNCTION(PyOverride)
		virtual void OnEndPlay() {}

		PLU_FUNCTION(PyNotCallable)
		void Cleanup();

		void TickObject(float deltaTime);

		PLU_FUNCTION()
		TUsePointer<GameObjectComponent> AddComponent(TClassPointer<GameObjectComponent> componentClass, String componentName);

		PLU_FUNCTION()
		DynamicArray<TOwningPointer<GameObjectComponent>>* GetObjectComponents();

		PLU_FUNCTION()
		DynamicArray<TOwningPointer<WorldComponent>>* GetObjectWorldComponents();

		PLU_FUNCTION()
		TUsePointer<GameObjectComponent> GetActivatedComponentByClass(const TClassPointer<GameObjectComponent>& componentClass);

		PLU_FUNCTION()
		[[nodiscard]] Vec3 GetObjectLocation() const;
		PLU_FUNCTION()
		[[nodiscard]] Vec3 GetObjectRotation() const;
		PLU_FUNCTION()
		[[nodiscard]] Vec3 GetObjectScale() const;

		PLU_FUNCTION()
		void SetObjectLocation(const Vec3& location);
		PLU_FUNCTION()
		void SetObjectRotation(const Vec3& rotation);
		PLU_FUNCTION()
		void SetObjectScale(const Vec3& scale);

		PLU_FUNCTION()
		[[nodiscard]] Vec3 GetObjectForwardVector() const;
		PLU_FUNCTION()
		[[nodiscard]] Vec3 GetObjectRightVector() const;
		PLU_FUNCTION()
		[[nodiscard]] Vec3 GetObjectUpVector() const;

		PLU_FUNCTION()
		PluUUID& GetObjectUUID();

		virtual InputHandler* GetInputHandler() {return nullptr;};
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
