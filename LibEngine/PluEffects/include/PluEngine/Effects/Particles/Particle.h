//
// Created by Plutex on 8/31/26.
//

#ifndef PLUENGINE_PARTICLE_H
#define PLUENGINE_PARTICLE_H
#include "PluEngine/Core.h"
#include "PluEngine/PluTypes.h"
#include "Particle.generated.h"

namespace Plu
{
    struct StaticMesh;


    PLU_STRUCT()
    struct PLUEFFECTS_API ParticleClass
    {
        REFLECTION_BODY_PARTICLECLASS()

        PLU_PROPERTY()
        float Lifetime = 10;

        PLU_PROPERTY()
        bool LaunchOnSpawn = true;
        PLU_PROPERTY()
        float LaunchStrength = 1.0f;

        PLU_PROPERTY()
        float Drag = 0.2f;
        PLU_PROPERTY()
        float DragRandomness = 0.4f;

        PLU_PROPERTY()
        bool Loop = true;
        PLU_PROPERTY()
        float LoopLength = 1.0f;

        PLU_PROPERTY()
        bool KillWhenSlow = true;
        PLU_PROPERTY()
        float KillWhenSlowSpeed = 0.1f;

        PLU_PROPERTY()
        Vec3 Gravity = {0.0f,-9.8f,0.0f};
        PLU_PROPERTY()
        float Mass = 10.0f;
    };

    PLU_STRUCT(NoVirtualClass)
    struct PLUEFFECTS_API Particle
    {
        REFLECTION_BODY_PARTICLE()

        //Current Stats
        Vec3 Location;
        Vec3 Velocity;
        Vec3 LaunchDirection;

        float DragRandomness;
        float Lifetime;

        bool FirstFrame = true;

        //Class
        ParticleClass* Class;
    };
}

#endif //PLUENGINE_PARTICLE_H
