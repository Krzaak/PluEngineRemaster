//
// Created by Plutex on 1/19/26.
//

#ifndef PLUENGINE_CAMERACOMPONENT_H
#define PLUENGINE_CAMERACOMPONENT_H
#include "PluEngine/GameObject/WorldComponent.h"
#include "CameraComponent.generated.h"

namespace Plu
{
	enum class PerspectiveType
	{
		Perspective,
		Orthographic
	};

	PLU_STRUCT()
	struct CameraOptions
	{
		REFLECTION_BODY_CAMERAOPTIONS()

		PerspectiveType CameraPerspective;

		PLU_PROPERTY()
		float OrthoWidth = 100;

		PLU_PROPERTY()
		float FieldOfView = 45;
	};

	PLU_CLASS(PyExport)
	class PLU_API CameraComponent final : public WorldComponent
	{
		REFLECTION_BODY_CAMERACOMPONENT()
	public:
		CameraComponent() = default;
		~CameraComponent() override = default;

		void OnUpdate(float deltaTime) override;
		void OnBeginPlay() override;
		void OnEndPlay() override;

		PLU_PROPERTY()
		CameraOptions Options;
	};
}

#endif //PLUENGINE_CAMERACOMPONENT_H