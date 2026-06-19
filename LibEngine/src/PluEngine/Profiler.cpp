//
// Created by Plutex on 2026-06-20.
//

#include "PluEngine/Profiler.h"

namespace Plu {

    Profiler* Profiler::GetInstance()
    {
        // Function-local static: gwarantowana pojedyncza, thread-safe inicjalizacja.
        static Profiler instance;
        return &instance;
    }

    void Profiler::Record(const String& name, float durationMs)
    {
        std::lock_guard lock(mMutex);

        ProfilerEntry& entry = mEntries[name]; // tworzy domyślny wpis, jeśli brak

        entry.History[entry.WriteIndex] = durationMs;
        entry.WriteIndex = (entry.WriteIndex + 1) % ProfilerEntry::kHistorySize;
        if (entry.SampleCount < ProfilerEntry::kHistorySize) {
            entry.SampleCount++;
        }
        entry.LastMs = durationMs;
        entry.TotalCalls++;

        // Statystyki z aktualnego okna historii.
        float sum = 0.0f;
        float minMs = entry.History[0];
        float maxMs = entry.History[0];
        for (Int4 i = 0; i < entry.SampleCount; i++) {
            const float v = entry.History[i];
            sum += v;
            if (v < minMs) minMs = v;
            if (v > maxMs) maxMs = v;
        }
        entry.MinMs = minMs;
        entry.MaxMs = maxMs;
        entry.AvgMs = entry.SampleCount > 0 ? sum / static_cast<float>(entry.SampleCount) : 0.0f;
    }

    GameHashMap<String, ProfilerEntry> Profiler::Snapshot()
    {
        std::lock_guard lock(mMutex);
        return mEntries; // głęboka kopia
    }

    void Profiler::Clear()
    {
        std::lock_guard lock(mMutex);
        mEntries.Clear();
    }

}
