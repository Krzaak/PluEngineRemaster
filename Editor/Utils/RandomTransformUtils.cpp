//
// Created by Plutex on 2026-07-19.
//

#include "RandomTransformUtils.h"

#include <algorithm>
#include <random>

#include "PluEngine/Timer.h"

namespace
{
    // Nazwy z prefiksem, bo edytor buduje się z UNITY_BUILD — anonimowy namespace
    // dzieli TU z innymi plikami.
    std::mt19937_64 RandomTransformMakeEngine(UInt64 seed)
    {
        if (seed == Plu::RandomTransformSeedAuto) {
            std::random_device device;
            return std::mt19937_64((static_cast<UInt64>(device()) << 32) ^ device());
        }
        return std::mt19937_64(seed);
    }

    Vec3 RandomTransformNextInRange(std::mt19937_64& engine, const Vec3& min, const Vec3& max)
    {
        // std::uniform_real_distribution wymaga a <= b, więc osie z odwróconymi
        // limitami normalizujemy zamiast zwracać UB.
        std::uniform_real_distribution<float> x(std::min(min.x, max.x), std::max(min.x, max.x));
        std::uniform_real_distribution<float> y(std::min(min.y, max.y), std::max(min.y, max.y));
        std::uniform_real_distribution<float> z(std::min(min.z, max.z), std::max(min.z, max.z));
        return Vec3(x(engine), y(engine), z(engine));
    }

    void RandomTransformFill(DynamicArray<Vec3>* outArray, UInt32 count,
                             const Vec3& min, const Vec3& max, UInt64 seed)
    {
        if (outArray == nullptr) {
            return;
        }

        outArray->Clear();
        outArray->Reserve(count);

        std::mt19937_64 engine = RandomTransformMakeEngine(seed);
        for (UInt32 i = 0; i < count; ++i) {
            outArray->PushBack(RandomTransformNextInRange(engine, min, max));
        }
    }
}

void Plu::GenerateRandomLocations(DynamicArray<Vec3>* outLocations, UInt32 count,
                                  const Vec3& min, const Vec3& max, UInt64 seed)
{
    PLU_PROFILE_SCOPE("GenerateRandomLocations");
    RandomTransformFill(outLocations, count, min, max, seed);
}

void Plu::GenerateRandomRotations(DynamicArray<Vec3>* outRotations, UInt32 count,
                                  const Vec3& min, const Vec3& max, UInt64 seed)
{
    PLU_PROFILE_SCOPE("GenerateRandomRotations");
    RandomTransformFill(outRotations, count, min, max, seed);
}

void Plu::GenerateRandomScales(DynamicArray<Vec3>* outScales, UInt32 count,
                               const Vec3& min, const Vec3& max, UInt64 seed)
{
    PLU_PROFILE_SCOPE("GenerateRandomScales");
    RandomTransformFill(outScales, count, min, max, seed);
}

void Plu::GenerateRandomUniformScales(DynamicArray<Vec3>* outScales, UInt32 count,
                                      float min, float max, UInt64 seed)
{
    PLU_PROFILE_SCOPE("GenerateRandomUniformScales");
    if (outScales == nullptr) {
        return;
    }

    outScales->Clear();
    outScales->Reserve(count);

    std::mt19937_64 engine = RandomTransformMakeEngine(seed);
    std::uniform_real_distribution<float> dist(std::min(min, max), std::max(min, max));
    for (UInt32 i = 0; i < count; ++i) {
        const float value = dist(engine);
        outScales->PushBack(Vec3(value));
    }
}
