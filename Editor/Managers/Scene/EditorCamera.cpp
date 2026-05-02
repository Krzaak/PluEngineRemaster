//
// Created by Plutex on 2026-03-26.
//

#include "EditorCamera.h"

#include "EditorAppContext.h"
#include "EditorSettings/EditorSettingsManager.h"
#include "PluEngine/Application.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/Input/InputManager.h"
#include "PluEngine/Window/Window.h"

extern Plu::EditorAppContext* gEditorAppContext;
extern Plu::ApplicationInfo* gApplicationInfo;

#define InputBackend gApplicationInfo->AppInputManager->GetInputBackend()

Plu::EditorSceneCamera::EditorSceneCamera()
{
	mCameraOptions.CameraPerspective = PerspectiveType::Perspective;
	mCameraOptions.FieldOfView = 50;

	mLocation = mNiceRotation = mRotation = Vec3(0);
}

void Plu::EditorSceneCamera::OnUpdate(float deltaTime)
{
	Vec3 move = Vec3(0);
	EditorSettings* settings = EditorSettingsManager::GetInstance()->GetSettings();
	if (InputBackend->GetKeyboard().IsDown(Key::W)) move += GetForwardVector(mRotation) * settings->EditorCameraMoveSpeedMultiplier;
	if (InputBackend->GetKeyboard().IsDown(Key::S)) move -= GetForwardVector(mRotation) * settings->EditorCameraMoveSpeedMultiplier;
	if (InputBackend->GetKeyboard().IsDown(Key::A)) move -= GetRightVector(mRotation) * settings->EditorCameraMoveSpeedMultiplier;
	if (InputBackend->GetKeyboard().IsDown(Key::D)) move += GetRightVector(mRotation) * settings->EditorCameraMoveSpeedMultiplier;
	move += InputBackend->GetMouse().scrollY * GetForwardVector(mRotation) * 10.0f * settings->EditorCameraScrollWheelSpeedMultiplier;
	if (InputBackend->GetMouse().IsDown(MouseButton::Middle)) {
		gApplicationInfo->AppWindow->SetCursorPosition(IVec2(-InputBackend->GetMouse().deltaX, -InputBackend->GetMouse().deltaY) + gApplicationInfo->AppWindow->GetCursorPosition());
		move += -InputBackend->GetMouse().deltaY * GetUpVector(mRotation) * settings->EditorCameraPanSpeedMultiplier;
		move += InputBackend->GetMouse().deltaX * GetRightVector(mRotation) * settings->EditorCameraPanSpeedMultiplier;
	}
	mLocation += move * mMoveSpeed * deltaTime;
	if (InputBackend->GetMouse().IsDown(MouseButton::Right)) {
		gApplicationInfo->AppWindow->SetCursorPosition(IVec2(-InputBackend->GetMouse().deltaX, -InputBackend->GetMouse().deltaY) + gApplicationInfo->AppWindow->GetCursorPosition());
		float pitch = ClampAngle(InputBackend->GetMouse().deltaY * settings->EditorCameraLookSpeedMultiplier * 0.3f + mNiceRotation.x, -89.9,89.9);
		mNiceRotation = Vec3(pitch, -InputBackend->GetMouse().deltaX * settings->EditorCameraLookSpeedMultiplier * 0.3f + mNiceRotation.y, 0.0f);
		NormalizeVec3Rotation(&mNiceRotation);
		Vec3 newPoint = GetSphericalOrbitPoint(mLocation, 5, mNiceRotation.y, mNiceRotation.x);
		mRotation = GetLookAtRotatorDegrees(mLocation, newPoint);
	}
}

void Plu::EditorSceneCamera::SetLocation(Vec3 newLocation)
{
	mLocation = newLocation;
}

void Plu::EditorSceneCamera::SetRotation(Vec3 newRotation)
{
	mNiceRotation = newRotation;
	NormalizeVec3Rotation(&mNiceRotation);
	Vec3 newPoint = GetSphericalOrbitPoint(mLocation, 5, mNiceRotation.y, mNiceRotation.x);
	mRotation = GetLookAtRotatorDegrees(mLocation, newPoint);
}

Vec3 Plu::EditorSceneCamera::GetCameraLocation()
{
	return mLocation;
}

Plu::CameraOptions * Plu::EditorSceneCamera::GetCameraOptions()
{
	return &mCameraOptions;
}

Vec3 Plu::EditorSceneCamera::GetCameraRotation()
{
	return mRotation;
}

Vec3 Plu::EditorSceneCamera::GetNiceRotation()
{
	return mNiceRotation;
}
