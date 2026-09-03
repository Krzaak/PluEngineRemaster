//
// Created by Plutex on 2026-03-07.
//

#ifndef PLUENGINE_PHYSICSLAYERS_H
#define PLUENGINE_PHYSICSLAYERS_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>


namespace Plu::CollisionLayers {
	static constexpr JPH::ObjectLayer STATIC = 0;
	static constexpr JPH::ObjectLayer DYNAMIC = 1;
	static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}


#endif //PLUENGINE_PHYSICSLAYERS_H