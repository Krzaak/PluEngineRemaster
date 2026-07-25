//
// Created by Plutex on 7/24/26.
//

#ifndef PLUENGINE_ANIMGRAPHINSTANCE_H
#define PLUENGINE_ANIMGRAPHINSTANCE_H

#include "PluEngine/Core.h"
#include "PluEngine/Animation/AnimGraphVariableStore.h"
#include "Pointers/TUsePointer.h"
#include "AnimGraphInstance.generated.h"

namespace Plu
{
	struct AnimationGraph;

	// Per-user live values for one AnimationGraph. Owned by SkeletalMeshComponent
	// (SkeletalMeshComponent::EnsureAnimGraphInstance), never the asset — two components driving the
	// same graph asset each get their own instance, so SetFloat("Speed", ...) on one never touches
	// the other. See AnimGraphVariableStore for the value storage itself.
	PLU_STRUCT(PyExport)
	struct PLU_API AnimGraphInstance
	{
		REFLECTION_BODY_ANIMGRAPHINSTANCE()

#ifdef PLU_ENGINE_EDITOR_BUILD
		~AnimGraphInstance();
#endif

		// (Re)binds to `graph`: clones its default variables into this instance's store. Cheap no-op
		// when already bound to the same asset uuid at the same VariablesRevision — a change in
		// either (new graph, or the same graph's variable list edited in PIE) triggers a
		// RebuildFrom/MergeFrom on the underlying store.
		void BindTo(const TUsePointer<AnimationGraph>& graph);
		[[nodiscard]] TUsePointer<AnimationGraph> GetGraph() const { return mGraph; }
		AnimGraphVariableStore& GetVariables() { return mVariables; }

		// --- C++ ---
		template <typename T>
		bool Set(const String& name, const T& value) { return mVariables.Set<T>(name, value); }

		template <typename T>
		bool TryGet(const String& name, T& outValue) const { return mVariables.TryGet<T>(name, outValue); }

		// --- Python (typed: pybind11 can't bind the Set<T>/TryGet<T> templates above) ---
		PLU_FUNCTION(PyExport)
		bool SetFloat(const String& name, float value);
		PLU_FUNCTION(PyExport)
		float GetFloat(const String& name, float fallback = 0.0f);
		PLU_FUNCTION(PyExport)
		bool SetInt(const String& name, int value);
		PLU_FUNCTION(PyExport)
		int GetInt(const String& name, int fallback = 0);
		PLU_FUNCTION(PyExport)
		bool SetBool(const String& name, bool value);
		PLU_FUNCTION(PyExport)
		bool GetBool(const String& name, bool fallback = false);
		PLU_FUNCTION(PyExport)
		bool SetString(const String& name, String value);
		PLU_FUNCTION(PyExport)
		String GetString(const String& name, String fallback = String());
		PLU_FUNCTION(PyExport)
		bool SetVec3(const String& name, Vec3 value);
		PLU_FUNCTION(PyExport)
		Vec3 GetVec3(const String& name, Vec3 fallback = Vec3(0.0f));
		PLU_FUNCTION(PyExport)
		bool HasVariable(const String& name);
		PLU_FUNCTION(PyExport)
		DynamicArray<String> GetVariableNames();

		// Evaluation state per instance — today just the playback clock; state-machine state will
		// live here too once graphs grow one.
		float TimeSeconds = 0.0f;

#ifdef PLU_ENGINE_EDITOR_BUILD
		// Debug label set by SkeletalMeshComponent ("<GameObject> / <Component>") so the Variables
		// panel's PIE instance picker has something readable to show.
		String DebugName;

		// Live instances for one graph asset, keyed by its uuid — registered in BindTo, dropped in
		// the destructor. Raw pointers: the owning SkeletalMeshComponent (via
		// TOwningPointer<AnimGraphInstance>) is always the real owner.
		static DynamicArray<AnimGraphInstance*>& GetLiveInstances(UInt64 graphUuid);
#endif

	private:
		TUsePointer<AnimationGraph> mGraph;
		AnimGraphVariableStore mVariables;
		UInt64 mBoundGraphUuid = 0;
		UInt32 mBoundVariablesRevision = 0;
	};
}

#endif //PLUENGINE_ANIMGRAPHINSTANCE_H
