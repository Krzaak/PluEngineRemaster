//
// Created by Plutex on 1/13/26.
//

#include "TextureDisplayPanel.h"

#include "TextureViewport.h"

void Plu::TextureViewport::OnClosed()
{
}

void Plu::TextureViewport::OnOpened()
{
	AddPanel(TextureDisplayPanel::GetStaticClass(), false);
}

void Plu::TextureViewport::OnPanelRegister()
{
}

void Plu::TextureViewport::OnUpdate(float deltaTime)
{
	if (BeginWindow()) {
		UpdatePanels(deltaTime);
	}
	EndWindow();
}
