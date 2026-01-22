//
// Created by Plutex on 1/3/26.
//

#include "PluEngine/Reflection/ReflectionBase.h"

#include "PluEngine/Log.h"
#include "PluEngine/Objects/EngineObjectManager.h"

namespace Plu
{
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

	TypeInfo::TypeInfo(UInt64 size, String typeName, TypeType type) : TypeSize(size), TypeName(typeName), Type(type)
	{
	}

	TypeInfo::~TypeInfo() = default;

	TypeRegistry * TypeRegistry::GetInstance()
	{
		static TypeRegistry* instance;
		if (!instance) {
			PLU_CORE_WARN("No reflection instance, creating new!");
			instance = new TypeRegistry();
		}
		return instance;
	}

	void TypeRegistry::AddType(TypeInfo *typeInfo)
	{
		PLU_CORE_TRACE("Type {} added to global TypeRegistry", typeInfo->TypeName.CStr());
		mTypeMap.Insert(typeInfo->TypeName, std::move(typeInfo));
	}

	TypeInfo * TypeRegistry::GetTypeOfName(const String& typeName)
	{
		return *mTypeMap.Find(typeName);
	}

	GameHashMap<String, TypeInfo*> * TypeRegistry::GetTypeMap()
	{
		return &mTypeMap;
	}
}
