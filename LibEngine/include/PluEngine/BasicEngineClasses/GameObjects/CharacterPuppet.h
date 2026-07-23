//
// Created by Plutex on 2026-06-14.
//

#ifndef PLUENGINE_CHARACTERPUPPET_H
#define PLUENGINE_CHARACTERPUPPET_H
#include "PluEngine/Core.h"
#include "PluEngine/GameCore/Puppet.h"
#include "CharacterPuppet.generated.h"

namespace Plu
{
	class CameraComponent;
	class PhysicsCapsuleComponent;

	PLU_CLASS(PyExport, PyDerive)
	class PLU_API CharacterPuppet : public Puppet
	{
		REFLECTION_BODY_CHARACTERPUPPET()
	private:
		Vec3 mMoveInput;
		bool mSprinting = false;
		bool mWantsJump = false;
		bool mIsGrounded = false;
		bool mMovingForward = false;

		TUsePointer<PhysicsCapsuleComponent> mCapsule;

		bool CheckGrounded();

	public:
		CharacterPuppet() = default;
		~CharacterPuppet() override = default;

		PLU_PROPERTY(PyExport)
		float WalkSpeed = 5.f;

		PLU_PROPERTY(PyExport)
		float SprintSpeedMultiplier = 2.f;

		PLU_PROPERTY(PyExport)
		float JumpForce = 8.f;

		PLU_PROPERTY(PyExport)
		float MouseSensitivity = 1.f;

		PLU_PROPERTY(PyExport)
		float CapsuleRadius = 0.4f;

		PLU_PROPERTY(PyExport)
		float CapsuleHalfHeight = 0.9f;

		PLU_PROPERTY(PyExport)
		float CameraHeightOffset = 0.75f;

		PLU_PROPERTY(PyExport)
		float AirControl = 0.3f;

		PLU_PROPERTY(PyExport)
		bool SprintForwardOnly = false;

		TUsePointer<CameraComponent> Camera;

		PLU_FUNCTION(PyExport)
		TUsePointer<CameraComponent> GetCamera();

		Vec3 GetSpawnOffset() const override;

		void OnSetupComponents() override;
		void OnBeginPlay() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_CHARACTERPUPPET_H
