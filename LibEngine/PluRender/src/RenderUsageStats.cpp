//
// Created by Plutex on 2026-07-03.
//

#include "PluEngine/Render/RenderUsageStats.h"

namespace Plu {

    RenderUsageStats* RenderUsageStats::GetInstance()
    {
        // Function-local static: gwarantowana pojedyncza, thread-safe inicjalizacja.
        static RenderUsageStats instance;
        return &instance;
    }

    void RenderUsageStats::BeginFrame()
    {
        for (auto& pair : mMeshes) {
            pair.second.LastFrameUses = pair.second.CurrentFrameUses;
            pair.second.CurrentFrameUses = 0;
        }
        for (auto& pair : mSkeletalMeshes) {
            pair.second.LastFrameUses = pair.second.CurrentFrameUses;
            pair.second.CurrentFrameUses = 0;
        }
        for (auto& pair : mTextures) {
            pair.second.LastFrameUses = pair.second.CurrentFrameUses;
            pair.second.CurrentFrameUses = 0;
        }
    }

    void RenderUsageStats::RecordMesh(UInt64 meshUuid, UInt32 count)
    {
        if (meshUuid == 0 || count == 0) return;
        AssetUsageEntry& entry = mMeshes[meshUuid]; // tworzy domyślny wpis, jeśli brak
        entry.CurrentFrameUses += count;
        entry.TotalUses += count;
    }

    void RenderUsageStats::RecordSkeletalMesh(UInt64 meshUuid, UInt32 count)
    {
        if (meshUuid == 0 || count == 0) return;
        AssetUsageEntry& entry = mSkeletalMeshes[meshUuid]; // tworzy domyślny wpis, jeśli brak
        entry.CurrentFrameUses += count;
        entry.TotalUses += count;
    }

    void RenderUsageStats::RecordTexture(UInt64 textureUuid, UInt32 count)
    {
        if (textureUuid == 0 || count == 0) return;
        AssetUsageEntry& entry = mTextures[textureUuid];
        entry.CurrentFrameUses += count;
        entry.TotalUses += count;
    }

    void RenderUsageStats::Clear()
    {
        mMeshes.Clear();
        mSkeletalMeshes.Clear();
        mTextures.Clear();
    }

}
