//
// Created by Plutex on 1/3/26.
//

#include "PluEngine/Reflection/ReflectionBase.h"

#include "PluEngine/Application.h"
#include "PluEngine/Log.h"
#include "PluEngine/Objects/EngineObjectManager.h"

namespace Plu
{
	void RegisterPluClass(pybind11::type type)
	{
		std::string name = pybind11::str(type.attr("__name__"));
		pybind11::tuple bases = type.attr("__bases__");

		pybind11::handle b = bases[0];
		TypeInfo* base = TypeRegistry::GetInstance()->GetTypeOfName(std::string(pybind11::str(pybind11::reinterpret_borrow<pybind11::type>(b).attr("__name__"))).c_str());
		if (!base) {
			PLU_CORE_ERROR("RegisterPluClass: base type of Python class '{}' is not known to reflection", name);
			return;
		}
		if (!base->IsDerivedOfOrSame(EngineObject::GetStaticClass())) return;
		TypeInfo* newType = new TypeInfo{0, name.c_str(), TypeType::CLASS};
		newType->BaseType = base;
		newType->Constructor = nullptr;
		newType->IsPythonType = true;
		newType->PythonType = type;
		newType->IsAbstract = false;
		TypeRegistry::GetInstance()->AddType(newType);
		PLU_CORE_INFO("Class from python {} -> {}", name, base->TypeName.CStr());
	}

	void * PropertyInfo::GetPtr(void *objectInstance) const
	{
		return static_cast<char*>(objectInstance) + PropertyOffset;
	}

	PropertyInfo * TypeInfo::GetTypeUuidProp() const
	{
		if (TypeUuidProp) return TypeUuidProp;
		if (BaseType) return BaseType->GetTypeUuidProp();
		return nullptr;
	}

	void TypeInfo::AddProperty(PropertyInfo *propertyInfo)
	{
		Properties.PushBack(propertyInfo);
	}

	void * TypeInfo::Construct() const
	{
		return Constructor ? Constructor() : nullptr;
	}

	PropertyInfo * TypeInfo::FindProperty(const String &propertyName)
	{
		for (PropertyInfo* p: Properties) {
			if (p->PropertyName == propertyName) {
				return p;
			}
		}
		if (BaseType) return BaseType->FindProperty(propertyName);
		return nullptr;
	}

	bool TypeInfo::IsChildOf(TypeInfo *potentialParent)
	{
		return potentialParent == this->BaseType;
	}

	void GatherAllParentTypes(TypeInfo* me, DynamicArray<TypeInfo*>* typeInfos)
	{
		if (!me->BaseType) return;
		typeInfos->PushBack(me->BaseType);
		GatherAllParentTypes(me->BaseType, typeInfos);
	}

	bool TypeInfo::IsDerivedOf(TypeInfo *potentialParent)
	{
		DynamicArray<TypeInfo*> parents;
		GatherAllParentTypes(this, &parents);
		return parents.Contains(potentialParent);
	}

	bool TypeInfo::IsDerivedOfOrSame(TypeInfo *potentialParent)
	{
		return IsDerivedOf(potentialParent) || this == potentialParent;
	}

	nlohmann::json TypeInfo::SerializeToJSON(void* obj) const
	{
		return SerializeToJson ? SerializeToJson(obj) : nlohmann::json();
	}

	void * TypeInfo::DeSerializeFromJSON(DeserializationContext *dc, const nlohmann::json &j) const
	{
		return DeserializeFromJson ? DeserializeFromJson(dc, j) : nullptr;
	}

	TypeInfo::TypeInfo(UInt64 size, String typeName, TypeType type) : TypeSize(size), TypeName(typeName), Type(type)
	{
	}

	TypeInfo::~TypeInfo() = default;

	void EnumInfo::AddValue(String enumName, UInt64 value)
	{
		EnumValues.PushBack(new EnumValue{enumName, value});
	}

	TUsePointer<EngineObjectManager> TypeRegistry::GetObjectManager()
	{
		return mApplicationInfo->AppObjectManager;
	}

	TUsePointer<EngineAssetManager> TypeRegistry::GetAssetManager()
	{
		return mApplicationInfo->AppAssetManager;
	}

	TypeRegistry * TypeRegistry::GetInstance()
	{
		// Function-local static: guaranteed single, thread-safe initialisation.
		static TypeRegistry instance;
		return &instance;
	}

	void TypeRegistry::AddType(TypeInfo *typeInfo)
	{
		if (mTypeMap.Contains(typeInfo->TypeName)) {
			mTypeMap.Remove(typeInfo->TypeName);
			PLU_CORE_TRACE("Type {} removed from global TypeRegistry", typeInfo->TypeName.CStr());
		}
		PLU_CORE_TRACE("Type {} added to global TypeRegistry", typeInfo->TypeName.CStr());
		mTypeMap.Insert(typeInfo->TypeName, typeInfo);
	}

	TypeInfo * TypeRegistry::GetTypeOfName(const String& typeName)
	{
		if (mTypeMap.Contains(typeName)) {
			return *mTypeMap.Find(typeName);
		}
		return nullptr;
	}

	GameHashMap<String, TypeInfo*> * TypeRegistry::GetTypeMap()
	{
		return &mTypeMap;
	}

	void TypeRegistry::Cleanup()
	{
		for (auto& entry : mTypeMap) {
			TypeInfo* typeInfo = entry.second;
			if (!typeInfo) continue;
			// The interpreter (Py_Finalize in ~EditorPythonManager) is usually gone by the time we
			// run — a decref through the pybind11 handle would touch a dead interpreter. Drop the
			// reference without decref then; with a live interpreter a normal decref is fine.
			if (!Py_IsInitialized()) {
				typeInfo->PythonType.release();
			}
			for (PropertyInfo* property : typeInfo->Properties) {
				delete property;
			}
			typeInfo->Properties.Clear();
			delete typeInfo;
		}
		mTypeMap.Clear();

		for (auto& entry : mEnumMap) {
			EnumInfo* enumInfo = entry.second;
			if (!enumInfo) continue;
			for (EnumValue* value : enumInfo->EnumValues) {
				delete value;
			}
			delete enumInfo;
		}
		mEnumMap.Clear();
	}
}
