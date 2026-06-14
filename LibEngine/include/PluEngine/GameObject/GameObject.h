//
// Created by Plutex on 1/11/26.
//

#ifndef PLUENGINE_GAMEOBJECT_H
#define PLUENGINE_GAMEOBJECT_H

#include "PluEngine/Core.h"
#include "PluEngine/PluTypes.h"
#include "PluEngine/Reflection/ClassPointer.h"
#include "PluEngine/GameObject/GameObjectComponent.h"
#include "GameObject.generated.h"
#include "WorldComponent.h"
#include "PluEngine/Reflection/TypeTraits.h"

namespace Plu
{
	class PhysicsCompoundShape;
	class PhysicsBody;
	class GameObjectComponent;
	class InputHandler;
	class WorldComponent;
	class ShaderProgram;
}

namespace Plu
{
	PLU_CLASS(PyExport, PyDerive)
	class PLU_API GameObject : public EngineObject
	{
		REFLECTION_BODY_GAMEOBJECT()
	private:
		Vec3 mLocation = Vec3(0);
		Vec3 mRotation = Vec3(0);
		Vec3 mScale = Vec3(1);

		TOwningPointer<PhysicsCompoundShape> mCompoundShape;
		EngineObjectHandle mPhysicsBodyHandle;

		Matrix4 mWorldMatrix = glm::identity<Matrix4>();
		bool mRegenerateWorldMatrix = true;

		PluUUID mUuid;

		DynamicArray<TOwningPointer<GameObjectComponent>> mComponents;
		DynamicArray<TOwningPointer<WorldComponent>> mWorldComponents;
		bool mRedoWorldComponentList = true;

		DynamicArray<TUsePointer<WorldComponent>> mCachedWorldComponents;

		TUsePointer<class SceneWorld> mWorld;
		TUsePointer<class EngineObjectManager> mObjectManager;

		friend class GameObjectComponent;
		friend class WorldComponent;
		friend class SceneWorld;
		friend class PhysicsWorld;

		void InitGameObject(const TUsePointer<class SceneWorld>& sceneWorld, const TUsePointer<class EngineObjectManager>& objectManager);
		void OnAttachComponent(const TOwningPointer<WorldComponent>& component, const TUsePointer<WorldComponent>& attachPoint);
		void OnDetachComponent(const TOwningPointer<WorldComponent>& component);
		void SyncFromPhysicsBody(const Vec3& worldLocation, const Vec3& worldRotationDeg);
	protected:
		TUsePointer<GameObject> This();
		TUsePointer<SceneWorld> GetWorld();
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

		PLU_FUNCTION(PyOverride)
		virtual void OnOverlapBegin(GameObjectComponent* component) {}

		PLU_FUNCTION(PyOverride)
		virtual void OnOverlapEnd(GameObjectComponent* component) {}

		PLU_FUNCTION(PyNotCallable)
		void Cleanup();

		void TickObject(float deltaTime);

		PLU_FUNCTION()
		TUsePointer<GameObjectComponent> AddComponent(TClassPointer<GameObjectComponent> componentClass, String componentName);
		void RegisterComponent(EngineObjectHandle component);

		PLU_FUNCTION()
		DynamicArray<TOwningPointer<GameObjectComponent>>* GetObjectComponents();

		PLU_FUNCTION()
		DynamicArray<TUsePointer<WorldComponent>>* GetObjectWorldComponents(bool force = false);
		PLU_FUNCTION()
		DynamicArray<TUsePointer<WorldComponent>> GetDirectlyAttachedWorldComponents();

		PLU_FUNCTION()
		TUsePointer<GameObjectComponent> GetComponentByClass(const TClassPointer<GameObjectComponent>& componentClass);

		PLU_FUNCTION()
		[[nodiscard]] Vec3 GetObjectLocation() const;
		PLU_FUNCTION()
		[[nodiscard]] Vec3 GetObjectRotation() const;
		PLU_FUNCTION()
		[[nodiscard]] Vec3 GetObjectScale() const;

		[[nodiscard]] Matrix4 GetObjectWorldMatrix();

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

		PLU_FUNCTION()
		TUsePointer<PhysicsBody> GetPhysicsBody();

		virtual InputHandler* GetInputHandler() {return nullptr;};
	};

	template<>
	struct TypeSerializer<GameObject>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			GameObject* obj = static_cast<GameObject *>(dataToSerialize);
			JSON j = TypeSerializer<TypeInfo *>::Serialize(obj->GetClass(), obj);
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
			for (const auto& worldComp : obj->GetDirectlyAttachedWorldComponents()) {
				//TypeSerializer<TypeInfo*>::Serialize(worldComp->GetClass(), worldComp.GetRaw())
				j["worldComponents"].push_back(worldComp->Serialize());
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
