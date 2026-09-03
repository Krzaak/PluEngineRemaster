#pragma once
#include <cstdint>
#include <random>

// Lightweight, header-only random number generation for PluSTL.
// The engine is thread_local — there is no synchronization, so drawing from any
// thread is safe, but sequences are not shared between threads.
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

    // Seeds this thread's engine — deterministic draws (tests, replay).
    inline void Seed(uint64_t seed) { Engine().seed(seed); }

    inline uint64_t NextUInt64() { return Engine()(); }

    // Random index in [0, size). Returns 0 for size == 0 — check emptiness yourself.
    inline size_t NextIndex(size_t size)
    {
        if (size == 0) return 0;
        return static_cast<size_t>(Engine()() % size);
    }

    // Random int in [min, max] (both ends inclusive). Reversed bounds are normalized.
    inline int64_t NextInt(int64_t min, int64_t max)
    {
        if (min > max) { const int64_t t = min; min = max; max = t; }
        return std::uniform_int_distribution<int64_t>(min, max)(Engine());
    }

    // Random float in [min, max). Reversed bounds are normalized.
    inline float NextFloat(float min = 0.0f, float max = 1.0f)
    {
        if (min > max) { const float t = min; min = max; max = t; }
        return std::uniform_real_distribution<float>(min, max)(Engine());
    }

    // Coin flip with the given success probability.
    inline bool NextBool(float probability = 0.5f)
    {
        return NextFloat(0.0f, 1.0f) < probability;
    }
}
