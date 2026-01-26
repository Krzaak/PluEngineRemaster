//
// Created by Plutex on 1/19/26.
//

#ifndef PLUENGINE_RENDERINGINTERFACES_H
#define PLUENGINE_RENDERINGINTERFACES_H
#include "PluEngine/Interface/PluInterface.h"
#include "Pointers/TUsePointer.h"

namespace Plu
{
	struct StaticMesh;
}

namespace Plu
{
	class ShaderProgram;
}

namespace Plu
{
	//Simple interfaces for Rendering stuff
	PLU_INTERFACE()
	class IRenderable : public PluInterface
	{
	public:
		virtual ~IRenderable() = default;
		virtual TUsePointer<ShaderProgram>& GetShaderProgramToRender() = 0;
		virtual TUsePointer<StaticMesh>& GetStaticMeshToRender() = 0;
	};
}

#endif //PLUENGINE_RENDERINGINTERFACES_H