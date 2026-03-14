//
// Created by Plutex on 2026-03-07.
//

#include "PluEngine/Physics/PhysicsBody.h"
#include "PluEngine/Physics/PhysicsLayers.h"

using namespace Plu;

PhysicsBody::PhysicsBody(
    JPH::BodyInterface& BodyInterface,
    JPH::ShapeRefC      Shape,
    const JPH::RVec3&   Position,
    BodyType           Type)
    : mBodyInterface(BodyInterface)
{
    JPH::BodyCreationSettings Settings(
        Shape,
        Position,
        JPH::Quat::sIdentity(),
        ToJoltMotionType(Type),
        ToJoltLayer(Type)
    );

    Settings.mAllowedDOFs = JPH::EAllowedDOFs::All;

    mBodyID = mBodyInterface.CreateAndAddBody(
        Settings,
        Type != BodyType::Static
            ? JPH::EActivation::Activate
            : JPH::EActivation::DontActivate
    );
}

PhysicsBody::~PhysicsBody() {
    if (IsValid()) {
        mBodyInterface.RemoveBody(mBodyID);
        mBodyInterface.DestroyBody(mBodyID);
    }
}

JPH::RVec3 PhysicsBody::GetPosition() const {
    return mBodyInterface.GetPosition(mBodyID);
}

JPH::Quat PhysicsBody::GetRotation() const {
    return mBodyInterface.GetRotation(mBodyID);
}

void PhysicsBody::SetPosition(const JPH::RVec3& Position) {
    mBodyInterface.SetPosition(mBodyID, Position, JPH::EActivation::Activate);
}

void PhysicsBody::SetRotation(const JPH::Quat& Rotation) {
    mBodyInterface.SetRotation(mBodyID, Rotation, JPH::EActivation::Activate);
}

void PhysicsBody::AddForce(const JPH::Vec3& Force) {
    mBodyInterface.AddForce(mBodyID, Force);
}

void PhysicsBody::AddImpulse(const JPH::Vec3& Impulse) {
    mBodyInterface.AddImpulse(mBodyID, Impulse);
}

void PhysicsBody::SetLinearVelocity(const JPH::Vec3& Velocity) {
    mBodyInterface.SetLinearVelocity(mBodyID, Velocity);
}

JPH::EMotionType PhysicsBody::ToJoltMotionType(BodyType Type) {
    switch (Type) {
        case BodyType::Dynamic:   return JPH::EMotionType::Dynamic;
        case BodyType::Kinematic: return JPH::EMotionType::Kinematic;
        default:                   return JPH::EMotionType::Static;
    }
}

JPH::ObjectLayer PhysicsBody::ToJoltLayer(BodyType Type) {
    return Type == BodyType::Static
        ? CollisionLayers::STATIC
        : CollisionLayers::DYNAMIC;
}