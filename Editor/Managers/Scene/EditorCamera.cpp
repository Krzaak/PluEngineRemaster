//
// Created by Plutex on 2026-03-26.
//

#include "EditorCamera.h"

Plu::EditorSceneCamera::EditorSceneCamera()
{
	mCameraOptions.CameraPerspective = PerspectiveType::Perspective;
	mCameraOptions.FieldOfView = 50;

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
