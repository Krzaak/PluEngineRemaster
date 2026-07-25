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
	class PLU_API StaticMeshComponent : public WorldComponent
	{
		REFLECTION_BODY_STATICMESHCOMPONENT()
	public:
		StaticMeshComponent() = default;
		~StaticMeshComponent() override = default;

		PLU_PROPERTY(Setter=SetStaticMesh, Getter=GetStaticMesh)
		TUsePointer<StaticMesh> StaticMeshToDisplay;

		PLU_PROPERTY()
		TUsePointer<MaterialInfo> Material;

		BoundingBox MeshBoundingBox;
		// Twardy guard: CreateBoundingBoxForStaticMesh chodzi po każdym wierzchołku, więc liczymy
		// raz (przy SetStaticMesh, jeśli mesh już załadowany, albo leniwie w RenderSnapshotBuilder,
		// gdy dojedzie asynchronicznie), nigdy per klatka.
		bool MeshBoundingBoxComputed = false;

		PLU_PROPERTY(PyExport)
		bool CastsShadow = true;

		PLU_FUNCTION(PyExport)
		TUsePointer<StaticMesh> GetStaticMesh();
		PLU_FUNCTION(PyExport)
		void SetStaticMesh(TUsePointer<StaticMesh> staticMesh);

		PLU_FUNCTION(PyExport)
		TUsePointer<MaterialInfo> GetMaterial();
		PLU_FUNCTION(PyExport)
		void SetMaterial(TUsePointer<MaterialInfo> material);

		//Rendering
		Matrix4 GetRenderMatrix();

	protected:
		// Only meaningful for meshes that carry collision shapes — those are baked into the owning
		// object's compound shape at this component's relative transform.
		void OnRelativeTransformChanged() override;
	};
}

#endif //PLUENGINE_STATICMESHCOMPONENT_H
