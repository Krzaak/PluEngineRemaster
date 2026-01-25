//
// Created by Plutex on 1/19/26.
//

#ifndef PLUENGINE_STATICMESHCOMPONENT_H
#define PLUENGINE_STATICMESHCOMPONENT_H
#include "PluEngine/GameObject/WorldComponent.h"
#include "StaticMeshComponent.generated.h"
#include "PluEngine/Renderer/RenderingInterfaces.h"

namespace Plu
{
	struct StaticMesh;
	PLU_CLASS()
	class PLU_API StaticMeshComponent : public WorldComponent, public IRenderable
	{
		REFLECTION_BODY_STATICMESHCOMPONENT()
	private:
		TUsePointer<StaticMesh> mStaticMesh;
	public:
		StaticMeshComponent() = default;
		~StaticMeshComponent() override = default;

		TUsePointer<StaticMesh> GetStaticMesh();
		void SetStaticMesh(TUsePointer<StaticMesh> staticMesh);

		//Rendering
		TUsePointer<ShaderProgram>& GetShaderProgramToRender() override;
		TUsePointer<StaticMesh>& GetStaticMeshToRender() override;
	};
}

#endif //PLUENGINE_STATICMESHCOMPONENT_H
