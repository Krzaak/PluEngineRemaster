//
// Created by Plutex on 2026-01-21.
//

#ifndef PLUENGINE_TYPETRAITS_H
#define PLUENGINE_TYPETRAITS_H

#include "ReflectionBase.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

namespace Plu
{
	template <>
	struct TypeSerializer<int>
	{
		static nlohmann::json Serialize(void* dataToSerialize) { return {*static_cast<int*>(dataToSerialize)}; }
		static void Deserialize(DeserializationContext* deserializationContext, const nlohmann::json& json, void* outValue) { *static_cast<int*>(outValue) = json.get<int>(); }
		static void EditorControl(void* value) { ImGui::DragInt("##", static_cast<int*>(value)); }
	};

	template <>
	struct TypeSerializer<bool>
	{
		static nlohmann::json Serialize(void* dataToSerialize) { return {*static_cast<bool*>(dataToSerialize) ? "true" : "false"}; }
		static void Deserialize(DeserializationContext* deserializationContext, const nlohmann::json& json, void* outValue) { *static_cast<bool *>(outValue) = json.get<bool>(); }
		static void EditorControl(void* value) { ImGui::Checkbox("##", static_cast<bool*>(value)); }
	};

	template <>
	struct TypeSerializer<Path>
	{
		static nlohmann::json Serialize(void* dataToSerialize) { return {static_cast<Path*>(dataToSerialize)->CStr()}; }
		static void Deserialize(DeserializationContext* deserializationContext, const nlohmann::json& json, void* outValue) { *static_cast<Path*>(outValue) = json.get<std::string>().c_str(); }
		static void EditorControl(void* value)
		{
			std::string str = std::string(static_cast<Path *>(value)->CStr());
			ImGui::InputText("##", &str);
		}
	};

	template <>
	struct TypeSerializer<PathW>
	{
		static nlohmann::json Serialize(void* dataToSerialize) { return {static_cast<PathW*>(dataToSerialize)->CStr()}; }
		static void Deserialize(DeserializationContext* deserializationContext, const nlohmann::json& json, void* outValue) { *static_cast<PathW*>(outValue) = json.get<std::wstring>().c_str(); }
		static void EditorControl(void* value, const String& name)
		{
			std::string str = std::string(static_cast<Path *>(value)->CStr());
			ImGui::InputText(name.CStr(), &str);
		}
	};

	template <>
	struct TypeSerializer<TypeInfo*>
	{
		static nlohmann::json Serialize(TypeInfo* dataToSerialize, void* object)
		{
			nlohmann::json json = nlohmann::json::array();
			for (auto property : dataToSerialize->Properties) {
				json.push_back({property->PropertyName.CStr(), property->SerializePtr(property->GetPtr(object))});
			}
			return json;
		}

		static void* Deserialize(DeserializationContext* deserializationContext, const nlohmann::json& json)
		{
			//TODO
			return nullptr;
		}

		static void EditorControl(TypeInfo* value)
		{
			for (auto property : value->Properties) {
				//property->EditorPtr()
				//TODO
			}
		}
	};
}

#endif //PLUENGINE_TYPETRAITS_H