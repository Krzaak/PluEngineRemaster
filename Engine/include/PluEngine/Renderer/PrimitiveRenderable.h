//
// Created by Plutex on 2026-03-05.
//

#ifndef PLUENGINE_PRIMITIVERENDERABLE_H
#define PLUENGINE_PRIMITIVERENDERABLE_H
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "PrimitiveRenderable.generated.h"
#include "RenderingInterfaces.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

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

		Matrix4 mWorldMatrix;
		bool mRegenerateWorldMatrix = true;
	public:
		PrimitiveRenderable(const TUsePointer<MaterialInfo> &material, const TUsePointer<StaticMesh> &mesh, const Vec3 loc = Vec3(0), const Vec3 rot = Vec3(0), const Vec3 scale = Vec3(1))
		{
			mLocation = loc;
			mMaterial = material;
			mRotation = rot;
			mScale = scale;
			mStaticMesh = mesh;
			mRegenerateWorldMatrix = true;
		}
		~PrimitiveRenderable() override = default;

		void SetMaterial(const TUsePointer<MaterialInfo> &material) {mMaterial = material;}
		void SetLocation(Vec3 newLoc)
		{
			mLocation = newLoc;
			mRegenerateWorldMatrix = true;
		}
		void SetRotation(Vec3 newRot)
		{
			mRotation = newRot;
			mRegenerateWorldMatrix = true;
		}
		void SetScale(Vec3 newScale)
		{
			mScale = newScale;
			mRegenerateWorldMatrix = true;
		}

		MaterialInfo *GetMaterialInfoToRender() override {return mMaterial.GetRaw();}
		StaticMesh *GetStaticMeshToRender() override {return mStaticMesh.GetRaw();}
		Matrix4 GetRenderMatrix() override
		{
			if (mRegenerateWorldMatrix) {
				mRegenerateWorldMatrix = false;
				Matrix4 model = glm::translate(glm::mat4(1.0f), mLocation) *
						  glm::mat4_cast(glm::quat(glm::radians(mRotation))) *
						  glm::scale(glm::mat4(1.0f), mScale);
				mWorldMatrix = model;
			}
			return mWorldMatrix;
		}
		EngineObjectHandle *GetRenderableObjectHandle() override {return GetEngineObjectHandle();}
	};
}

#endif //PLUENGINE_PRIMITIVERENDERABLE_H
