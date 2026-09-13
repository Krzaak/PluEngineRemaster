//
// Created by Plutex on 8/31/26.
//

#include "PluEngine/Gameplay/Components/ParticleSpawnerComponent.h"

#include "PluEngine/PluUtils.h"

void Plu::ParticleSpawnerComponent::SpawnParticles(int numParticles)
{
    if (numParticles <= 0) return;
    mRequestedParticles += static_cast<UInt64>(numParticles);
    mLastBurstSize = numParticles;
}

Vec3 Plu::ParticleSpawnerComponent::GetLaunchDirection()
{
    return GetForwardVector(GetWorldRotation());
}

void Plu::ParticleSpawnerComponent::OnBeginPlay()
{
    SpawnParticles(NumParticlesToSpawn);
}

void Plu::ParticleSpawnerComponent::OnUpdate(float deltaTime)
{
}
