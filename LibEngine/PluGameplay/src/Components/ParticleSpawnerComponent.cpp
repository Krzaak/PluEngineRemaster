//
// Created by Plutex on 8/31/26.
//

#include "PluEngine/Gameplay/Components/ParticleSpawnerComponent.h"

#include "PluEngine/Core/Objects/EngineObjectManager.h"
#include "PluEngine/Gameplay/Scenes/SceneWorld.h"

void Plu::ParticleSpawnerComponent::OnBeginPlay()
{
    GetWorld()->SpawnParticlesForComponent(This(), NumParticlesToSpawn);
}

void Plu::ParticleSpawnerComponent::OnUpdate(float deltaTime)
{
}
