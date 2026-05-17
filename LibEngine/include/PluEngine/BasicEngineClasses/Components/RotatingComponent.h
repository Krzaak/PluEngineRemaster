//
// Created by Plutex on 2026-02-11.
//

#ifndef PLUENGINE_ROTATINGCOMPONENT_H
#define PLUENGINE_ROTATINGCOMPONENT_H
#include "PluEngine/Core.h"
#include "PluEngine/GameObject/GameObjectComponent.h"
#include "RotatingComponent.generated.h"

namespace Plu
{
	PLU_CLASS(PyExport)
	class PLU_API RotatingComponent final : public GameObjectComponent
	{
		REFLECTION_BODY_ROTATINGCOMPONENT()
	public:
		RotatingComponent() = default;
		~RotatingComponent() override = default;

		PLU_PROPERTY(PyExport)
		bool RotateAroundYaw = false;

		PLU_PROPERTY(PyExport)
		bool RotateAroundRoll = false;

		PLU_PROPERTY(PyExport)
		bool RotateAroundPitch = false;

		PLU_PROPERTY(PyExport)
		float RotateYawSpeed = 0.1f;

		PLU_PROPERTY(PyExport)
		float RotatePitchSpeed = 0.1f;

		PLU_PROPERTY(PyExport)
		float RotateRollSpeed = 0.1f;

		void OnBeginPlay() override;
		void OnEndPlay() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_ROTATINGCOMPONENT_H
