//
// Created by Plutex on 1/19/26.
//

#ifndef PLUENGINE_STATICMESHCOMPONENT_H
#define PLUENGINE_STATICMESHCOMPONENT_H
#include "PluEngine/GameObject/WorldComponent.h"
#include "StaticMeshComponent.generated.h"
#include "PluEngine/Renderer/RenderingInterfaces.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"

namespace Plu
{
	struct StaticMesh;
	PLU_CLASS(PyExport)
	class PLU_API StaticMeshComponent : public WorldComponent, public IRenderable
	{
		REFLECTION_BODY_STATICMESHCOMPONENT()
	public:
		StaticMeshComponent() = default;
		~StaticMeshComponent() override = default;

		PLU_PROPERTY()
		TUsePointer<StaticMesh> StaticMeshToDisplay;

		PLU_PROPERTY()
		TUsePointer<MaterialInfo> Material;

		PLU_FUNCTION(PyExport)
		TUsePointer<StaticMesh> GetStaticMesh();
		PLU_FUNCTION(PyExport)
		void SetStaticMesh(TUsePointer<StaticMesh> staticMesh);

		PLU_FUNCTION(PyExport)
		TUsePointer<MaterialInfo> GetMaterial();
		PLU_FUNCTION(PyExport)
		void SetMaterial(TUsePointer<MaterialInfo> material);

		//Rendering
		MaterialInfo *GetMaterialInfoToRender() override;
		StaticMesh* GetStaticMeshToRender() override;
		EngineObjectHandle *GetRenderableObjectHandle() override;

		Vec3 GetRenderLocation() override;
		Vec3 GetRenderRotation() override;
		Vec3 GetRenderScale() override;
	};
}

#endif //PLUENGINE_STATICMESHCOMPONENT_H
