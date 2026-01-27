//
// Created by Plutex on 1/3/26.
//

#ifndef PLUENGINE_REFLECTIONBASE_H
#define PLUENGINE_REFLECTIONBASE_H

#include <functional>

#include "imgui.h"
#include "json.hpp"
#include "json_fwd.hpp"
#include "PluSTL_FWD.h"
#include "PluEngine/Core.h"
#include "PluEngine/Log.h"

namespace Plu
{
	struct ApplicationInfo;
	class EngineObjectManager;
}

namespace Plu
{
	class IScenesManager;
	class IAssetManager;
	class IShaderManager;
	struct TypeInfo;

	struct DeserializationContext
	{
		TUsePointer<IShaderManager> shaderManager;
		TUsePointer<IAssetManager> assetManager;
		TUsePointer<IScenesManager> scenesManager;
	};

	template<typename T> struct TypeSerializer
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return {"NO TYPE SERIALIZATION"};
		}

		static void Deserialize(DeserializationContext* deserializationContext, const nlohmann::json& json, void* outValue)
		{
			PLU_CORE_ERROR("NO TYPE DESERIALIZATION!");
		}

		static void EditorControl(void* value, const String& name)
		{
			ImGui::Text("Unsupported type!");
		}
	};

	enum class PropertyType
	{
		Int,
		Float,
		Bool,
		String,
		StringW,
		UserPointer,
		DynamicArray,
		Unknown
	};

	using SerializeFn   = nlohmann::json (*)(void* ptr);
	using DeserializeFn = void (*)(DeserializationContext* deserializationContext, const nlohmann::json& j, void* outValue);
	using EditorFn      = void (*)(void* ptr, const String& name);

	struct PLU_API PropertyInfo
	{
		String PropertyName;
		UInt64 PropertyOffset;
		UInt64 PropertySize;
		PropertyType PropertyType;
		String PropertyTypeName;

		SerializeFn   SerializePtr;
		DeserializeFn DeserializePtr;
		EditorFn      EditorControlPtr;

		bool IsPersistent = true;
		bool IsVisibleInEditor = false;

		TypeInfo* UuidForClass = nullptr;

		void* GetPtr(void* objectInstance) const;
	};

	enum class TypeType
	{
		CLASS,
		STRUCT,
		ENUM,
		INTERFACE,
		UNKNOWN
	};

	struct PLU_API TypeInfo
	{
		using ConstructorFunc = std::function<void*()>;
		using SerializeJsonFunc = std::function<nlohmann::json(void*)>;
		using DeSerializeJsonFunc = std::function<void*(DeserializationContext*, nlohmann::json&)>;

		UInt64 TypeSize;
		String TypeName;
		TypeType Type;
		TypeInfo* BaseType = nullptr; //We only support inheritance through single parent. I'm lazy :)
		PropertyInfo* TypeUuidProp = nullptr;
		[[nodiscard]] PropertyInfo* GetTypeUuidProp() const;

		DynamicArray<PropertyInfo*> Properties;
		ConstructorFunc Constructor = [this]()->void* {
			String info = "Class is probably Abstract! No Constructor for ";
			info += TypeName;
			info += "! Try removing the Abstract property in PLU_CLASS().";
			PLU_CORE_ASSERT(false, info.CStr())
			return nullptr;
		};
		void AddProperty(PropertyInfo* propertyInfo);
		[[nodiscard]] void* Construct() const;
		[[nodiscard]] PropertyInfo* FindProperty(const String& propertyName);

		[[nodiscard]] bool IsChildOf(TypeInfo* potentialParent); //Base type is ?
		[[nodiscard]] bool IsDerivedOf(TypeInfo* potentialParent); //Can scan more types
		[[nodiscard]] bool IsDerivedOfOrSame(TypeInfo* potentialParent); //Can scan more types

		SerializeJsonFunc SerializeToJson = [this](void* obj)->nlohmann::json {
			String info = "No JSON serialization for type: ";
			info += TypeName;
			info += "! Try rerunning ReflectionGenerator.py";
			PLU_CORE_ASSERT(false, info.CStr())
			return {};
		};
		DeSerializeJsonFunc DeserializeFromJson = [this](DeserializationContext* dc, nlohmann::json& j)->void* {
			String info = "No JSON deserialization for type: ";
			info += TypeName;
			info += "! Try rerunning ReflectionGenerator.py";
			PLU_CORE_ASSERT(false, info.CStr())
			return {};
		};
		nlohmann::json SerializeToJSON(void* obj) const;
		void* DeSerializeFromJSON(DeserializationContext* dc, const nlohmann::json &j) const;

		TypeInfo(UInt64 size, String typeName, TypeType type);
		~TypeInfo();
	};

	class PLU_API TypeRegistry
	{
		GameHashMap<String, TypeInfo*> mTypeMap;
		ApplicationInfo* mApplicationInfo;
		friend class Application;
	public:
		TUsePointer<EngineObjectManager> GetObjectManager();
		TUsePointer<IAssetManager> GetAssetManager();
		static TypeRegistry* GetInstance();
		void AddType(TypeInfo* typeInfo);
		TypeInfo* GetTypeOfName(const String& typeName);
		GameHashMap<String, TypeInfo*>* GetTypeMap();
	};
}

#endif //PLUENGINE_REFLECTIONBASE_H