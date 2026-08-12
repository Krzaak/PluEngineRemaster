//
// Created by Plutex on 2026-06-29.
//

#ifndef PLUENGINE_PLUPHYSICSMATERIAL_H
#define PLUENGINE_PLUPHYSICSMATERIAL_H

#include "PluEngine/Core.h"

namespace Plu
{
	// Per-sub-shape physical surface: friction/restitution (combined per-contact in the contact
	// listener — a JPH::Body has a single body-level value, so these can't live on the body) plus
	// the UE-style collision channel index (overrides the body's CollisionGroup::GroupID fallback).
	struct PhysicsMaterialData
	{
		float  Friction              = 0.2f;
		float  Restitution           = 0.0f;
		UInt32 CollisionProfileIndex = 0;
	};

	// We store this in a Jolt leaf shape's 64-bit user data (Shape::SetUserData), read back per
	// sub-shape via Shape::GetSubShapeUserData (Jolt forwards it through compound/scaled shapes to
	// the leaf). This avoids subclassing JPH::PhysicsMaterial, which fails to link here: Jolt is
	// built without C++ RTTI, so an RTTI-enabled subclass references a missing base typeinfo (the
	// same reason we don't subclass JPH::GroupFilter — see PhysicsCollisionRules.h).
	//
	// Layout: [ present:1 (bit63) | unused | profileIndex:16 (bits32..47) | friction:16 | restitution:16 ].
	// Friction/restitution are quantized at 1e-4 (range 0..6.5535). The present bit disambiguates an
	// all-zero material from "no user data" (Jolt's default 0, e.g. mesh collision shapes).
	namespace PhysMat
	{
		static constexpr UInt64 PresentBit = (UInt64(1) << 63);
		static constexpr float  QuantMax   = 6.5535f; // UInt16 max / 10000

		inline UInt16 Quantize(float v)
		{
			float c = v < 0.0f ? 0.0f : (v > QuantMax ? QuantMax : v);
			return static_cast<UInt16>(c * 10000.0f + 0.5f);
		}
	}

	inline UInt64 PackPhysicsMaterial(const PhysicsMaterialData& d)
	{
		return PhysMat::PresentBit
		     | (static_cast<UInt64>(d.CollisionProfileIndex & 0xFFFF) << 32)
		     | (static_cast<UInt64>(PhysMat::Quantize(d.Friction)) << 16)
		     | static_cast<UInt64>(PhysMat::Quantize(d.Restitution));
	}

	// Returns false (and leaves out untouched) when the shape carries no Plu material.
	inline bool TryUnpackPhysicsMaterial(UInt64 packed, PhysicsMaterialData& out)
	{
		if (!(packed & PhysMat::PresentBit)) return false;
		out.CollisionProfileIndex = static_cast<UInt32>((packed >> 32) & 0xFFFF);
		out.Friction    = static_cast<float>((packed >> 16) & 0xFFFF) / 10000.0f;
		out.Restitution = static_cast<float>(packed & 0xFFFF) / 10000.0f;
		return true;
	}
}

#endif //PLUENGINE_PLUPHYSICSMATERIAL_H
