//
// Created by Plutex on 8/31/26.
//

#ifndef PLUENGINE_PARTICLESPAWNERCOMPONENT_H
#define PLUENGINE_PARTICLESPAWNERCOMPONENT_H
#include "PluEngine/Effects/Particles/ParticleSpawner.h"
#include "PluEngine/Core.h"
#include "PluEngine/Gameplay/WorldComponent.h"
#include "ParticleSpawnerComponent.generated.h"

namespace Plu
{
    PLU_CLASS()
    class PLUGAMEPLAY_API ParticleSpawnerComponent : public WorldComponent
    {
        REFLECTION_BODY_PARTICLESPAWNERCOMPONENT()
    private:
        TOwningPointer<ParticleSpawner> mParticleSpawner;
    public:
        ParticleSpawnerComponent() = default;
        virtual ~ParticleSpawnerComponent() override = default;

        PLU_PROPERTY()
        int NumParticlesToSpawn = 10;

        PLU_PROPERTY()
        ParticleClass SpawnerParticleClass;

        void OnBeginPlay() override;
        void OnUpdate(float deltaTime) override;
    };
}

#endif //PLUENGINE_PARTICLESPAWNERCOMPONENT_H
