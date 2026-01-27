//
// Created by Plutex on 2026-01-21.
//

#ifndef PLUENGINE_TYPETRAITS_H
#define PLUENGINE_TYPETRAITS_H

#include "ReflectionBase.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "PluEngine/PluUUID.h"
#include "PluEngine/Managers/AssetsManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"

namespace Plu
{
	template <>
	struct TypeSerializer<int>
	{
		static nlohmann::json Serialize(void* dataToSerialize) { return {*static_cast<int*>(dataToSerialize)}; }
		static void Deserialize(DeserializationContext* deserializationContext, const nlohmann::json& json, void* outValue) { *static_cast<int*>(outValue) = json.get<int>(); }
		static void EditorControl(void* value, const String& name) { ImGui::DragInt(name.CStr(), static_cast<int*>(value)); }
	};

	template <>
	struct TypeSerializer<bool>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return {*static_cast<bool*>(dataToSerialize)};
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<bool*>(outValue) = json.get<bool>();
		}

		static void EditorControl(void* value, const String& name)
		{
			ImGui::Checkbox(name.CStr(), static_cast<bool*>(value));
		}
	};

	template <>
	struct TypeSerializer<Int8>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return { *static_cast<Int8*>(dataToSerialize) };
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<Int8*>(outValue) = json.get<Int8>();
		}

		static void EditorControl(void* value, const String& name)
		{
			int v = *static_cast<Int8*>(value);
			if (ImGui::DragInt(name.CStr(), &v, 1, INT8_MIN, INT8_MAX))
				*static_cast<Int8*>(value) = static_cast<Int8>(v);
		}
	};

	template <>
	struct TypeSerializer<Int16>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return { *static_cast<Int16*>(dataToSerialize) };
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<Int16*>(outValue) = json.get<Int16>();
		}

		static void EditorControl(void* value, const String& name)
		{
			int v = *static_cast<Int16*>(value);
			if (ImGui::DragInt(name.CStr(), &v, 1, INT16_MIN, INT16_MAX))
				*static_cast<Int16*>(value) = static_cast<Int16>(v);
		}
	};

	template <>
	struct TypeSerializer<Int64>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return { *static_cast<Int64*>(dataToSerialize) };
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<Int64*>(outValue) = json.get<Int64>();
		}

		static void EditorControl(void* value, const String& name)
		{
			ImGui::DragScalar(
				name.CStr(),
				ImGuiDataType_S64,
				value
			);
		}
	};

	template <>
	struct TypeSerializer<UInt8>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return { *static_cast<UInt8*>(dataToSerialize) };
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<UInt8*>(outValue) = json.get<UInt8>();
		}

		static void EditorControl(void* value, const String& name)
		{
			int v = *static_cast<UInt8*>(value);
			if (ImGui::DragInt(name.CStr(), &v, 1, 0, UINT8_MAX))
				*static_cast<UInt8*>(value) = static_cast<UInt8>(v);
		}
	};

	template <>
	struct TypeSerializer<UInt16>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return { *static_cast<UInt16*>(dataToSerialize) };
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<UInt16*>(outValue) = json.get<UInt16>();
		}

		static void EditorControl(void* value, const String& name)
		{
			int v = *static_cast<UInt16*>(value);
			if (ImGui::DragInt(name.CStr(), &v, 1, 0, UINT16_MAX))
				*static_cast<UInt16*>(value) = static_cast<UInt16>(v);
		}
	};

	template <>
	struct TypeSerializer<UInt32>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return { *static_cast<UInt32*>(dataToSerialize) };
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<UInt32*>(outValue) = json.get<UInt32>();
		}

		static void EditorControl(void* value, const String& name)
		{
			ImGui::DragScalar(
				name.CStr(),
				ImGuiDataType_U32,
				value
			);
		}
	};

	template <>
	struct TypeSerializer<UInt64>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return { *static_cast<UInt64*>(dataToSerialize) };
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<UInt64*>(outValue) = json.get<UInt64>();
		}

		static void EditorControl(void* value, const String& name)
		{
			ImGui::DragScalar(
				name.CStr(),
				ImGuiDataType_U64,
				value
			);
		}
	};

	template <>
	struct TypeSerializer<float>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return { *static_cast<float*>(dataToSerialize) };
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<float*>(outValue) = json.get<float>();
		}

		static void EditorControl(void* value, const String& name)
		{
			ImGui::DragFloat(name.CStr(), static_cast<float*>(value), 0.1f);
		}
	};

	template <>
	struct TypeSerializer<double>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return { *static_cast<double*>(dataToSerialize) };
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<double*>(outValue) = json.get<double>();
		}

		static void EditorControl(void* value, const String& name)
		{
			ImGui::DragScalar(
				name.CStr(),
				ImGuiDataType_Double,
				value,
				0.1
			);
		}
	};







	template <>
	struct TypeSerializer<String>
	{
		static nlohmann::json Serialize(void* dataToSerialize) { return {static_cast<String*>(dataToSerialize)->CStr()}; }
		static void Deserialize(DeserializationContext* deserializationContext, const nlohmann::json& json, void* outValue) { *static_cast<String*>(outValue) = json.get<std::string>().c_str(); }
		static void EditorControl(void* value, const String& name)
		{
			std::string str = std::string(static_cast<String *>(value)->CStr());
			if (ImGui::InputText(name.CStr(), &str)) {
				*static_cast<String *>(value) = str.c_str();
			}
		}
	};

	template <>
	struct TypeSerializer<Path>
	{
		static nlohmann::json Serialize(void* dataToSerialize) { return {static_cast<Path*>(dataToSerialize)->CStr()}; }
		static void Deserialize(DeserializationContext* deserializationContext, const nlohmann::json& json, void* outValue) { *static_cast<Path*>(outValue) = json.get<std::string>().c_str(); }
		static void EditorControl(void* value, const String& name)
		{
			std::string str = std::string(static_cast<Path *>(value)->CStr());
			if (ImGui::InputText(name.CStr(), &str)) {
				*static_cast<Path *>(value) = str.c_str();
			}
		}
	};

	template <>
	struct TypeSerializer<StringW>
	{
		static nlohmann::json Serialize(void* dataToSerialize) { return {static_cast<StringW*>(dataToSerialize)->CStr()}; }
		static void Deserialize(DeserializationContext* deserializationContext, const nlohmann::json& json, void* outValue) { *static_cast<StringW*>(outValue) = json.get<std::wstring>().c_str(); }
		static void EditorControl(void* value, const String& name)
		{
			std::string str = std::string(static_cast<StringW *>(value)->ToNarrow().CStr());
			if (ImGui::InputText(name.CStr(), &str)) {
				*static_cast<StringW *>(value) = StringW::FromNarrow(str.c_str());
			}
		}
	};

	template <>
	struct TypeSerializer<PathW>
	{
		static nlohmann::json Serialize(void* dataToSerialize) { return {static_cast<PathW*>(dataToSerialize)->CStr()}; }
		static void Deserialize(DeserializationContext* deserializationContext, const nlohmann::json& json, void* outValue) { *static_cast<PathW*>(outValue) = json.get<std::wstring>().c_str(); }
		static void EditorControl(void* value, const String& name)
		{
			std::string str = std::string(static_cast<PathW *>(value)->ToString().ToNarrow().CStr());
			if (ImGui::InputText(name.CStr(), &str)) {
				*static_cast<PathW *>(value) = StringW::FromNarrow(str.c_str());
			}
		}
	};

	template<>
	struct TypeSerializer<PluUUID>
	{
		static nlohmann::json Serialize(void* dataToSerialize) { return static_cast<PluUUID*>(dataToSerialize)->getUUID(); }
		static void Deserialize(DeserializationContext* deserializationContext, const nlohmann::json& json, void* outValue) { *static_cast<PluUUID*>(outValue) = json.get<UInt64>(); }
		static void EditorControl(void* value, const String& name)
		{
			std::string str = std::string(static_cast<PluUUID *>(value)->toString().CStr());
			if (ImGui::InputText(name.CStr(), &str)) {
				*static_cast<PluUUID *>(value) = String(str.c_str()).ToInt();
			}
		}
	};

	template<typename T>
	struct TypeSerializer<TUsePointer<T>>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return {};
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
		}

		static void EditorControl(void* value, const String& name)
		{
			static DynamicArray<TUsePointer<EngineObject>> allObjectsOfTStatic;
			static DynamicArray<IAssetInfo*> allAssetsOfTStatic;
			static EngineObjectHandle selected;
			if (ImGui::Button(("Refresh##" + name).CStr()))
			{
				if (T::GetStaticClass()->IsDerivedOrSame(IAssetInfo::GetStaticClass())) {
					//TODO Asset display and array
				}
				allObjectsOfTStatic = TypeRegistry::GetInstance()->GetObjectManager()->GetAllObjectsOfClass(T::GetStaticClass());
			}
			String preview = "Nothing Selected!";
			if (TypeRegistry::GetInstance()->GetObjectManager()->IsValid(selected))
			{
				preview = TypeRegistry::GetInstance()->GetObjectManager()->GetObjectAsUser<EngineObject>(selected)->GetDisplayName();
			}
			if (ImGui::BeginCombo(name.CStr(), preview.CStr(), 0))
            {
                static ImGuiTextFilter filter;
                if (ImGui::IsWindowAppearing())
                {
                    ImGui::SetKeyboardFocusHere();
                    filter.Clear();
                }
                ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F);
                filter.Draw("##Filter", -FLT_MIN);

                for (int n = 0; n < allObjectsOfTStatic.Size(); n++)
                {
                    PropertyInfo* nameProp = allObjectsOfTStatic.At(n)->GetClass()->FindProperty("Name");
                    String objName;
                    if (nameProp) {
                        String* name = static_cast<String *>(nameProp->GetPtr(allObjectsOfTStatic.At(n).GetRaw()));
                        objName = *name;
                    } else {
                        objName = allObjectsOfTStatic.At(n)->GetDisplayName();
                    }
                    const bool is_selected = (*allObjectsOfTStatic.At(n)->GetEngineObjectHandle() == selected);
                    if (filter.PassFilter(objName.CStr()))
                        if (ImGui::Selectable(objName.CStr(), is_selected)) {
                            selected = *allObjectsOfTStatic.At(n)->GetEngineObjectHandle();
                        }
                }
                ImGui::EndCombo();
            }
		}
	};

	template <>
	struct TypeSerializer<TypeInfo*>
	{
		static nlohmann::json Serialize(TypeInfo* dataToSerialize, void* object)
		{
			nlohmann::json json;
			json["typeName"] = dataToSerialize->TypeName.CStr();
			json["fields"] = nlohmann::json::array();
			for (auto property : dataToSerialize->Properties) {
				nlohmann::json prop;
				prop["name"] = property->PropertyName.CStr();
				prop["value"] = property->SerializePtr(property->GetPtr(object));
				json["fields"].push_back(prop);
			}
			return json;
		}

		static void* Deserialize(DeserializationContext* deserializationContext, const nlohmann::json& json, TypeInfo* type)
		{
			void* newObj = type->Construct();
			for (auto field : json["fields"]) {
				PropertyInfo* prop = type->FindProperty(field["name"].get<std::string>().c_str());
				if (!prop) continue;
				void* propValue = prop->GetPtr(newObj);
				prop->DeserializePtr(deserializationContext, field["value"], propValue);
			}
			return newObj;
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