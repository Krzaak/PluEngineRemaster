#pragma once
#include <cstdint>
#include <random>

// Lekki, header-only generator liczb losowych dla PluSTL.
// Silnik jest thread_local — nie ma synchronizacji, więc losowanie z dowolnego
// wątku jest bezpieczne, ale sekwencje nie są współdzielone między wątkami.
namespace PluRandom
{
    inline std::mt19937_64& Engine()
    {
        thread_local std::mt19937_64 engine = [] {
            std::random_device device;
            return std::mt19937_64((static_cast<uint64_t>(device()) << 32) ^ device());
        }();
        return engine;
    }

    // Ustawia ziarno lokalnego silnika — deterministyczne losowanie (testy, replay).
    inline void Seed(uint64_t seed) { Engine().seed(seed); }

    inline uint64_t NextUInt64() { return Engine()(); }

    // Losowy indeks z [0, size). Dla size == 0 zwraca 0 — sprawdzaj pustość u siebie.
    inline size_t NextIndex(size_t size)
    {
        if (size == 0) return 0;
        return static_cast<size_t>(Engine()() % size);
    }

    // Losowy int z [min, max] (obustronnie domknięty). Odwrócone limity są normalizowane.
    inline int64_t NextInt(int64_t min, int64_t max)
    {
        if (min > max) { const int64_t t = min; min = max; max = t; }
        return std::uniform_int_distribution<int64_t>(min, max)(Engine());
    }

    // Losowy float z [min, max). Odwrócone limity są normalizowane.
    inline float NextFloat(float min = 0.0f, float max = 1.0f)
    {
        if (min > max) { const float t = min; min = max; max = t; }
        return std::uniform_real_distribution<float>(min, max)(Engine());
    }

    // Rzut monetą z zadanym prawdopodobieństwem sukcesu.
    inline bool NextBool(float probability = 0.5f)
    {
        return NextFloat(0.0f, 1.0f) < probability;
    }
}
