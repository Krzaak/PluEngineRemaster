//
// Created by Plutex on 2026-01-30.
//

#ifndef PLUENGINE_RGBTRANSFORMDRAGGER_H
#define PLUENGINE_RGBTRANSFORMDRAGGER_H

#include "imgui.h"
#include "imgui_internal.h"

static const ImGuiDataTypeInfo GDataTypeInfo[] =
{
	{ sizeof(char),             "S8",   "%d",   "%d"    },  // ImGuiDataType_S8
	{ sizeof(unsigned char),    "U8",   "%u",   "%u"    },
	{ sizeof(short),            "S16",  "%d",   "%d"    },  // ImGuiDataType_S16
	{ sizeof(unsigned short),   "U16",  "%u",   "%u"    },
	{ sizeof(int),              "S32",  "%d",   "%d"    },  // ImGuiDataType_S32
	{ sizeof(unsigned int),     "U32",  "%u",   "%u"    },
#ifdef _MSC_VER
	{ sizeof(ImS64),            "S64",  "%I64d","%I64d" },  // ImGuiDataType_S64
	{ sizeof(ImU64),            "U64",  "%I64u","%I64u" },
#else
	{ sizeof(ImS64),            "S64",  "%lld", "%lld"  },  // ImGuiDataType_S64
	{ sizeof(ImU64),            "U64",  "%llu", "%llu"  },
#endif
	{ sizeof(float),            "float", "%.3f","%f"    },  // ImGuiDataType_Float (float are promoted to double in va_arg)
	{ sizeof(double),           "double","%f",  "%lf"   },  // ImGuiDataType_Double
	{ sizeof(bool),             "bool", "%d",   "%d"    },  // ImGuiDataType_Bool
	{ 0,                        "char*","%s",   "%s"    },  // ImGuiDataType_String
};

inline bool RGBTransformDrag3(const char* label, void* p_data, int components, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	bool value_changed = false;
	ImGui::BeginGroup();
	ImGui::PushID(label);
	ImGui::PushMultiItemsWidths(components, ImGui::CalcItemWidth());
	size_t type_size = GDataTypeInfo[ImGuiDataType_Float].Size;
	for (int i = 0; i < components; i++)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.12f,0.12f,0.12f,1));
		if (i == 0)
		{
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.5f,0.1f,0.1f,0.5f));
		} else if (i ==1)
		{
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f,0.5f,0.1f,0.5f));
		} else if (i == 2)
		{
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f,0.1f,0.5f,0.5f));
		}
		ImGui::PushID(i);
		if (i > 0)
			ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);
		value_changed |= ImGui::DragScalar("", ImGuiDataType_Float, p_data, v_speed, p_min, p_max, format, flags);
		ImGui::PopID();
		ImGui::PopItemWidth();
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();
		p_data = (void*)((char*)p_data + type_size);
	}
	ImGui::PopID();

	const char* label_end = ImGui::FindRenderedTextEnd(label);
	if (label != label_end)
	{
		ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);
		ImGui::TextEx(label, label_end);
	}

	ImGui::EndGroup();
	return value_changed;
}

#endif //PLUENGINE_RGBTRANSFORMDRAGGER_H