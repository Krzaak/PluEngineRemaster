//
// Created by Plutex on 1/3/26.
//

#ifndef PLUENGINE_REFLECTIONBASE_H
#define PLUENGINE_REFLECTIONBASE_H

#include <functional>

#include "PluSTL_FWD.h"
#include "PluEngine/Core.h"
#include "PluEngine/Log.h"

namespace Plu
{
	enum class PropertyType
	{
		Int,
		Float,
		Bool,
		Double,
		String,
		StringW,
		UserPointer,
		OwningPointer,
		DynamicArray,
		Unknown
	};

	struct PLU_API PropertyInfo
	{
		String PropertyName;
		MaxUInt64 PropertyOffset;
		MaxUInt64 PropertySize;
		PropertyType PropertyType;
		String PropertyTypeName;

		bool IsPersistent = true;
		bool IsVisibleInEditor = false;

		void* GetPtr(void* objectInstance) const;
	};

	struct PLU_API TypeInfo
	{
		using ConstructorFunc = std::function<void*()>;
		MaxUInt64 TypeSize;
		String TypeName;
		TypeInfo* BaseType; //We only support inheritance through single parent. I'm lazy :)

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

		TypeInfo(MaxUInt64 size, String typeName);
		~TypeInfo();
	};

	class PLU_API TypeRegistry
	{
		FastHashMap<String, TypeInfo*> mTypeMap;
	public:
		static TypeRegistry* GetInstance();
		void AddType(TypeInfo* typeInfo);
		TypeInfo* GetTypeOfName(const String& typeName);
	};
}

#endif //PLUENGINE_REFLECTIONBASE_H