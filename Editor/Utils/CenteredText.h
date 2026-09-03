//
// Created by Plutex on 2026-07-02.
//

#ifndef PLUENGINE_CENTEREDTEXT_H
#define PLUENGINE_CENTEREDTEXT_H

#include "imgui.h"

// Wyśrodkowuje tekst w poziomie względem szerokości bieżącego okna.
inline void TextCentered(const char* text)
{
	float winWidth  = ImGui::GetWindowSize().x;
	float textWidth = ImGui::CalcTextSize(text).x;
	ImGui::SetCursorPosX((winWidth - textWidth) * 0.5f);
	ImGui::TextUnformatted(text);
}

// Wyśrodkowuje tekst w poziomie i pionie względem rozmiaru bieżącego okna.
inline void TextCenteredBoth(const char* text)
{
	ImVec2 win = ImGui::GetWindowSize();
	ImVec2 sz  = ImGui::CalcTextSize(text);
	ImGui::SetCursorPos(ImVec2((win.x - sz.x) * 0.5f, (win.y - sz.y) * 0.5f));
	ImGui::TextUnformatted(text);
}

#endif //PLUENGINE_CENTEREDTEXT_H
