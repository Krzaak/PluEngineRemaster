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
    const JPH::Quat&    Rotation,
    BodyType            Type,
    PhysicsBodyMode     Mode)
    : mBodyInterface(BodyInterface)
{
    JPH::BodyCreationSettings Settings(
        Shape,
        Position,
        Rotation,
        ToJoltMotionType(Type),
        ToJoltLayer(Type)
    );

    Settings.mAllowedDOFs = JPH::EAllowedDOFs::All;
    Settings.mIsSensor = (Mode == PhysicsBodyMode::Trigger);

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

JPH::Vec3 PhysicsBody::GetLinearVelocity() const {
    return mBodyInterface.GetLinearVelocity(mBodyID);
}

void PhysicsBody::SetLinearVelocity(const JPH::Vec3& Velocity) {
    mBodyInterface.SetLinearVelocity(mBodyID, Velocity);
}

void PhysicsBody::AddLinearVelocity(const JPH::Vec3& Velocity) {
    mBodyInterface.AddLinearVelocity(mBodyID, Velocity);
}

JPH::Vec3 PhysicsBody::GetAngularVelocity() const {
    return mBodyInterface.GetAngularVelocity(mBodyID);
}

void PhysicsBody::SetAngularVelocity(const JPH::Vec3& AngularVelocity) {
    mBodyInterface.SetAngularVelocity(mBodyID, AngularVelocity);
}

void PhysicsBody::AddForce(const JPH::Vec3& Force) {
    mBodyInterface.AddForce(mBodyID, Force);
}

void PhysicsBody::AddTorque(const JPH::Vec3& Torque) {
    mBodyInterface.AddTorque(mBodyID, Torque);
}

void PhysicsBody::AddImpulse(const JPH::Vec3& Impulse) {
    mBodyInterface.AddImpulse(mBodyID, Impulse);
}

void PhysicsBody::AddAngularImpulse(const JPH::Vec3& Impulse) {
    mBodyInterface.AddAngularImpulse(mBodyID, Impulse);
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