//
// Created by Plutex on 2026-03-05.
//

#ifndef PLUENGINE_PRIMITIVERENDERABLE_H
#define PLUENGINE_PRIMITIVERENDERABLE_H
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "PrimitiveRenderable.generated.h"
#include "RenderingInterfaces.h"

namespace Plu
{
	struct StaticMesh;
	struct MaterialInfo;
	PLU_CLASS(Abstract)
	class PLU_API PrimitiveRenderable : public EngineObject, public IRenderable
	{
		REFLECTION_BODY_PRIMITIVERENDERABLE()
	private:
		TUsePointer<MaterialInfo> mMaterial;
		TUsePointer<StaticMesh> mStaticMesh;
		Vec3 mLocation;
		Vec3 mScale;
		Vec3 mRotation;
	public:
		PrimitiveRenderable(const TUsePointer<MaterialInfo> &material, const TUsePointer<StaticMesh> &mesh, const Vec3 loc = Vec3(0), const Vec3 rot = Vec3(0), const Vec3 scale = Vec3(1))
		{
			mLocation = loc;
			mMaterial = material;
			mRotation = rot;
			mScale = scale;
			mStaticMesh = mesh;
		}
		~PrimitiveRenderable() override = default;

		void SetMaterial(const TUsePointer<MaterialInfo> &material) {mMaterial = material;}
		void SetLocation(Vec3 newLoc) {mLocation = newLoc;}

		MaterialInfo *GetMaterialInfoToRender() override {return mMaterial.GetRaw();}
		StaticMesh *GetStaticMeshToRender() override {return mStaticMesh.GetRaw();}
		Vec3 GetRenderLocation() override {return mLocation;}
		Vec3 GetRenderRotation() override {return mRotation;}
		Vec3 GetRenderScale() override {return mScale;}
		EngineObjectHandle *GetRenderableObjectHandle() override {return GetEngineObjectHandle();}
	};
}

#endif //PLUENGINE_PRIMITIVERENDERABLE_H
