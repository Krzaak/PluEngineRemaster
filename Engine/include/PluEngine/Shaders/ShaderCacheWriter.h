//
// Created by Plutex on 1/19/26.
//

#ifndef PLUENGINE_SHADERCACHEWRITER_H
#define PLUENGINE_SHADERCACHEWRITER_H
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "IShaderCacheWriter.generated.h"

namespace Plu
{
	PLU_CLASS()
	class PLU_API IShaderCacheWriter : public EngineObject
	{
		REFLECTION_BODY_ISHADERCACHEWRITER()
	public:
		virtual PathW GetShaderCacheDirectory() = 0;
	};

	TUsePointer<IShaderCacheWriter> GetGlobalShaderCacheWriter();
}

#endif //PLUENGINE_SHADERCACHEWRITER_H
