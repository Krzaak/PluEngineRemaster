//
// Created by Plutex on 2026-09-15.
//

#ifndef PLUENGINE_RENDERPARTICLESTATS_H
#define PLUENGINE_RENDERPARTICLESTATS_H

#include "PluEngine/Core.h"
#include "PluEngine/PluTypes.h"
#include "PluEngine/Core/Objects/EngineObjectHandle.h"
#include "PluEngine/Effects/Particles/ParticleSpawner.h"

namespace Plu
{
    // Particle state as the render thread saw it at the end of one particle tick (Debug Particles
    // panel). Particles live on the render thread and main never touches a ParticleSpawner
    // (MULTITHREADING.md), so this is a copy published under a mutex rather than a live view.
    struct ParticleDebugStats
    {
        // Bumped on every publish; 0 = nothing published yet. Lets the reader tell stale data apart.
        UInt64 PublishCount = 0;

        // World whose snapshot was rendered, and the particle delta time used for it.
        EngineObjectHandle SceneHandle;
        float DeltaTime = 0.0f;

        // Every spawner of SceneHandle's world.
        DynamicArray<ParticleSpawnerDebugStats> Spawners;

        // Spawners the renderer still holds for other worlds (e.g. the PIE world after PIE ends,
        // which is only reconciled when it publishes again).
        UInt32 OtherWorldSpawners = 0;
        UInt64 OtherWorldAliveParticles = 0;
    };

    // Gathering walks every particle, so it only happens on request: the reader calls
    // RequestParticleDebugStats every frame it wants data, the render thread consumes the request
    // once per rendered snapshot and publishes. Same renew-every-frame contract as
    // RenderingManager::RequestShadowCascadeView. All four are thread-safe.
    PLURENDER_API void RequestParticleDebugStats();
    // Render thread. Returns true (and clears the request) if somebody asked since the last call.
    PLURENDER_API bool ConsumeParticleDebugStatsRequest();
    // Render thread. Stamps PublishCount.
    PLURENDER_API void PublishParticleDebugStats(ParticleDebugStats&& stats);
    // Copy of the last published stats.
    PLURENDER_API ParticleDebugStats GetParticleDebugStats();
}

#endif //PLUENGINE_RENDERPARTICLESTATS_H
