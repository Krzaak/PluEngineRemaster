//
// Created by Plutex on 1/19/26.
//

#ifndef PLUENGINE_SHADERCACHEWRITER_H
#define PLUENGINE_SHADERCACHEWRITER_H
#include "PluEngine/Core.h"
#include "PluEngine/Core/Objects/EngineObject.h"
#include "ShaderCacheWriter.generated.h"

namespace Plu
{
	PLU_CLASS(Abstract)
	class PLURENDER_API IShaderCacheWriter : public EngineObject
	{
		REFLECTION_BODY_ISHADERCACHEWRITER()
	public:
		virtual PathW GetShaderCacheDirectory() = 0;
	};

	PLURENDER_API TUsePointer<IShaderCacheWriter> GetGlobalShaderCacheWriter();
	PLURENDER_API void SetGlobalShaderCacheWriter(const TUsePointer<IShaderCacheWriter> &newShaderCacheWriter);
}

#endif //PLUENGINE_SHADERCACHEWRITER_H
