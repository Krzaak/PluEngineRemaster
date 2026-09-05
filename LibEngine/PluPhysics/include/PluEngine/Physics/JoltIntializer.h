//
// Created by Plutex on 2026-03-07.
//

#ifndef PLUENGINE_JOLTINTIALIZER_H
#define PLUENGINE_JOLTINTIALIZER_H

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>

namespace JPH
{
	class JobSystem;
	class JobSystemThreadPool;
}

namespace Plu
{
	class PhysicsWorld;
	struct ApplicationInfo;
	struct EngineObjectHandle;
}

namespace Plu::JoltPhysics {
	void Init(ApplicationInfo* applicationInfo);
	void Shutdown();

	TUsePointer<PhysicsWorld> GetPhysicsWorldBySceneHandle(EngineObjectHandle sceneHandle);
	TOwningPointer<JPH::JobSystem> GetJoltThreadPool();
}

#endif //PLUENGINE_JOLTINTIALIZER_H