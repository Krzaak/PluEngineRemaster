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

        // Seconds a particle lives.
        PLU_PROPERTY()
        float Lifetime = 10;

        // Spawn with a velocity of LaunchStrength (m/s) in a uniformly random direction inside the
        // launch cone.
        PLU_PROPERTY()
        bool LaunchOnSpawn = true;
        PLU_PROPERTY()
        float LaunchStrength = 1.0f;
        // Half-angle of the launch cone in degrees, around the spawner's forward vector.
        // 0 = straight forward, 90 = hemisphere, 180 = every direction.
        PLU_PROPERTY()
        float LaunchConeAngle = 180.0f;

        // Quadratic air drag: deceleration = Drag / Mass * speed^2 (1/m after dividing by mass).
        // Terminal speed under gravity g is sqrt(g * Mass / Drag).
        PLU_PROPERTY()
        float Drag = 0.2f;
        // Per-particle drag spread, 0..1: each particle gets Drag * (1 +- DragRandomness).
        PLU_PROPERTY()
        float DragRandomness = 0.4f;

        // Re-spawn the last burst every LoopLength seconds.
        PLU_PROPERTY()
        bool Loop = true;
        PLU_PROPERTY()
        float LoopLength = 1.0f;

        // Kill a particle once its speed (m/s) drops to KillWhenSlowSpeed. Only armed after the
        // particle has been faster than that, so particles spawned at rest are not killed at once.
        PLU_PROPERTY()
        bool KillWhenSlow = true;
        PLU_PROPERTY()
        float KillWhenSlowSpeed = 0.1f;

        // Acceleration in m/s^2, independent of mass.
        PLU_PROPERTY()
        Vec3 Gravity = {0.0f,-9.8f,0.0f};
        // Kilograms. Only scales drag: a heavier particle keeps its speed longer.
        PLU_PROPERTY()
        float Mass = 10.0f;
    };

    PLU_STRUCT(NoVirtualClass)
    struct PLUEFFECTS_API Particle
    {
        REFLECTION_BODY_PARTICLE()

        Vec3 Location = Vec3(0.0f);
        Vec3 Velocity = Vec3(0.0f);

        // This particle's drag coefficient (class Drag with DragRandomness applied).
        float Drag = 0.0f;
        // Seconds left to live.
        float Lifetime = 0.0f;

        bool Alive = false;
        // Set once the particle is faster than KillWhenSlowSpeed; KillWhenSlow waits for it.
        bool SlowKillArmed = false;
    };
}

#endif //PLUENGINE_PARTICLE_H
