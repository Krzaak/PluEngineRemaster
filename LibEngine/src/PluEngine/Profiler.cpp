//
// Created by Plutex on 2026-06-20.
//

#include "PluEngine/Profiler.h"

#include <algorithm>

#include "PluEngine/Threading/ThreadAffinity.h"

namespace Plu {

    Profiler* Profiler::GetInstance()
    {
        // Function-local static: gwarantowana pojedyncza, thread-safe inicjalizacja.
        static Profiler instance;
        return &instance;
    }

    String Profiler::MakeKey(const String& name, const String& threadName)
    {
        return threadName + "|" + name;
    }

    void Profiler::Record(const String& name, float durationMs)
    {
        RecordForThread(name, GetCurrentThreadName(), durationMs);
    }

    void Profiler::RecordForThread(const String& name, const String& threadName, float durationMs)
    {
        std::lock_guard lock(mMutex);

        ProfilerEntry& entry = mEntries[MakeKey(name, threadName)]; // tworzy domyślny wpis, jeśli brak
        entry.Name = name;
        entry.ThreadName = threadName;

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

    DynamicArray<String> Profiler::SnapshotThreadNames()
    {
        std::lock_guard lock(mMutex);

        DynamicArray<String> names;
        for (const auto& pair : mEntries) {
            const String& threadName = pair.second.ThreadName;
            bool alreadyListed = false;
            for (const String& existing : names) {
                if (existing == threadName) {
                    alreadyListed = true;
                    break;
                }
            }
            if (!alreadyListed) {
                names.PushBack(threadName);
            }
        }

        // Stabilna kolejność w combo — inaczej pozycje skakałyby przy rehashu mapy.
        std::sort(names.begin(), names.end(), [](const String& a, const String& b) { return a.Compare(b) < 0; });
        return names;
    }

    void Profiler::Clear()
    {
        std::lock_guard lock(mMutex);
        mEntries.Clear();
    }

}
