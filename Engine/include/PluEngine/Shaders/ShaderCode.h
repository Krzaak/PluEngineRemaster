//
// Created by Plutex on 1/19/26.
//

#ifndef PLUENGINE_SHADERCODE_H
#define PLUENGINE_SHADERCODE_H
#include "PluEngine/Objects/EngineObject.h"
#include "ShaderCode.generated.h"
#include "PluEngine/PluUUID.h"

namespace Plu
{
	//Runtime and Editor specific shader code loader
	PLU_CLASS(Abstract, UUID=Uuid)
	class PLU_API IShaderCode : public EngineObject
	{
		REFLECTION_BODY_ISHADERCODE()
	public:
		PLU_PROPERTY()
		PluUUID Uuid;

		PLU_PROPERTY()
		String Name;

		virtual String GetCode() = 0;
	};
}

#endif //PLUENGINE_SHADERCODE_H
