//
// Created by Plutex on 8/31/26.
//

#include "PluEngine/Effects/Particles/ParticleSpawner.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "PluEngine/PluUtils.h"
#include "PluEngine/Timer.h"

namespace
{
    // Longest step a single tick may integrate. A render hitch (window drag, breakpoint, shader
    // compile) would otherwise launch every particle across the scene in one step.
    constexpr float kMaxParticleDeltaTime = 0.1f;
    // Shortest allowed loop period, so a LoopLength of ~0 cannot spawn a burst per loop iteration forever.
    constexpr float kMinLoopLength = 0.01f;
    // Mass floor; drag divides by it.
    constexpr float kMinParticleMass = 0.001f;

    // Uniformly distributed direction inside a cone of the given half-angle around axis (Archimedes:
    // on a sphere, z uniform in [cos(halfAngle), 1] with a uniform azimuth covers the cap evenly).
    // Normalizing a random point of a cube instead biases toward its corners.
    Vec3 RandomDirectionInCone(const Vec3& axis, float halfAngleRadians)
    {
        const float z = PluRandom::NextFloat(std::cos(halfAngleRadians), 1.0f);
        const float azimuth = PluRandom::NextFloat(0.0f, 6.2831853f);
        const float radius = std::sqrt(std::max(0.0f, 1.0f - z * z));

        const Vec3 reference = std::abs(axis.y) > 0.99f ? Vec3(0.0f, 0.0f, 1.0f) : Vec3(0.0f, 1.0f, 0.0f);
        const Vec3 right = glm::normalize(glm::cross(axis, reference));
        const Vec3 up = glm::cross(right, axis);
        return right * (radius * std::cos(azimuth)) + up * (radius * std::sin(azimuth)) + axis * z;
    }
}

void Plu::ParticleSpawner::SetParticleClass(const ParticleClass& particleClass)
{
    mParticleClass = particleClass;
}

void Plu::ParticleSpawner::SetTransform(const Vec3& location, const Vec3& launchDirection)
{
    mLocation = location;
    const float length = glm::length(launchDirection);
    mLaunchDirection = length > 0.0001f ? launchDirection / length : Vec3(0.0f, 0.0f, -1.0f);
}

void Plu::ParticleSpawner::SyncSpawnRequests(UInt64 requestedParticles, int lastBurstSize)
{
    mLastParticleSpawned = std::max(lastBurstSize, 0);
    // The counter never decreases for one component, so going backwards means it was recreated
    // under the same UUID (scene reload) — start counting from zero again.
    if (requestedParticles < mSyncedRequestedParticles) {
        mSyncedRequestedParticles = 0;
    }
    if (requestedParticles == mSyncedRequestedParticles) return;
    const UInt64 missing = requestedParticles - mSyncedRequestedParticles;
    mSyncedRequestedParticles = requestedParticles;
    SpawnParticles(static_cast<int>(std::min<UInt64>(missing, static_cast<UInt64>(std::numeric_limits<int>::max()))));
}

void Plu::ParticleSpawner::SpawnParticles(int numParticles)
{
    if (numParticles <= 0) return;
    PLU_PROFILE_SCOPE("ParticleSpawner::SpawnParticles");

    const float dragRandomness = std::clamp(mParticleClass.DragRandomness, 0.0f, 1.0f);
    const float coneHalfAngle = glm::radians(std::clamp(mParticleClass.LaunchConeAngle, 0.0f, 180.0f));
    for (int i = 0; i < numParticles; i++) {
        Particle particle;
        particle.Alive = true;
        particle.Location = mLocation;
        particle.Lifetime = mParticleClass.Lifetime;
        particle.Drag = std::max(0.0f, mParticleClass.Drag * (1.0f + PluRandom::NextFloat(-1.0f, 1.0f) * dragRandomness));
        if (mParticleClass.LaunchOnSpawn) {
            particle.Velocity = RandomDirectionInCone(mLaunchDirection, coneHalfAngle) * mParticleClass.LaunchStrength;
        }

        int index;
        if (mFreeParticles.TryPopFront(index)) {
            mParticles[index] = particle;
        } else {
            mParticles.PushBack(particle);
        }
    }
}

void Plu::ParticleSpawner::KillParticle(int index)
{
    mParticles[index].Alive = false;
    mParticles[index].Lifetime = 0.0f;
    mFreeParticles.PushBack(index);
}

void Plu::ParticleSpawner::TickParticles(float deltaTime, bool debug, DynamicArray<float>* debugPoints)
{
    PLU_PROFILE_SCOPE("ParticleSpawner::TickParticles");
    deltaTime = std::clamp(deltaTime, 0.0f, kMaxParticleDeltaTime);

    // Loop: repeat the last burst every LoopLength seconds. The remainder carries over, so the
    // spawn rate does not drift with the frame rate.
    if (mParticleClass.Loop && mLastParticleSpawned > 0) {
        const float loopLength = std::max(mParticleClass.LoopLength, kMinLoopLength);
        mLoopTime += deltaTime;
        while (mLoopTime >= loopLength) {
            mLoopTime -= loopLength;
            SpawnParticles(mLastParticleSpawned);
        }
    }

    const Vec3 gravity = mParticleClass.Gravity;
    const float invMass = 1.0f / std::max(mParticleClass.Mass, kMinParticleMass);
    const float killSpeed = mParticleClass.KillWhenSlowSpeed;

    for (int i = 0; i < static_cast<int>(mParticles.Size()); i++) {
        Particle& particle = mParticles[i];
        if (!particle.Alive) continue;

        particle.Lifetime -= deltaTime;
        if (particle.Lifetime <= 0.0f) {
            KillParticle(i);
            continue;
        }

        // Semi-implicit Euler: velocity first, then position with the new velocity.
        // Gravity is a constant acceleration, so the explicit step is fine.
        particle.Velocity += gravity * deltaTime;

        // Quadratic drag, dv/dt = -k|v|v with k = Drag / Mass. Solved implicitly:
        // v' = v / (1 + k|v|dt) — exact for drag alone, never overshoots or reverses the
        // velocity, and stable for any step (explicit Euler blows up once k|v|dt > 2).
        const float speed = glm::length(particle.Velocity);
        particle.Velocity /= 1.0f + particle.Drag * invMass * speed * deltaTime;

        particle.Location += particle.Velocity * deltaTime;

        if (mParticleClass.KillWhenSlow) {
            const float newSpeed = glm::length(particle.Velocity);
            if (newSpeed > killSpeed) {
                particle.SlowKillArmed = true;
            } else if (particle.SlowKillArmed) {
                KillParticle(i);
                continue;
            }
        }

        if (debug) {
            debugPoints->PushBack(particle.Location.x);
            debugPoints->PushBack(particle.Location.y);
            debugPoints->PushBack(particle.Location.z);
            debugPoints->PushBack(1.0f);
            debugPoints->PushBack(1.0f);
            debugPoints->PushBack(0.0f);
        }
    }
}
