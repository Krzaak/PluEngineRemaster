//
// Created by Plutex on 8/31/26.
//

#ifndef PLUENGINE_PARTICLESPAWNERCOMPONENT_H
#define PLUENGINE_PARTICLESPAWNERCOMPONENT_H
#include "PluEngine/Effects/Particles/Particle.h"
#include "PluEngine/Core.h"
#include "PluEngine/Gameplay/WorldComponent.h"
#include "ParticleSpawnerComponent.generated.h"

namespace Plu
{
    // Gameplay-side handle of a particle spawner. The simulation runs on the render thread
    // (MULTITHREADING.md); this component only holds the settings and the request counter that
    // RenderSnapshotBuilder packs into every snapshot.
    PLU_CLASS(PyExport)
    class PLUGAMEPLAY_API ParticleSpawnerComponent : public WorldComponent
    {
        REFLECTION_BODY_PARTICLESPAWNERCOMPONENT()
    private:
        // Cumulative, never reset: the render thread spawns the difference to the value it saw last.
        UInt64 mRequestedParticles = 0;
        int mLastBurstSize = 0;
    public:
        ParticleSpawnerComponent() = default;
        virtual ~ParticleSpawnerComponent() override = default;

        PLU_PROPERTY(PyExport)
        int NumParticlesToSpawn = 10;

        PLU_PROPERTY(PyExport)
        ParticleClass SpawnerParticleClass;

        // Requests a burst. Several calls in one frame add up; Loop repeats the latest burst size.
        PLU_FUNCTION(PyExport)
        void SpawnParticles(int numParticles);

        // Axis of the launch cone in world space — the component's forward vector (-Z at zero rotation).
        Vec3 GetLaunchDirection();

        UInt64 GetRequestedParticles() const { return mRequestedParticles; }
        int GetLastBurstSize() const { return mLastBurstSize; }

        void OnBeginPlay() override;
        void OnUpdate(float deltaTime) override;
    };
}

#endif //PLUENGINE_PARTICLESPAWNERCOMPONENT_H
