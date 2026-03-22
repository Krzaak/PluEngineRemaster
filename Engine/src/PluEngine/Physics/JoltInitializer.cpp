//
// Created by Plutex on 2026-03-07.
//

#include "PluEngine/Physics/JoltIntializer.h"

void Plu::JoltPhysics::Init() {
	JPH::RegisterDefaultAllocator();
	JPH::Factory::sInstance = new JPH::Factory();
	JPH::RegisterTypes();
}

void Plu::JoltPhysics::Shutdown() {
	JPH::UnregisterTypes();
	delete JPH::Factory::sInstance;
	JPH::Factory::sInstance = nullptr;
}