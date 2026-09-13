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
        // Particle pool; dead slots (Alive == false) are listed in mFreeParticles and reused.
        DynamicArray<Particle> mParticles;
        Queue<int> mFreeParticles;
        ParticleClass mParticleClass;
        float mLoopTime = 0.0f;
        // Size of the last burst, repeated by Loop. 0 = nothing spawned yet, so no looping.
        int mLastParticleSpawned = 0;
        // Component's cumulative requested particle count already turned into particles.
        UInt64 mSyncedRequestedParticles = 0;

        Vec3 mLocation = Vec3(0.0f);
        Vec3 mLaunchDirection = Vec3(0.0f, 0.0f, -1.0f);

        void KillParticle(int index);
        void SpawnParticles(int numParticles);
    public:
        ParticleSpawner() = default;
        virtual ~ParticleSpawner() override = default;

        PluUUID UUID;

        // Particles are simulated in world space: moving the spawner only moves where new particles
        // start and which way they launch, particles already in flight keep their trajectory.
        void SetParticleClass(const ParticleClass& particleClass);
        void SetTransform(const Vec3& location, const Vec3& launchDirection);

        // Catches up with the owning component's cumulative request counter: spawns
        // (requestedParticles - already synced) particles. Idempotent — syncing the same value again
        // spawns nothing, so a snapshot that is rendered twice or skipped cannot double or lose bursts.
        // lastBurstSize is the size Loop repeats.
        void SyncSpawnRequests(UInt64 requestedParticles, int lastBurstSize);

        void TickParticles(float deltaTime, bool debug, DynamicArray<float>* debugPoints);
    };
}

#endif //PLUENGINE_PARTICLESPAWNER_H
