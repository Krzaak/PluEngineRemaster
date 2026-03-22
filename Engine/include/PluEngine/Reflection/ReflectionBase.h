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
#include "pybind11/pybind11.h"

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

	template<typename T>
	struct TypeSerializer
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return {"NO TYPE SERIALIZATION"};
		}

		static void Deserialize(DeserializationContext* deserializationContext, const nlohmann::json& json, void* outValue)
		{
			if constexpr (std::is_enum_v<T>) {
				PLU_CORE_ERROR("NO ENUM DESERIALIZATION!");
			} else {
				PLU_CORE_ERROR("NO TYPE DESERIALIZATION! ({})", T::GetStaticClass()->TypeName.CStr());
			}
		}

		static void EditorControl(void* value, const String& name)
		{
			if constexpr (std::is_enum_v<T>) {
				ImGui::Text("Unsupported enum");
			} else {
				ImGui::Text("Unsupported type %s!", T::GetStaticClass()->TypeName.CStr());
			}
		}
	};

	using SerializeFn   = nlohmann::json (*)(void* ptr);
	using DeserializeFn = void (*)(DeserializationContext* deserializationContext, const nlohmann::json& j, void* outValue);
	using EditorFn      = void (*)(void* ptr, const String& name);

	struct PLU_API PropertyInfo
	{
		String PropertyName;
		UInt64 PropertyOffset;
		UInt64 PropertySize;
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

		bool IsPythonType = false;
		pybind11::type PythonType = pybind11::object();

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

	struct PLU_API EnumValue
	{
		String ValueName;
		UInt64 Value;
	};

	struct PLU_API EnumInfo
	{
		String EnumName;
		DynamicArray<EnumValue*> EnumValues;

		EnumInfo(String enumName) : EnumName(enumName) {};

		void AddValue(String enumName, UInt64 value);
	};

	class PLU_API TypeRegistry
	{
		GameHashMap<String, TypeInfo*> mTypeMap;
		GameHashMap<String, EnumInfo*> mEnumMap;
		ApplicationInfo* mApplicationInfo;
		friend class Application;
	public:
		std::function<void(String, void*, TypeInfo*)> editorAssetTUsePointerControl;
		TUsePointer<EngineObjectManager> GetObjectManager();
		TUsePointer<IAssetManager> GetAssetManager();
		static TypeRegistry* GetInstance();
		void AddType(TypeInfo* typeInfo);
		void AddEnum(EnumInfo* enumInfo);
		TypeInfo* GetTypeOfName(const String& typeName);
		GameHashMap<String, TypeInfo*>* GetTypeMap();
	};

	PLU_FUNCTION()
	void PLU_API RegisterPluClass(pybind11::type type);

	template<typename T> T FromString(const String& str);
}

#endif //PLUENGINE_REFLECTIONBASE_H