//
// Created by Plutex on 2026-06-14.
//

#include "PluEngine/BasicEngineClasses/GameObjects/CharacterPuppet.h"

#include "PluEngine/BasicEngineClasses/Components/CameraComponent.h"
#include "PluEngine/BasicEngineClasses/Components/PhysicsCapsuleComponent.h"
#include "PluEngine/GameCore/Controller.h"
#include "PluEngine/Physics/PhysicsWorld.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/Scenes/SceneWorld.h"

void Plu::CharacterPuppet::OnSetupComponents()
{
	Camera = AddComponent(CameraComponent::GetStaticClass(), "CharacterCamera");
	Camera->SetRelativeLocation(Vec3(0.f, CameraHeightOffset, 0.f));

	mCapsule = AddComponent(PhysicsCapsuleComponent::GetStaticClass(), "CharacterCapsule");
	ActiveBody = true; // simulate this character's body (now a per-object property)
	mCapsule->CapsuleRadius = CapsuleRadius;
	mCapsule->CapsuleHalfHeight = CapsuleHalfHeight;

	GetInputHandler()->AddActionOnHold(Key::W, [this]()
	{
		mMoveInput += GetObjectForwardVector();
		mMovingForward = true;
	});
	GetInputHandler()->AddActionOnHold(Key::S, [this](){ mMoveInput -= GetObjectForwardVector(); });
	GetInputHandler()->AddActionOnHold(Key::A, [this](){ mMoveInput -= GetObjectRightVector(); });
	GetInputHandler()->AddActionOnHold(Key::D, [this](){ mMoveInput += GetObjectRightVector(); });
	GetInputHandler()->AddActionOnHold(Key::LeftShift, [this](){ mSprinting = true; });
	GetInputHandler()->AddActionOnPress(Key::Space, [this](){ mWantsJump = true; });
}

void Plu::CharacterPuppet::OnBeginPlay()
{
	GetController()->HideCursor();
}

bool Plu::CharacterPuppet::CheckGrounded()
{
	float checkDist = CapsuleHalfHeight + CapsuleRadius + 0.15f;
	RaycastHit hit = GetWorld()->GetPhysicsWorld()->Raycast(
		GetObjectLocation(), Vec3(0.f, -1.f, 0.f), checkDist);
	return hit.Hit && hit.HitObject != nullptr && hit.HitObject != this;
}

void Plu::CharacterPuppet::OnUpdate(float deltaTime)
{
	float pitch = ClampAngle(GetInputHandler()->GetMouseDeltaY() * MouseSensitivity
		+ GetController()->GetControlRotation().x, -89.9f, 89.9f);
	Vec3 newRot = Vec3(pitch,
		GetInputHandler()->GetMouseDeltaX() * -1.f * MouseSensitivity + GetController()->GetControlRotation().y,
		0.f);
	GetController()->SetControlRotation(newRot);

	SetObjectRotation(Vec3(0.f, GetController()->GetControlRotation().y * -1.f, 0.f));
	Camera->SetRelativeRotation(Vec3(GetController()->GetControlRotation().x, 0.f, 0.f));

	mIsGrounded = CheckGrounded();

	Vec3 currentVel = mCapsule->GetLinearVelocity();

	Vec3 hVel = Vec3(0.f);
	if (mMoveInput != Vec3(0.f))
	{
		bool canSprint = mSprinting && (!SprintForwardOnly || mMovingForward);
		float speed = WalkSpeed * (canSprint ? SprintSpeedMultiplier : 1.f)
			* (mIsGrounded ? 1.f : AirControl);
		hVel = glm::normalize(mMoveInput) * speed;
	}

	float yVel = (mWantsJump && mIsGrounded) ? JumpForce : currentVel.y;
	mCapsule->SetLinearVelocity(Vec3(hVel.x, yVel, hVel.z));
	mCapsule->SetAngularVelocity(Vec3(0.f));

	mMoveInput = Vec3(0.f);
	mSprinting = false;
	mWantsJump = false;
	mMovingForward = false;
}
