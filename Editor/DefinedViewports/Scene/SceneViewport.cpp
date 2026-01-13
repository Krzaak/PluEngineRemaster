//
// Created by Plutex on 1/13/26.
//

#include "SceneViewport.h"

void Plu::SceneViewport::OnClosed()
{
}

void Plu::SceneViewport::OnOpened()
{
}

void Plu::SceneViewport::OnPanelRegister()
{
}

void Plu::SceneViewport::OnUpdate(float deltaTime)
{
	if (BeginWindow()) {
		UpdatePanels(deltaTime);
	}
	EndWindow();
}
