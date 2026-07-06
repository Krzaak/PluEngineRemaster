//
// Created by Plutex on 2026-07-03.
//

#ifndef PLUENGINE_RENDERUSAGESTATS_H
#define PLUENGINE_RENDERUSAGESTATS_H

#include "PluEngine/Core.h"
#include "PluSTL_FWD.h"

namespace Plu {

    // Pojedynczy wpis licznika użycia assetu (mesh / tekstura). Kluczem w rejestrze jest
    // surowy UInt64 UUID assetu; nazwę do wyświetlenia panel rozwiązuje przez AssetManager.
    struct AssetUsageEntry {
        UInt64 TotalUses = 0;        // suma użyć od ostatniego resetu (ranking "hottest")
        UInt32 LastFrameUses = 0;    // ile razy asset był użyty w POPRZEDNIej klatce (bieżąca liczba instancji)
        UInt32 CurrentFrameUses = 0; // akumulator dla trwającej klatki (rolowany w BeginFrame)
    };

    // Globalny rejestr "gorących" assetów renderowania: które meshe i tekstury są najczęściej
    // używane w głównym passie sceny. Plain singleton (nie EngineObject), tak jak Profiler.
    //
    // Bez synchronizacji: i zapis (RenderSnapshotBuilder), i odczyt (panel edytora) dzieją się na
    // wątku MAIN, więc dostęp jest z natury sekwencyjny. Rejestr celowo NIE liczy tekstur cieni
    // (mapy kaskad CSM) — do liczników trafiają wyłącznie tekstury materiałów, a mapy cieni są
    // ustawiane silnikowo w Rendererze, poza materiałem.
    class PLU_API RenderUsageStats {
    public:
        static RenderUsageStats* GetInstance();

        // Zamyka poprzednią klatkę (CurrentFrameUses -> LastFrameUses) i zeruje akumulator.
        // Woływane raz na klatkę, na początku budowania snapshotu.
        void BeginFrame();

        void RecordMesh(UInt64 meshUuid);
        void RecordSkeletalMesh(UInt64 meshUuid);
        void RecordTexture(UInt64 textureUuid);

        // Bezpośredni dostęp do rejestrów (odczyt na tym samym wątku co zapis — MAIN).
        const GameHashMap<UInt64, AssetUsageEntry>& GetMeshUsage() const { return mMeshes; }
        const GameHashMap<UInt64, AssetUsageEntry>& GetSkeletalMeshUsage() const { return mSkeletalMeshes; }
        const GameHashMap<UInt64, AssetUsageEntry>& GetTextureUsage() const { return mTextures; }

        // Zeruje wszystkie zebrane liczniki.
        void Clear();

    private:
        RenderUsageStats() = default;

        GameHashMap<UInt64, AssetUsageEntry> mMeshes;
        GameHashMap<UInt64, AssetUsageEntry> mSkeletalMeshes;
        GameHashMap<UInt64, AssetUsageEntry> mTextures;
    };

}

#endif //PLUENGINE_RENDERUSAGESTATS_H
