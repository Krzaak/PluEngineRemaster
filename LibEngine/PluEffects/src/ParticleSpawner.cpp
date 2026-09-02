//
// Created by Plutex on 8/31/26.
//

#include "PluEngine/Effects/Particles/ParticleSpawner.h"

#include "PluEngine/PluUtils.h"


Plu::ParticleSpawner::~ParticleSpawner()
{
    for (auto particle : mParticles) {
        delete particle.second;
    }
    mParticles.Clear();
}

void Plu::ParticleSpawner::InitializeSpawner(ParticleClass particleClass)
{
    ParticleClass* newClass = new ParticleClass();
    *newClass = particleClass;
    mParticleClass = newClass;
}

void Plu::ParticleSpawner::SpawnParticles(int numParticles)
{
    PLU_CORE_ASSERT(numParticles != 0, "Need more than one particle spawned!");
    mLastParticleSpawned = numParticles;
    for (int i = 0; i < numParticles; i++) {
        Particle* particle = nullptr;
        int idx;
        bool found = mFreeParticles.TryPopFront(idx);
        if (found) {
            particle = mParticles[idx].second;
            mParticles[idx].first = true;
            *particle = Particle();
        } else {
            particle = new Particle();
        }
        particle->Lifetime = mParticleClass->Lifetime;
        particle->Location = Location;
        particle->DragRandomness = (PluRandom::NextFloat(-1,1) * mParticleClass->DragRandomness) * mParticleClass->Drag;
        if (!found) {
            mParticles.EmplaceBack(true, particle);
        }
    }
}

void Plu::ParticleSpawner::TickParticles(float deltaTime, bool debug, DynamicArray<float>* debugPoints)
{
    if (mParticleClass->Loop) {
        mLoopTime += deltaTime;
    }
    if (mLoopTime >= mParticleClass->LoopLength && mParticles.Size() > 0) {
        SpawnParticles(mLastParticleSpawned);
        mLoopTime = 0.0f;
    }
    for (int i = 0; i < mParticles.Size(); i++) {
        std::pair<bool, Particle*> particle = mParticles[i];
        if (!particle.first) continue;
        particle.second->Lifetime -= deltaTime;
        if (particle.second->Lifetime < 0.0f) {
            particle.second->Lifetime = 0.0f;
            mParticles[i].first = false;
            mFreeParticles.PushBack(i);
            //Kill Particle TODO
        }
        if (mParticles[i].first) {
            //Here calculations
            Particle* particleRaw = mParticles[i].second;
            if (particleRaw->FirstFrame && mParticleClass->LaunchOnSpawn) {
                Vec3 randomDir;
                randomDir.x = PluRandom::NextFloat(-180, 180);
                randomDir.y = PluRandom::NextFloat(-180, 180);
                randomDir.z = PluRandom::NextFloat(-180, 180);
                randomDir = normalize(randomDir);
                particleRaw->Velocity += randomDir * mParticleClass->LaunchStrength;
            }
            particleRaw->FirstFrame = false;
            //Drag
            Vec3 dir = normalize(particleRaw->Velocity);
            particleRaw->Velocity -= dir * (particleRaw->DragRandomness + mParticleClass->Drag) * deltaTime;
            particleRaw->Location += particleRaw->Velocity * deltaTime;
            if (glm::length(particleRaw->Velocity) <= mParticleClass->KillWhenSlowSpeed && mParticleClass->KillWhenSlow) {
                particle.second->Lifetime = 0.0f;
                mParticles[i].first = false;
                mFreeParticles.PushBack(i);
                //Kill Particle TODO
            }
        }
        if (debug && mParticles[i].first) {
            debugPoints->PushBack(particle.second->Location.x);
            debugPoints->PushBack(particle.second->Location.y);
            debugPoints->PushBack(particle.second->Location.z);
            debugPoints->PushBack(1.0);
            debugPoints->PushBack(1.0);
            debugPoints->PushBack(0.0);
        }
    }
}
