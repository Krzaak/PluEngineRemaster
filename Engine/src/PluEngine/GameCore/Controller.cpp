//
// Created by Plutex on 2026-02-14.
//

#include "PluEngine/GameCore/Controller.h"

#include "PluEngine/Application.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/BasicEngineClasses/Components/CameraComponent.h"
#include "PluEngine/GameCore/GameClient.h"
#include "PluEngine/GameCore/Puppet.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Renderer/Renderer.h"

Plu::InputHandler * Plu::Controller::GetInputHandler()
{
	return &mControllerInputHandler;
}

void Plu::Controller::SetControlRotation(Vec3 newRot)
{
	NormalizeVec3Rotation(&newRot);
	mControlRotation = newRot;
	Vec3 newPoint = GetSphericalOrbitPoint(Vec3(0), 5, newRot.y, newRot.x);
	mRealControlRotation = GetLookAtRotatorDegrees(Vec3(0), newPoint);
}

Vec3 Plu::Controller::GetControlRotation() const
{
	return mControlRotation;
}

Vec3 Plu::Controller::GetControlRotationForPuppet() const
{
	return mRealControlRotation;
}

Plu::Controller::~Controller()
{
	Unpossess();
}

void Plu::Controller::Possess(TUsePointer<Puppet> puppet)
{
	PLU_ASSERT(puppet, "Puppet is invalid on Possess!")
	mPossessedPuppet = puppet;
	puppet->mController = This();
	puppet->OnPossessed(This());
	GetWorld()->mRenderer->SetCamera(dynamic_cast<IRendererCamera *>(puppet->GetActivatedComponentByClass(CameraComponent::GetStaticClass()).GetRaw()));
}

void Plu::Controller::Unpossess()
{
	if (!mPossessedPuppet) return;
	mPossessedPuppet->OnUnpossessed();
	mPossessedPuppet = nullptr;
}

bool Plu::Controller::IsCursorShown() const
{
	return GetGameClient()->IsCursorShown();
}

void Plu::Controller::HideCursor()
{
	GetGameClient()->HideCursor();
}

void Plu::Controller::ShowCursor()
{
	GetGameClient()->ShowCursor();
}

Plu::TUsePointer<Plu::Puppet> Plu::Controller::GetControllerPuppet()
{
	return mPossessedPuppet;
}
