//
// Created by Plutex on 2026-06-20.
//

#ifndef PLUENGINE_PROFILER_H
#define PLUENGINE_PROFILER_H

#include <mutex>
#include "Core.h"
#include "PluSTL_FWD.h"

namespace Plu {

    // Pojedynczy wpis profilera: nazwa -> historia ostatnich pomiarów + statystyki.
    // History to ring buffer; WriteIndex wskazuje następne miejsce do zapisu.
    struct ProfilerEntry {
        static constexpr Int4 kHistorySize = 120;
        float History[kHistorySize] = {}; // ostatnie próbki w ms
        Int4 WriteIndex = 0;              // następna pozycja zapisu (offset dla PlotLines)
        Int4 SampleCount = 0;             // ile realnych próbek (<= kHistorySize)
        float LastMs = 0.0f;
        float MinMs = 0.0f;
        float MaxMs = 0.0f;
        float AvgMs = 0.0f;               // średnia z aktualnego okna historii
        UInt64 TotalCalls = 0;            // całkowita liczba zarejestrowanych pomiarów
    };

    // Globalny rejestr timingów. Plain singleton (nie EngineObject).
    // Thread-safe: pomiary trafiają z wątku gry / render-threada, panel czyta z wątku UI.
    class PLU_API Profiler {
    public:
        static Profiler* GetInstance();

        // Dopisuje pomiar do historii danego timera (tworzy wpis, jeśli nie istnieje).
        void Record(const String& name, float durationMs);

        // Kopia rejestru do bezpiecznego odczytu przez panel.
        GameHashMap<String, ProfilerEntry> Snapshot();

        // Czyści wszystkie zebrane timingi.
        void Clear();

    private:
        Profiler() = default;

        GameHashMap<String, ProfilerEntry> mEntries;
        std::mutex mMutex;
    };

}

#endif //PLUENGINE_PROFILER_H
