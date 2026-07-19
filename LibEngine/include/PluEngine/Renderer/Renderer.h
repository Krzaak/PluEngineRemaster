//
// Created by Plutex on 6/22/26.
//

#ifndef PLUENGINE_RENDERER_H
#define PLUENGINE_RENDERER_H

#include "GLFrameBuffer.h"
#include "GLShaderStorageBuffer.h"
#include "HashSet/HashSet.h"
#include "PluEngine/Core.h"
#include "PluEngine/PluTypes.h"
#include "PluEngine/Objects/EngineObject.h"
#include "RenderThreading.h"
#include "RenderUtils.h"

namespace Plu
{
    class ShaderProgram;
    struct RenderSnapshot;
    struct StaticMesh;
    struct SkeletalMesh;

    //This is a render thread only object
    class Renderer
    {
        // Liczba kaskad cieni (Cascaded Shadow Maps) dla światła kierunkowego.
        // Musi się zgadzać z #define CASCADE_COUNT w Shadow.frag i PBR.frag.
        // 5 kaskad (splity sterowane lambdą w RenderShadowPass): pierwsza kończy się ~1 m od
        // kamery (teksel ~1 mm = ostre cienie małych obiektów z bliska), ostatnia zaczyna ~62 m.
        static constexpr int kCascadeCount = 5;

        // Rozdzielczość mapy głębi per kaskada. Dalekie kaskady pokrywają dziesiątki/setki
        // metrów — ich gęstość tekseli na metr jest i tak znikoma, więc niższa rozdzielczość
        // jest tam wizualnie prawie nieodróżnialna, a tnie VRAM i fillrate passu cieni.
        // Shadery (Shadow.frag/PBR.frag) czytają rozmiar przez textureSize per kaskada,
        // a texel snapping w GetCascadedLightMatrices dostaje te wartości per kaskada.
        // VRAM (D16, patrz Initialize): 32+32+8+8+2 = 82 MB (vs 320 MB przy 5x4096^2 32F).
        static constexpr Int32 kCascadeResolutions[kCascadeCount] = {4096, 4096, 2048, 2048, 1024};

        // Round-robin dalekich kaskad: kaskady od tego indeksu w górę są odświeżane co drugą
        // klatkę (naprzemiennie, max jedna daleka kaskada per klatka). Pokrywają 14-300 m,
        // więc klatka opóźnienia względem ruchu kamery/animacji jest tam niezauważalna.
        // Pominięta kaskada renderuje się w głównym passie STARĄ macierzą viewProj z
        // mCascadeCache — macierz musi odpowiadać zawartości mapy głębi, nie bieżącej kamerze.
        static constexpr int kFirstStaggeredCascade = 3;

        ApplicationInfo* mApplicationInfo;
        TOwningPointer<FrameBuffer> mMainBuffer;
        // Mapy głębi per-kaskada (DepthOnly, D16). Tworzone eager w Initialize (wątek renderu
        // posiada kontekst GL), więc na ścieżce klatki nie ma per-klatkowego CreateObject.
        DynamicArray<TOwningPointer<FrameBuffer>> mCascadeFrameBuffers;

        // Stan round-robina dalekich kaskad: macierze/splity użyte przy OSTATNIM renderze
        // każdej kaskady (do samplowania pominiętych kaskad), kierunek światła przy którym
        // cache powstał (zmiana = wymuszenie pełnego odświeżenia — stare mapy są bezużyteczne)
        // i licznik klatek passu cieni sterujący naprzemiennością. Pusty cache = pełny render.
        DynamicArray<ShadowCascadeData> mCascadeCache;
        Vec3 mCascadeCacheLightDir = Vec3(0.0f);
        UInt64 mShadowFrameIndex = 0;

        // VAO/VBO debugowej geometrii fizyki (linie + punkty). Tworzone eager w Initialize na
        // wątku renderu; uploadowane per-klatkę z płaskich buforów snapshotu (pos(3)+color(3)).
        unsigned int mDebugVao = 0;
        unsigned int mDebugVbo = 0;

        // Meshe snapshotu rozwiązane RAZ na klatkę (ResolveSnapshotMeshes na początku
        // RenderSnapshot); indeks = indeks batcha w StaticMeshBatches / obiektu w
        // SkeletalMeshRenderObjects. GetAssetDataNoLoad bierze shared_lock na mutexie
        // AssetManagera — bez tego cache'u pass cieni brał go per batch PER KASKADA (x5),
        // kontendując z main threadem o ten sam mutex. Membery reużywane między klatkami
        // (Clear zachowuje capacity). Wpisy mogą być nullem (asset jeszcze nie w mapie).
        DynamicArray<TUsePointer<StaticMesh>> mResolvedBatchMeshes;
        DynamicArray<TUsePointer<SkeletalMesh>> mResolvedSkeletalMeshes;
        void ResolveSnapshotMeshes(RenderSnapshot* snapshot);

        // Pass 1: renderuje głębię casterów do map kaskad i zwraca macierze/splity kaskad.
        DynamicArray<ShadowCascadeData> RenderShadowPass(RenderSnapshot* snapshot, const Matrix4& cameraView);

        // Rysuje debugową geometrię fizyki ze snapshotu (linie + punkty) shaderem DebugLine.
        void RenderDebugGeometry(RenderSnapshot* snapshot, const Matrix4& viewProj);

        //Skeletal sTUFF
        ShaderStorageBuffer<Matrix4> mSkeletalMatricesBuffer;

        // Palety skinningu (skin = global * offset) WSZYSTKICH skeletal meshy klatki, liczone
        // raz w RenderSnapshot (BuildSkeletalPalettes) — dawniej pass cieni przeliczał je per
        // kaskada × obiekt (z alokacją), a główny pass jeszcze raz. Płaski scratch + zakresy
        // per obiekt (indeks = indeks w snapshot->SkeletalMeshRenderObjects); tablice są
        // memberami reużywanymi między klatkami (Clear zachowuje capacity — zero alokacji
        // w ustalonym stanie).
        struct SkeletalPaletteRange { UInt32 Offset = 0; UInt32 Count = 0; };
        DynamicArray<Matrix4> mSkeletalPaletteScratch;
        DynamicArray<SkeletalPaletteRange> mSkeletalPaletteRanges;

        // Liczy palety do scratcha (patrz wyżej). Wołane raz na klatkę, przed RenderShadowPass.
        void BuildSkeletalPalettes(RenderSnapshot* snapshot);

        // Uploaduje paletę obiektu do mSkeletalMatricesBuffer (binding 0). Rośnie z zapasem 2x
        // (jak mInstanceBuffer); BindBase PO ewentualnym Resize — Resize tworzy nowe ID bufora,
        // a indeksowany punkt bindowania trzymałby skasowany bufor.
        void UploadSkeletalPalette(UInt32 objectIndex);

        // Dane instancji static meshy (SSBO binding 1 — binding 0 to BoneMatrices skeletal, patrz
        // HELPERS.md). Bufor bindowany w CAŁOŚCI (BindBase) na całą klatkę w RenderSnapshot();
        // batche adresują swój zakres uniformem "instanceBaseIndex", nie BindRange per batch
        // (offsety BindRange muszą być wielokrotnością GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT,
        // które u części sterowników wynosi 256 — sizeof(InstanceGPUData)==128 by tego nie
        // gwarantowało). Skeletal używa osobnego indeksowanego punktu 0 (BoneMatrices), więc
        // bufor instancji przeżywa całą klatkę bez potrzeby rebindu.
        ShaderStorageBuffer<InstanceGPUData> mInstanceBuffer;

        // Programy shaderowe, dla których już ostrzegliśmy, że skeletal mesh jest rysowany bez
        // bloku SSBO "BoneMatrices" (mesh zamarza w bind pose) — warning raz, nie per klatkę.
        HashSet<UInt64> mWarnedNonSkeletalPrograms;

        // Programy shaderowe, dla których już ostrzegliśmy, że batch instancji (VisibleCount > 1)
        // renderuje się fallbackiem per-obiekt, bo shader nie ma bloku SSBO "InstanceMatrices" —
        // warning raz, nie per klatkę (analogicznie do mWarnedNonSkeletalPrograms).
        HashSet<UInt64> mWarnedNonInstancedPrograms;
    public:
        Renderer() = default;
        ~Renderer() = default;

        TUsePointer<FrameBuffer> GetMainFrameBuffer();

        void Initialize(ApplicationInfo* applicationInfo);
        void RenderSnapshot(RenderSnapshot* snapshot);
        void Shutdown();
    };
}

#endif //PLUENGINE_RENDERER_H
