//
// Created by Plutex on 1/19/26.
//

#ifndef PLUENGINE_SHADERCACHEWRITER_H
#define PLUENGINE_SHADERCACHEWRITER_H
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "ShaderCacheWriter.generated.h"

namespace Plu
{
	PLU_CLASS(Abstract)
	class PLU_API IShaderCacheWriter : public EngineObject
	{
		REFLECTION_BODY_ISHADERCACHEWRITER()
	public:
		virtual PathW GetShaderCacheDirectory() = 0;
	};

	PLU_API TUsePointer<IShaderCacheWriter> GetGlobalShaderCacheWriter();
	PLU_API void SetGlobalShaderCacheWriter(const TOwningPointer<IShaderCacheWriter> &newShaderCacheWriter);
}

#endif //PLUENGINE_SHADERCACHEWRITER_H
