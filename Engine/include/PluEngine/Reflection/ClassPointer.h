//
// Created by Plutex on 2026-02-14.
//

#ifndef PLUENGINE_CLASSPOINTER_H
#define PLUENGINE_CLASSPOINTER_H
#include "ReflectionBase.h"
#include "PluEngine/Objects/EngineObject.h"

namespace Plu
{
	class EngineObject;
	template<typename T>
	concept EngineObjectConc = std::is_same_v<EngineObject, T> || std::derived_from<T, EngineObject>;

	template<typename T>
	requires EngineObjectConc<T>
	class TClassPointer
	{
		TypeInfo* type;
	public:
		TClassPointer()
		{
			type = nullptr;
		}

		TClassPointer(const TClassPointer& other)
		{
			type = other.type;
		}

		TClassPointer(TClassPointer&& other) noexcept
		{
			type = other.type;
			other.type = nullptr;
		}

		TClassPointer& operator=(TypeInfo* typeInfo)
		{
			assert(T::GetStaticClass()->IsDerivedOfOrSame(typeInfo) && "Type is not derived from T");
			type = typeInfo;
			return *this;
		}

		TClassPointer& operator=(const TClassPointer& other)
		{
			assert(T::GetStaticClass()->IsDerivedOfOrSame(other.type) && "Type is not derived from T");
			if (this != &other) {
				type = other.type;
			}
			return *this;
		}

		TClassPointer& operator=(TClassPointer&& other) noexcept
		{
			assert(T::GetStaticClass()->IsDerivedOfOrSame(other.type) && "Type is not derived from T");
			if (this != &other) {
				type = other.type;
				other.type = nullptr;
			}
			return *this;
		}

		[[nodiscard]] TypeInfo* GetRawType() const
		{
			return type;
		}

		[[nodiscard]] TypeInfo* GetTType() const
		{
			return T::GetStaticClass();
		}
	};
}

#endif //PLUENGINE_CLASSPOINTER_H