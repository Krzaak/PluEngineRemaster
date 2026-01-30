//
// Created by Plutex on 1/19/26.
//

#ifndef PLUENGINE_RENDERINGINTERFACES_H
#define PLUENGINE_RENDERINGINTERFACES_H
#include "PluEngine/Interface/PluInterface.h"
#include "Pointers/TUsePointer.h"
#include "RenderingInterfaces.generated.h"
#include "PluEngine/PluTypes.h"

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
	class IRenderable
	{
	public:
		virtual ~IRenderable() = default;
		virtual ShaderProgram* GetShaderProgramToRender() = 0;
		virtual StaticMesh* GetStaticMeshToRender() = 0;
		virtual Vec3 GetRenderLocation() = 0;
		virtual Vec3 GetRenderRotation() = 0;
		virtual Vec3 GetRenderScale() = 0;
	};
}

#endif //PLUENGINE_RENDERINGINTERFACES_H