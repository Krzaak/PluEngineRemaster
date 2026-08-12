//
// Created by Plutex on 5/30/26.
//

#include "ProjectSettingsPanel.h"

#include "EditorAppContext.h"
#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/PluGame.h"
#include "PluEngine/Core/CollisionChannels.h"
#include "PluEngine/Core/Reflection/TypeTraits.h"
#include "UI/IconsFontAwesome7.h"
#include "String/String.h"
#include "Array/Array.h"
#include <cstdio>
#include <cfloat>

Plu::String Plu::ProjectSettingsPanel::GetPanelName()
{
    return ICON_FA_GEARS " Project Settings";
}

void Plu::ProjectSettingsPanel::OnHide()
{
}

void Plu::ProjectSettingsPanel::OnShow()
{
    SetCanClose(true);
}

void Plu::ProjectSettingsPanel::OnUpdate(float deltaTime)
{
    if (BeginPanel()) {
        if (!mEditorAppContext->EditorProjectManager->IsAnyProjectOpen())
        {
            ImGui::Text("Open Project before browsing settings!");
            EndPanel();
            return;
        }
        TypeSerializer<TypeInfo*>::EditorControl(GameStartupSettings::GetStaticClass(), mEditorAppContext->EditorProjectManager->GetGameStartupSettings().GetRaw());
        DrawCollisionSettings();
    }
    EndPanel();
}

namespace
{
    // Tint per response (Ignore / Overlap / Block) — reused for headers + radio checkmarks.
    const ImVec4 kResponseColors[3] = {
        ImVec4(0.78f, 0.42f, 0.42f, 1.0f), // Ignore  — muted red
        ImVec4(0.90f, 0.78f, 0.34f, 1.0f), // Overlap — amber
        ImVec4(0.45f, 0.80f, 0.48f, 1.0f), // Block   — green
    };
    const char* kResponseNames[3] = { "Ignore", "Overlap", "Block" };
}

void Plu::ProjectSettingsPanel::DrawCollisionSettings()
{
    CollisionConfig& cfg = ActiveCollisionConfig();

    ImGui::Spacing();
    if (!ImGui::CollapsingHeader(ICON_FA_SHIELD_HALVED "  Collision", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    const UInt32 channelCount = cfg.ChannelNames.Size();
    if (mSelectedPreset >= static_cast<int>(cfg.Profiles.Size()))
        mSelectedPreset = static_cast<int>(cfg.Profiles.Size()) - 1;
    if (mSelectedPreset < 0)
        mSelectedPreset = 0;

    const float paneHeight = 230.0f;
    const float listWidth  = 190.0f;

    // ---- Left pane: preset list ----
    ImGui::BeginChild("##presetList", ImVec2(listWidth, paneHeight), true);
    ImGui::SeparatorText("Presets");
    for (UInt32 p = 0; p < cfg.Profiles.Size(); ++p)
    {
        ImGui::PushID(static_cast<int>(p));
        if (ImGui::Selectable(cfg.Profiles[p].Name.CStr(), mSelectedPreset == static_cast<int>(p)))
            mSelectedPreset = static_cast<int>(p);
        ImGui::PopID();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // ---- Right pane: selected preset details ----
    ImGui::BeginChild("##presetDetails", ImVec2(0, paneHeight), true);
    if (mSelectedPreset >= 0 && mSelectedPreset < static_cast<int>(cfg.Profiles.Size()) && channelCount > 0)
    {
        CollisionProfile& prof = cfg.Profiles[mSelectedPreset];
        const bool isFallback = (mSelectedPreset == 0);

        char nameBuf[64];
        std::snprintf(nameBuf, sizeof(nameBuf), "%s", prof.Name.CStr());
        ImGui::SetNextItemWidth(220.0f);
        ImGui::BeginDisabled(isFallback); // keep the "Default" fallback name stable
        if (ImGui::InputText("Preset Name", nameBuf, sizeof(nameBuf)))
            prof.Name = String(nameBuf);
        ImGui::EndDisabled();

        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::BeginCombo("Object Type",
                              prof.ObjectType < channelCount ? cfg.ChannelNames[prof.ObjectType].CStr() : "?"))
        {
            for (UInt32 c = 0; c < channelCount; ++c)
                if (ImGui::Selectable(cfg.ChannelNames[c].CStr(), prof.ObjectType == c))
                    prof.ObjectType = static_cast<UInt8>(c);
            ImGui::EndCombo();
        }

        ImGui::Spacing();
        ImGui::SeparatorText("Collision Responses");

        // Column layout: channel label on the left, three response columns right-aligned.
        const float colW = 78.0f;
        const float radioInset = 22.0f; // center the bare radio under its column header
        const float availW = ImGui::GetContentRegionAvail().x;
        float base = availW - colW * 3.0f;
        if (base < 130.0f) base = 130.0f;

        // Header row
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Channel");
        for (int r = 0; r < 3; ++r)
        {
            ImGui::SameLine(base + colW * r);
            ImGui::TextColored(kResponseColors[r], "%s", kResponseNames[r]);
        }

        // One row per channel with three radio buttons.
        for (UInt32 c = 0; c < channelCount; ++c)
        {
            ImGui::PushID(static_cast<int>(c));
            int resp = (c < prof.ResponseTo.Size()) ? static_cast<int>(prof.ResponseTo[c]) : 2;

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(cfg.ChannelNames[c].CStr());
            for (int r = 0; r < 3; ++r)
            {
                ImGui::SameLine(base + colW * r + radioInset);
                ImGui::PushID(r);
                ImGui::PushStyleColor(ImGuiCol_CheckMark, kResponseColors[r]);
                if (ImGui::RadioButton("##resp", resp == r) && c < prof.ResponseTo.Size())
                    prof.ResponseTo[c] = static_cast<CollisionResponse>(r);
                ImGui::PopStyleColor();
                ImGui::PopID();
            }
            ImGui::PopID();
        }
    }
    else
    {
        ImGui::TextDisabled("No preset selected.");
    }
    ImGui::EndChild();

    // ---- Preset add/remove (under the list) ----
    if (ImGui::Button(ICON_FA_PLUS " Add Preset"))
    {
        CollisionProfile prof;
        prof.Name = String("NewPreset");
        prof.ObjectType = 0;
        for (UInt32 c = 0; c < channelCount; ++c)
            prof.ResponseTo.PushBack(CollisionResponse::Block);
        cfg.Profiles.PushBack(prof);
        mSelectedPreset = static_cast<int>(cfg.Profiles.Size()) - 1;
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(mSelectedPreset <= 0); // index 0 is the mandatory fallback
    if (ImGui::Button(ICON_FA_TRASH " Remove Preset"))
    {
        cfg.Profiles.RemoveAt(static_cast<UInt32>(mSelectedPreset));
        mSelectedPreset = 0;
    }
    ImGui::EndDisabled();

    ImGui::TextDisabled("Combined response = weaker of both directions (Ignore < Overlap < Block).");

    // ---- Channels (object types) — folded away by default ----
    ImGui::Spacing();
    if (ImGui::TreeNode(ICON_FA_LAYER_GROUP " Edit Channels"))
    {
        int channelToRemove = -1;
        for (UInt32 i = 0; i < cfg.ChannelNames.Size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%s", cfg.ChannelNames[i].CStr());
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::InputText("##channelName", buf, sizeof(buf)))
                cfg.ChannelNames[i] = String(buf);
            ImGui::SameLine();
            ImGui::BeginDisabled(cfg.ChannelNames.Size() <= 1);
            if (ImGui::SmallButton(ICON_FA_TRASH))
                channelToRemove = static_cast<int>(i);
            ImGui::EndDisabled();
            ImGui::PopID();
        }
        if (channelToRemove >= 0)
        {
            const UInt32 k = static_cast<UInt32>(channelToRemove);
            cfg.ChannelNames.RemoveAt(k);
            for (UInt32 p = 0; p < cfg.Profiles.Size(); ++p)
            {
                CollisionProfile& prof = cfg.Profiles[p];
                if (k < prof.ResponseTo.Size()) prof.ResponseTo.RemoveAt(k);
                if (prof.ObjectType == k)       prof.ObjectType = 0;
                else if (prof.ObjectType > k)   prof.ObjectType -= 1;
            }
            cfg.NormalizeProfiles();
        }
        if (ImGui::Button(ICON_FA_PLUS " Add Channel"))
        {
            cfg.ChannelNames.PushBack(String("NewChannel"));
            cfg.NormalizeProfiles();
        }
        ImGui::TreePop();
    }
}
