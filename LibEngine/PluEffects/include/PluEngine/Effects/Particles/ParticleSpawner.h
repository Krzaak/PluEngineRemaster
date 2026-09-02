//
// Created by Plutex on 8/31/26.
//

#ifndef PLUENGINE_PARTICLESPAWNER_H
#define PLUENGINE_PARTICLESPAWNER_H

#include "PluEngine/Core.h"
#include "PluEngine/Core/Objects/EngineObject.h"
#include "PluEngine/Effects/Particles/Particle.h"
#include "ParticleSpawner.generated.h"
#include "PluEngine/PluUUID.h"

namespace Plu
{
    struct RenderSnapshot;
    PLU_CLASS()
    class PLUEFFECTS_API ParticleSpawner : public EngineObject
    {
        REFLECTION_BODY_PARTICLESPAWNER()
    private:
        //RenderPArticle, Particle Ptr
        DynamicArray<std::pair<bool, Particle*>> mParticles;
        Queue<int> mFreeParticles;
        ParticleClass* mParticleClass;
        float mLoopTime = 0.0f;
        int mLastParticleSpawned = 1;
    public:
        ParticleSpawner() = default;
        virtual ~ParticleSpawner() override;

        Vec3 Location;
        Vec3 Rotation;
        PluUUID UUID;

        void InitializeSpawner(ParticleClass particleClass);
        void SpawnParticles(int numParticles);

        void TickParticles(float deltaTime, bool debug, DynamicArray<float>* debugPoints);
    };
}

#endif //PLUENGINE_PARTICLESPAWNER_H
