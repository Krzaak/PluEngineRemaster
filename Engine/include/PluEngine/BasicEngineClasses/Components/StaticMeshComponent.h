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
	public:
		StaticMeshComponent() = default;
		~StaticMeshComponent() override = default;

		PLU_PROPERTY()
		TUsePointer<StaticMesh> StaticMeshToDisplay;

		PLU_PROPERTY()
		TUsePointer<ShaderProgram> Shader;

		TUsePointer<StaticMesh> GetStaticMesh();
		void SetStaticMesh(TUsePointer<StaticMesh> staticMesh);

		//Rendering
		ShaderProgram* GetShaderProgramToRender() override;
		StaticMesh* GetStaticMeshToRender() override;

		Vec3 GetRenderLocation() override;
		Vec3 GetRenderRotation() override;
		Vec3 GetRenderScale() override;
	};
}

#endif //PLUENGINE_STATICMESHCOMPONENT_H
