//
// Created by Plutex on 2026-01-21.
//

#ifndef PLUENGINE_TYPETRAITS_H
#define PLUENGINE_TYPETRAITS_H

#include "ReflectionBase.h"
#include "misc/cpp/imgui_stdlib.h"
#include "PluEngine/PluTypes.h"
#include "PluEngine/PluUUID.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <pybind11/pybind11.h>

#include "ClassPointer.h"
#include "PluEngine/Managers/AssetsManager.h"

namespace Plu
{
	bool TUsePointerAssetUI(void* value, String name, TypeInfo* typeInfo);

	template <>
	struct TypeSerializer<int>
	{
		static nlohmann::json Serialize(void* dataToSerialize) { return *static_cast<int*>(dataToSerialize); }
		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue) { *static_cast<int*>(outValue) = json.get<int>(); }
		static bool EditorControl(void* value, const String& name) { return ImGui::DragInt(name.CStr(), static_cast<int*>(value)); }
	};

	template <>
	struct TypeSerializer<bool>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return *static_cast<bool*>(dataToSerialize);
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<bool*>(outValue) = json.get<bool>();
		}

		static bool EditorControl(void* value, const String& name)
		{
			return ImGui::Checkbox(name.CStr(), static_cast<bool*>(value));
		}
	};

	template <>
	struct TypeSerializer<Int8>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return  *static_cast<Int8*>(dataToSerialize);
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<Int8*>(outValue) = json.get<Int8>();
		}

		static bool EditorControl(void* value, const String& name)
		{
			int v = *static_cast<Int8*>(value);
			if (ImGui::DragInt(name.CStr(), &v, 1, INT8_MIN, INT8_MAX)) {
				*static_cast<Int8*>(value) = static_cast<Int8>(v);
				return true;
			}
			return false;
		}
	};

	template <>
	struct TypeSerializer<Int16>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return *static_cast<Int16*>(dataToSerialize);
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<Int16*>(outValue) = json.get<Int16>();
		}

		static bool EditorControl(void* value, const String& name)
		{
			int v = *static_cast<Int16*>(value);
			if (ImGui::DragInt(name.CStr(), &v, 1, INT16_MIN, INT16_MAX)) {
				*static_cast<Int16*>(value) = static_cast<Int16>(v);
				return true;
			}
			return false;
		}
	};

	template <>
	struct TypeSerializer<Int64>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return *static_cast<Int64*>(dataToSerialize);
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<Int64*>(outValue) = json.get<Int64>();
		}

		static bool EditorControl(void* value, const String& name)
		{
			return ImGui::DragScalar(
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
			return *static_cast<UInt8*>(dataToSerialize);
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<UInt8*>(outValue) = json.get<UInt8>();
		}

		static bool EditorControl(void* value, const String& name)
		{
			int v = *static_cast<UInt8*>(value);
			if (ImGui::DragInt(name.CStr(), &v, 1, 0, UINT8_MAX)) {
				*static_cast<UInt8*>(value) = static_cast<UInt8>(v);
				return true;
			}
			return false;
		}
	};

	template <>
	struct TypeSerializer<UInt16>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return  *static_cast<UInt16*>(dataToSerialize);
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<UInt16*>(outValue) = json.get<UInt16>();
		}

		static bool EditorControl(void* value, const String& name)
		{
			int v = *static_cast<UInt16*>(value);
			if (ImGui::DragInt(name.CStr(), &v, 1, 0, UINT16_MAX)) {
				*static_cast<UInt16*>(value) = static_cast<UInt16>(v);
				return true;
			}
			return false;
		}
	};

	template <>
	struct TypeSerializer<UInt32>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return  *static_cast<UInt32*>(dataToSerialize);
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<UInt32*>(outValue) = json.get<UInt32>();
		}

		static bool EditorControl(void* value, const String& name)
		{
			return ImGui::DragScalar(
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
			return *static_cast<UInt64*>(dataToSerialize);
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<UInt64*>(outValue) = json.get<UInt64>();
		}

		static bool EditorControl(void* value, const String& name)
		{
			return ImGui::DragScalar(
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
			return *static_cast<float*>(dataToSerialize) ;
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<float*>(outValue) = json.get<float>();
		}

		static bool EditorControl(void* value, const String& name)
		{
			return ImGui::DragFloat(name.CStr(), static_cast<float*>(value), 0.1f);
		}
	};

	template <>
	struct TypeSerializer<double>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			return *static_cast<double*>(dataToSerialize);
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<double*>(outValue) = json.get<double>();
		}

		static bool EditorControl(void* value, const String& name)
		{
			return ImGui::DragScalar(
				name.CStr(),
				ImGuiDataType_Double,
				value,
				0.1f
			);
		}
	};

	template <>
	struct TypeSerializer<String>
	{
		static nlohmann::json Serialize(void* dataToSerialize) { return static_cast<String*>(dataToSerialize)->CStr(); }
		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue) { *static_cast<String*>(outValue) = json.get<std::string>().c_str(); }
		static bool EditorControl(void* value, const String& name)
		{
			std::string str = std::string(static_cast<String *>(value)->CStr());
			if (ImGui::InputText(name.CStr(), &str)) {
				*static_cast<String *>(value) = str.c_str();
				return true;
			}
			return false;
		}
	};

	template <>
	struct TypeSerializer<Path>
	{
		static nlohmann::json Serialize(void* dataToSerialize) { return static_cast<Path*>(dataToSerialize)->CStr(); }
		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue) { *static_cast<Path*>(outValue) = json.get<std::string>().c_str(); }
		static bool EditorControl(void* value, const String& name)
		{
			std::string str = std::string(static_cast<Path *>(value)->CStr());
			if (ImGui::InputText(name.CStr(), &str)) {
				*static_cast<Path *>(value) = str.c_str();
				return true;
			}
			return false;
		}
	};

	template <>
	struct TypeSerializer<StringW>
	{
		static nlohmann::json Serialize(void* dataToSerialize) { return static_cast<StringW*>(dataToSerialize)->CStr(); }
		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue) { *static_cast<StringW*>(outValue) = json.get<std::wstring>().c_str(); }
		static bool EditorControl(void* value, const String& name)
		{
			std::string str = std::string(static_cast<StringW *>(value)->ToNarrow().CStr());
			if (ImGui::InputText(name.CStr(), &str)) {
				*static_cast<StringW *>(value) = StringW::FromNarrow(str.c_str());
				return true;
			}
			return false;
		}
	};

	template <>
	struct TypeSerializer<PathW>
	{
		static nlohmann::json Serialize(void* dataToSerialize) { return static_cast<PathW*>(dataToSerialize)->CStr(); }
		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue) { *static_cast<PathW*>(outValue) = json.get<std::wstring>().c_str(); }
		static bool EditorControl(void* value, const String& name)
		{
			std::string str = std::string(static_cast<PathW *>(value)->ToString().ToNarrow().CStr());
			if (ImGui::InputText(name.CStr(), &str)) {
				*static_cast<PathW *>(value) = StringW::FromNarrow(str.c_str());
				return true;
			}
			return false;
		}
	};

	template<>
	struct TypeSerializer<PluUUID>
	{
		static nlohmann::json Serialize(void* dataToSerialize) { return static_cast<PluUUID*>(dataToSerialize)->getUUID(); }
		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			*static_cast<PluUUID*>(outValue) = json.get<UInt64>();
		}
		static bool EditorControl(void* value, const String&)
		{
			return false;
		}
	};

	template<typename T>
	struct TypeSerializer<DynamicArray<T>>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			nlohmann::json json = nlohmann::json::array();
			DynamicArray<T>* array = static_cast<DynamicArray<T> *>(dataToSerialize);
			for (T& item : *array) {
				json.push_back(TypeSerializer<T>::Serialize(&item));
			}
			return json;
		}
		static void Deserialize(DeserializationContext* deserializationContext, const nlohmann::json& json, void* outValue)
		{
			DynamicArray<T>* array = static_cast<DynamicArray<T> *>(outValue);
			array->Reserve(json.size());
			for (auto item : json) {
				T newItem = T();
				TypeSerializer<T>::Deserialize(deserializationContext, item, &newItem);
				array->PushBack(newItem);
			}
		}
		static bool EditorControl(void* value, const String& name)
		{
			return false;
		}
	};

	template<typename T>
	struct TypeSerializer<TUsePointer<T>>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			if (T::GetStaticClass()->IsDerivedOfOrSame(IAssetData::GetStaticClass())) {
				TypeInfo* assetToSerialize = T::GetStaticClass();
				if (!dataToSerialize) {
					return 0;
				}
				if (!static_cast<TUsePointer<T>*>(dataToSerialize)->GetRaw()) {
					return 0;
				}
				if (assetToSerialize->GetTypeUuidProp()) {
					return TypeSerializer<PluUUID>::Serialize(assetToSerialize->GetTypeUuidProp()->GetPtr(static_cast<TUsePointer<T>*>(dataToSerialize)->GetRaw()));
				}
			}
			if (!dataToSerialize) {
				return {};
			}
			if (!static_cast<TUsePointer<T>*>(dataToSerialize)->GetRaw()) {
				return {};
			}
			return TypeSerializer<T>::Serialize(static_cast<TUsePointer<T>*>(dataToSerialize)->GetRaw());
		}

		static void Deserialize(DeserializationContext* dc, const nlohmann::json& json, void* outValue)
		{
			if (T::GetStaticClass()->IsDerivedOfOrSame(IAssetData::GetStaticClass())) {
				TypeInfo* assetToSerialize = T::GetStaticClass();
				if (assetToSerialize->GetTypeUuidProp()) {
					PluUUID uuid;
					TypeSerializer<PluUUID>::Deserialize(dc, json, &uuid);
					if (uuid.getUUID() == 0) {
						return;
					}
					TUsePointer<IAssetData> asset = dc->assetManager->GetAssetData(uuid);
					if (asset) {
						*static_cast<TUsePointer<T>*>(outValue) = StaticCast<T>(asset);
					}
					return;
				}
			}
			TypeSerializer<T>::Deserialize(dc, json, outValue);
		}

		static bool EditorControl(void* value, const String& name)
		{
			static DynamicArray<EngineObjectHandle> allObjectsOfTStatic;
			static EngineObjectHandle selected;
			static bool assets = T::GetStaticClass()->IsDerivedOfOrSame(IAssetData::GetStaticClass());
			if (!assets) {
				if (ImGui::Button(("Refresh##" + name).CStr()))
				{
					if (T::GetStaticClass()->IsDerivedOfOrSame(IAssetData::GetStaticClass())) {
						assets = true;
					} else {
						assets = false;
						allObjectsOfTStatic = TypeRegistry::GetInstance()->GetObjectManager()->GetAllObjectsOfClass(T::GetStaticClass());
					}
				}
			}
			String preview = "Nothing Selected!";
			if (assets) {
#ifdef PLU_ENGINE_EDITOR_BUILD
				return TUsePointerAssetUI(value, name, T::GetStaticClass());
#else
				ImGui::Text("No Editor Utils in engine! Cannot show asset selection UI")
#endif
			}
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

				bool changed = false;

                for (int n = 0; n < allObjectsOfTStatic.Size(); n++)
                {
                	TUsePointer<EngineObject> obj = TypeRegistry::GetInstance()->GetObjectManager()->GetObjectAsUser<EngineObject>(allObjectsOfTStatic[n]);
                    PropertyInfo* nameProp = obj->GetClass()->FindProperty("Name");
                    String objName;
                    if (nameProp) {
                        String* name2 = static_cast<String *>(nameProp->GetPtr(obj.GetRaw()));
                        objName = *name2;
                    } else {
                        objName = obj->GetDisplayName();
                    }
                    const bool is_selected = (*obj->GetEngineObjectHandle() == selected);
                    if (filter.PassFilter(objName.CStr()))
                        if (ImGui::Selectable(objName.CStr(), is_selected)) {
                            selected = *obj->GetEngineObjectHandle();
                        	*static_cast<TUsePointer<EngineObject>*>(value) = obj;
                        	changed = true;
                        }
                }
                ImGui::EndCombo();
				return changed;
            }
			return false;
		}
	};

	template <typename T>
	struct TypeSerializer<TOwningPointer<T>>
	{
		static nlohmann::json Serialize(void* dataToSerialize) { return TypeSerializer<TUsePointer<T>>::Serialize(dataToSerialize); }
		static void Deserialize(DeserializationContext* deserializationContext, const nlohmann::json& json, void* outValue) { TypeSerializer<TUsePointer<T>>::Deserialize(deserializationContext, json, outValue); }
		static bool EditorControl(void* value, const String& name)
		{
			return TypeSerializer<TUsePointer<T>>::EditorControl(value, name);
		}
	};

	template <>
	struct TypeSerializer<TypeInfo*>
	{
		static nlohmann::json SerializeFields(TypeInfo* type, void* object)
		{
			nlohmann::json json;
			json["fields"] = nlohmann::json::array();
			for (auto property : type->Properties) {
				nlohmann::json prop;
				prop["name"] = property->PropertyName.CStr();
				prop["value"] = property->SerializePtr(property->GetPtr(object));
				json["fields"].push_back(prop);
			}
			return json;
		}
		static void SerializeTree(TypeInfo* type, void* object, nlohmann::json& json)
		{
			if (type->BaseType) {
				SerializeTree(type->BaseType, object, json);
			}
			nlohmann::json props = SerializeFields(type, object);
			for (auto prop : props["fields"]) {
				//Only for debugging
				String propName = prop["name"].get<std::string>().c_str();
				json["fields"].push_back(prop);
			}
		}
		static nlohmann::json Serialize(TypeInfo* dataToSerialize, void* object)
		{
			nlohmann::json json;
			json["typeName"] = dataToSerialize->TypeName.CStr();
			json["fields"] = nlohmann::json::array();
			SerializeTree(dataToSerialize, object, json);
			return json;
		}

		static void* Deserialize(DeserializationContext* deserializationContext, const nlohmann::json& json, TypeInfo* type)
		{
			void* newObj = type->Construct();
			if (!json.contains("fields")) return newObj;
			for (auto field : json["fields"]) {
				PropertyInfo* prop = type->FindProperty(field["name"].get<std::string>().c_str());
				if (!prop) continue;
				void* propValue = prop->GetPtr(newObj);
				prop->DeserializePtr(deserializationContext, field["value"], propValue);
			}
			return newObj;
		}
		static void* Deserialize(DeserializationContext* deserializationContext, const nlohmann::json& json, TypeInfo* type, void* obj)
		{
			if (!json.contains("fields")) return obj;
			for (auto field : json["fields"]) {
				PropertyInfo* prop = type->FindProperty(field["name"].get<std::string>().c_str());
				if (!prop) continue;
				void* propValue = prop->GetPtr(obj);
				prop->DeserializePtr(deserializationContext, field["value"], propValue);
			}
			return obj;
		}
	private:
		static void GatherParents(Plu::TypeInfo* typeInfo, DynamicArray<Plu::TypeInfo*>* parents)
		{
			if (!typeInfo) return;
			parents->PushBack(typeInfo);
			GatherParents(typeInfo->BaseType, parents);
		}
	public:
		static void EditorControl(TypeInfo* value, void* obj)
		{
			DynamicArray<TypeInfo*> parents;
			GatherParents(value, &parents);
			for (auto parent : parents) {
				for (PropertyInfo* prop : parent->Properties)
				{
					void* propValue = malloc(prop->PropertySize);
					memcpy(propValue, prop->GetPtr(obj), prop->PropertySize);
					if (prop->GetterPtr) {
						prop->GetterPtr(obj, propValue);
					}
					bool changed = prop->EditorControlPtr(propValue, prop->PropertyName);
					if (changed) {
						if (prop->SetterPtr) {
							prop->SetterPtr(obj, propValue);
						} else {
							memcpy(prop->GetPtr(obj), propValue, prop->PropertySize);
						}
					}
					free(propValue);
				}
			}
		}
	};

	template <>
	struct TypeSerializer<glm::vec2>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			auto& v = *static_cast<glm::vec2*>(dataToSerialize);
			return { v.x, v.y };
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			auto& v = *static_cast<glm::vec2*>(outValue);
			v.x = json[0].get<float>();
			v.y = json[1].get<float>();
		}

		static bool EditorControl(void* value, const String& name)
		{
			return ImGui::DragFloat2(name.CStr(), &static_cast<glm::vec2*>(value)->x, 0.1f);
		}
	};

	template <>
	struct TypeSerializer<glm::vec3>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			auto& v = *static_cast<glm::vec3*>(dataToSerialize);
			return { v.x, v.y, v.z };
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			auto& v = *static_cast<glm::vec3*>(outValue);
			v.x = json[0].get<float>();
			v.y = json[1].get<float>();
			v.z = json[2].get<float>();
		}

		static bool EditorControl(void* value, const String& name)
		{
			if (name.Contains("color") || name.Contains("colour") || name.Contains("Color") || name.Contains("Colour")) {
				return ImGui::ColorEdit3(name.CStr(), &static_cast<glm::vec3*>(value)->x);
			} else {
				return ImGui::DragFloat3(name.CStr(), &static_cast<glm::vec3*>(value)->x, 0.1f);
			}
		}
	};

	template <>
	struct TypeSerializer<glm::vec4>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			auto& v = *static_cast<glm::vec4*>(dataToSerialize);
			return { v.x, v.y, v.z, v.w };
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			auto& v = *static_cast<glm::vec4*>(outValue);
			v.x = json[0].get<float>();
			v.y = json[1].get<float>();
			v.z = json[2].get<float>();
			v.w = json[3].get<float>();
		}

		static bool EditorControl(void* value, const String& name)
		{
			if (name.Contains("color") || name.Contains("colour") || name.Contains("Color") || name.Contains("Colour")) {
				return ImGui::ColorEdit4(name.CStr(), &static_cast<glm::vec4*>(value)->x);
			} else {
				return ImGui::DragFloat4(name.CStr(), &static_cast<glm::vec4*>(value)->x, 0.1f);
			}
		}
	};

	template <>
	struct TypeSerializer<glm::quat>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			auto& q = *static_cast<glm::quat*>(dataToSerialize);
			return { q.x, q.y, q.z, q.w };
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			auto& q = *static_cast<glm::quat*>(outValue);
			q.x = json[0].get<float>();
			q.y = json[1].get<float>();
			q.z = json[2].get<float>();
			q.w = json[3].get<float>();
		}
		static bool EditorControl(void* value, const String& name)
		{
			auto& q = *static_cast<glm::quat*>(value);

			glm::vec3 euler = glm::degrees(glm::eulerAngles(q));

			if (ImGui::DragFloat3(name.CStr(), &euler.x, 0.5f))
			{
				q = glm::quat(glm::radians(euler));
				q = glm::normalize(q);
				return true;
			}
			return false;
		}
	};

	template <typename T>
	requires EngineObjectConc<T>
	struct TypeSerializer<TClassPointer<T>>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			TClassPointer<T>* classPtr = static_cast<TClassPointer<T> *>(dataToSerialize);
			return classPtr->GetRawType()->TypeName.CStr();
		}

		static void Deserialize(DeserializationContext*, const nlohmann::json& json, void* outValue)
		{
			TClassPointer<T>* classPtr = static_cast<TClassPointer<T> *>(outValue);
			*classPtr = TypeRegistry::GetInstance()->GetTypeOfName(json.get<std::string>().c_str());
		}

		static bool EditorControl(void* value, const String& name)
		{
			static GameHashMap<String, DynamicArray<TypeInfo*>> typesPerT;

			if (ImGui::Button("Refresh")) {
				DynamicArray<TypeInfo*> types;
				for (const auto& type : *TypeRegistry::GetInstance()->GetTypeMap()) {
					if (type.second->IsDerivedOfOrSame(T::GetStaticClass())) {
						types.PushBack(type.second);
					}
				}
				typesPerT[T::GetStaticClass()->TypeName] = types;
			}

			if (!typesPerT.Contains(T::GetStaticClass()->TypeName)) {
				DynamicArray<TypeInfo*> types;
				for (const auto& type : *TypeRegistry::GetInstance()->GetTypeMap()) {
					if (type.second->IsDerivedOfOrSame(T::GetStaticClass())) {
						types.PushBack(type.second);
					}
				}
				typesPerT[T::GetStaticClass()->TypeName] = types;
			}

			String preview;
			TypeInfo* selectedType = nullptr;
			TClassPointer<T>* ptr = static_cast<TClassPointer<T> *>(value);
			if (typesPerT[T::GetStaticClass()->TypeName].Contains(ptr->GetRawType())) {
				preview = ptr->GetRawType()->TypeName;
				selectedType = ptr->GetRawType();
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

				bool changed = false;

				for (int n = 0; n < typesPerT[T::GetStaticClass()->TypeName].Size(); n++)
				{
					String objName = typesPerT[T::GetStaticClass()->TypeName].At(n)->TypeName;
					const bool is_selected = (typesPerT[T::GetStaticClass()->TypeName].At(n) == selectedType);
					if (filter.PassFilter(objName.CStr()))
						if (ImGui::Selectable(objName.CStr(), is_selected)) {
							selectedType = typesPerT[T::GetStaticClass()->TypeName].At(n);
							*ptr = typesPerT[T::GetStaticClass()->TypeName].At(n);
							changed = true;
						}
				}
				ImGui::EndCombo();
				return changed;
			}
			return false;
		}
	};
	//Here Serializer
}

namespace pybind11::detail {

	template <typename Allocator>
	class type_caster<Plu::BasicString<char, Allocator>> {
	public:
		using StringType = Plu::BasicString<char, Allocator>;
		PYBIND11_TYPE_CASTER(StringType, const_name("str"));

		// Python str → C++ BasicString
		bool load(handle src, bool) {
			PyObject* tmp = PyUnicode_AsUTF8String(src.ptr());
			if (!tmp) return false;
			const char* utf8 = PyBytes_AS_STRING(tmp);
			value = StringType(utf8);
			Py_DECREF(tmp);
			return true;
		}

		// C++ BasicString → Python str
		static handle cast(const StringType& src, return_value_policy, handle) {
			return PyUnicode_FromStringAndSize(src.CStr(), src.Length());
		}
	};

}

#endif //PLUENGINE_TYPETRAITS_H