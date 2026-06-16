//
// Created by Plutex on 2026-02-19.
//

#ifndef PLUENGINE_SPECTATORPUPPET_H
#define PLUENGINE_SPECTATORPUPPET_H
#include "PluEngine/Core.h"
#include "PluEngine/GameCore/Puppet.h"
#include "SpectatorPuppet.generated.h"

namespace Plu
{
	class CameraComponent;
	PLU_CLASS(PyExport, PyDerive)
	class PLU_API SpectatorPuppet : public Puppet
	{
		REFLECTION_BODY_SPECTATORPUPPET()
	private:
		Vec3 mDirection;
		bool mSprinting = false;
	public:
		SpectatorPuppet() = default;
		~SpectatorPuppet() override = default;

		PLU_PROPERTY()
		float MovementSpeed = 15.f;

		PLU_PROPERTY()
		float SprintSpeedMultiplier = 3.f;

		PLU_PROPERTY()
		float MouseSensitivity = 1.f;

		TUsePointer<CameraComponent> Camera;

		void OnSetupComponents() override;
		void OnUpdate(float deltaTime) override;
		void OnBeginPlay() override;
	};
}

#endif //PLUENGINE_SPECTATORPUPPET_H
