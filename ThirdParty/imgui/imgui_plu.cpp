#include "imgui_plu.h"

#include "imgui.h"

void ImGui::Text(const Plu::String& text)
{
    TextUnformatted(text.CStr());
}
