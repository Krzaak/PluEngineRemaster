//
// Created by Plutex on 1/13/26.
//

#include "MaterialDetailsPanel.h"

#include "MaterialViewport.h"

void Plu::MaterialInfoViewport::OnClosed()
{
}

void Plu::MaterialInfoViewport::OnOpened()
{
	AddPanel(MaterialDetailsPanel::GetStaticClass(), false);
}

void Plu::MaterialInfoViewport::OnPanelRegister()
{
}

void Plu::MaterialInfoViewport::OnUpdate(float deltaTime)
{
	if (BeginWindow()) {
		UpdatePanels(deltaTime);
	}
	EndWindow();
}
