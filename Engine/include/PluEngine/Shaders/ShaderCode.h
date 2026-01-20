//
// Created by Plutex on 1/19/26.
//

#ifndef PLUENGINE_SHADERCODE_H
#define PLUENGINE_SHADERCODE_H
#include "PluEngine/Objects/EngineObject.h"
#include "IShaderCode.generated.h"

namespace Plu
{
	//Runtime and Editor specific shader code loader
	PLU_CLASS(Abstract)
	class PLU_API IShaderCode : public EngineObject
	{
		REFLECTION_BODY_ISHADERCODE()
	public:
		virtual String GetCode() = 0;
	};
}

#endif //PLUENGINE_SHADERCODE_H
