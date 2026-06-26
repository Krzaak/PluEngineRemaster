//
// Created by Plutex on 2026-06-25.
//

#ifndef PLUENGINE_IMGUIDRAWSNAPSHOT_H
#define PLUENGINE_IMGUIDRAWSNAPSHOT_H

#include "imgui.h"

namespace Plu
{
    // Deep copy of one ImGui frame, handed from the Main thread (which builds the
    // UI) to the Render thread (which owns the GL context and submits
    // ImGui_ImplOpenGL3_RenderDrawData) through a TripleBuffer - same handoff
    // pattern as RenderSnapshot.
    //
    // Why a deep copy is required:
    //  - ImDrawData::CmdLists point to ImDrawList objects owned by ImGuiContext and
    //    recycled on the next ImGui::NewFrame(). The render thread reads with up to
    //    one frame of latency, so the lists must be cloned (ImDrawList::CloneOutput).
    //  - ImDrawData::Textures points to ImGui::GetPlatformIO().Textures, a vector
    //    the main thread rebuilds every frame in UpdateTexturesEndFrame()
    //    (resize(0) + repopulate). Reading that vector from the render thread would
    //    race on its storage, so we copy the pointer list into snapshot-owned
    //    memory. The ImTextureData objects themselves are stable for the program
    //    lifetime; with a static font atlas they reach ImTextureStatus_OK after the
    //    first upload and are no longer mutated by either side.
    struct ImGuiDrawSnapshot
    {
        ImDrawData DrawData;                 // CmdLists here are OUR clones (we own/free them)
        ImVector<ImTextureData*> Textures;   // snapshot-owned copy of the texture pointer list

        // Free the cloned draw lists and reset for reuse in the triple buffer slot.
        void Clear()
        {
            for (ImDrawList* list : DrawData.CmdLists)
                IM_DELETE(list);
            DrawData.CmdLists.clear();
            Textures.clear();
            DrawData.Valid = false;
        }

        ~ImGuiDrawSnapshot()
        {
            Clear();
        }

        // Deep-copy from live ImGui draw data. Call right after ImGui::Render(),
        // on the thread that built the frame.
        void CopyFrom(const ImDrawData* src)
        {
            Clear();
            DrawData.Valid            = src->Valid;
            DrawData.CmdListsCount    = src->CmdListsCount;
            DrawData.TotalIdxCount    = src->TotalIdxCount;
            DrawData.TotalVtxCount    = src->TotalVtxCount;
            DrawData.DisplayPos       = src->DisplayPos;
            DrawData.DisplaySize      = src->DisplaySize;
            DrawData.FramebufferScale = src->FramebufferScale;
            DrawData.OwnerViewport    = nullptr;

            DrawData.CmdLists.reserve(src->CmdLists.Size);
            for (ImDrawList* list : src->CmdLists)
                DrawData.CmdLists.push_back(list->CloneOutput());

            if (src->Textures != nullptr)
                for (ImTextureData* tex : *src->Textures)
                    Textures.push_back(tex);
            DrawData.Textures = &Textures;
        }
    };
}

#endif //PLUENGINE_IMGUIDRAWSNAPSHOT_H
