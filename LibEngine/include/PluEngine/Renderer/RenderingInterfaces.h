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
	struct MaterialInfo;
	struct StaticMesh;
}

namespace Plu
{
	class ShaderProgram;
}

namespace Plu
{
	PLU_ENUM(PyExport, PyNamespace=Plu)
	enum class PerspectiveType
	{
		Perspective,
		Orthographic
	};

	PLU_STRUCT()
	struct CameraOptions
	{
		REFLECTION_BODY_CAMERAOPTIONS()

		PLU_PROPERTY()
		PerspectiveType CameraPerspective;

		PLU_PROPERTY()
		float OrthoWidth = 100;

		PLU_PROPERTY()
		float FieldOfView = 45;
	};

	//Simple interfaces for Rendering stuff
	PLU_INTERFACE()
	class IRenderable
	{
	public:
		virtual ~IRenderable() = default;
		virtual MaterialInfo* GetMaterialInfoToRender() = 0;
		virtual StaticMesh* GetStaticMeshToRender() = 0;
		virtual struct EngineObjectHandle* GetRenderableObjectHandle() = 0;
		virtual Matrix4 GetRenderMatrix() = 0;
	};

	PLU_INTERFACE()
	class IRendererCamera
	{
	public:
		virtual ~IRendererCamera() = default;
		virtual CameraOptions* GetCameraOptions() = 0;
		virtual Vec3 GetCameraLocation() = 0;
		virtual Vec3 GetCameraRotation() = 0;
	};
}

#endif //PLUENGINE_RENDERINGINTERFACES_H