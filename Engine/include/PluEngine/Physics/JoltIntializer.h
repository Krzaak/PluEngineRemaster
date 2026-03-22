//
// Created by Plutex on 2026-03-07.
//

#ifndef PLUENGINE_JOLTINTIALIZER_H
#define PLUENGINE_JOLTINTIALIZER_H

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>

namespace Plu::JoltPhysics {
	void Init();
	void Shutdown();
}

#endif //PLUENGINE_JOLTINTIALIZER_H