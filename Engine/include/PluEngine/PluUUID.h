//
// Created by Plutex on 1/3/26.
//

#ifndef PLUENGINE_PLUUUID_H
#define PLUENGINE_PLUUUID_H
#include "Core.h"
#include "String/String.h"

namespace Plu
{
	class PluUUID
	{
		MaxUInt64 mUUID;
	public:
		PluUUID();
		PluUUID(MaxUInt64 UUID);
		PluUUID(const PluUUID& other);
		~PluUUID() = default;

		bool operator ==(const PluUUID& other) const { return (mUUID == other.mUUID); }
		bool operator ==(const MaxUInt64& other) const { return (mUUID == other); }
		bool operator !=(const MaxUInt64& other) const { return (mUUID != other); }
		bool operator !=(const PluUUID& other) const { return (mUUID != other.mUUID); }
		PluUUID& operator =(const PluUUID& other);
		operator MaxUInt64() const { return (mUUID); }

		inline String toString() const
		{
			return String::FromInt(mUUID);
		}

		inline MaxUInt64 getUUID() const { return mUUID; }

	};
}

#endif //PLUENGINE_PLUUUID_H