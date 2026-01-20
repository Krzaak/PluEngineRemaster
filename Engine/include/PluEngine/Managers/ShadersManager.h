//
// Created by Plutex on 1/4/26.
//

#ifndef PLUENGINE_SHADERSMANAGER_H
#define PLUENGINE_SHADERSMANAGER_H
#include "PluEngine/Objects/EngineObject.h"
#include "ShadersManager.generated.h"
#include "PluEngine/PluUUID.h"

namespace Plu
{
	class IShaderCode;
	class ShaderProgram;
	PLU_CLASS(Abstract)
	class PLU_API IShaderManager : public EngineObject
	{
		REFLECTION_BODY_ISHADERMANAGER()
	public:
		virtual TUsePointer<ShaderProgram> GetShaderProgram(PluUUID uuid) = 0;
		virtual TUsePointer<IShaderCode> GetShaderCode(PluUUID uuid) = 0;
	};
}

#endif //PLUENGINE_SHADERSMANAGER_H
