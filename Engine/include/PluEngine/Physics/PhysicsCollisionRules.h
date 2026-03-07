//
// Created by Plutex on 2026-03-07.
//

#ifndef PLUENGINE_PHYSICSCOLLISIONRULES_H
#define PLUENGINE_PHYSICSCOLLISIONRULES_H

#include "Jolt/Jolt.h"
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include "PhysicsLayers.h"

namespace Plu
{
	class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
	public:
		bool ShouldCollide(JPH::ObjectLayer A, JPH::ObjectLayer B) const override {
			switch (A) {
				case CollisionLayers::STATIC:
					return B == CollisionLayers::DYNAMIC;
				case CollisionLayers::DYNAMIC:
					return true;
				default:
					return false;
			}
		}
	};

	namespace BroadPhaseLayers {
		static constexpr JPH::BroadPhaseLayer STATIC(0);
		static constexpr JPH::BroadPhaseLayer DYNAMIC(1);
	}

	class BPLayerInterfaceImpl : public JPH::BroadPhaseLayerInterface {
	public:
		uint GetNumBroadPhaseLayers() const override { return 2; }

		JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override {
			return layer == CollisionLayers::STATIC
				? BroadPhaseLayers::STATIC
				: BroadPhaseLayers::DYNAMIC;
		}
	};

	class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
	public:
		bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer bp) const override {
			switch (layer) {
				case CollisionLayers::STATIC:
					return bp == BroadPhaseLayers::DYNAMIC;
				case CollisionLayers::DYNAMIC:
					return true;
				default:
					return false;
			}
		}
	};
}

#endif //PLUENGINE_PHYSICSCOLLISIONRULES_H