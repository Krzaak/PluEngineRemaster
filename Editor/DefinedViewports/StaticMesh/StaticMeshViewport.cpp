//
// Created by Plutex on 1/13/26.
//

#include "StaticMeshViewport.h"

void Plu::StaticMeshViewport::OnClosed()
{
}

void Plu::StaticMeshViewport::OnOpened()
{
}

void Plu::StaticMeshViewport::OnPanelRegister()
{
}

void Plu::StaticMeshViewport::OnUpdate(float deltaTime)
{
	if (BeginWindow()) {
		UpdatePanels(deltaTime);
	}
	EndWindow();
}
