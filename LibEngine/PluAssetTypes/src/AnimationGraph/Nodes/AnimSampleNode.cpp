//
// Created by Plutex on 7/21/26.
//

#include "PluEngine/AssetTypes/AnimationGraph/Nodes/AnimSampleNode.h"

#include <cmath>

#include "PluEngine/AssetCore/AssetDescriptor.h"
#include "PluEngine/AssetCore/EngineAssetManager.h"

namespace Plu
{
	String AnimSampleNode::GetDisplayName()
	{
#ifdef PLU_ENGINE_EDITOR_BUILD
		if (AnimationAsset) {
			return TypeRegistry::GetInstance()->GetAssetManager()->GetAssetDescriptor(AnimationAsset->Uuid)->AssetName;
		} else {
			return "Sample Animation";
		}
#else
		return "Sample Animation";
#endif
	}

	Pose AnimSampleNode::Evaluate(AnimEvalContext& context)
	{
		if (!context.TargetSkeleton) return Pose();

		const SkeletonPoseLayout& layout = context.TargetSkeleton->GetPoseLayout();
		if (!layout.IsValid()) return Pose();

		Pose pose;
		layout.MakeBindPose(pose);
		if (!AnimationAsset) return pose;

		const double duration = static_cast<double>(AnimationAsset->FramesAmount);
		double ticks = static_cast<double>(context.TimeSeconds) * AnimationAsset->FramesPerSecond;
		if (duration > 0.0)
		{
			if (context.Loop)
			{
				ticks = std::fmod(ticks, duration);
				if (ticks < 0.0) ticks += duration;
			}
			else
			{
				ticks = std::clamp(ticks, 0.0, duration);
			}
		}

		const DynamicArray<const AnimationTrack*>& binding = AnimationAsset->GetTrackBinding(*context.TargetSkeleton);
		for (UInt64 i = 0; i < layout.NodeCount() && i < binding.Size(); ++i)
		{
			if (const AnimationTrack* track = binding[i])
			{
				pose[i].Location = track->GetLocationAtTime(ticks);
				pose[i].Rotation = track->GetRotationAtTime(ticks);
				pose[i].Scale    = track->GetScaleAtTime(ticks);
			}
		}
		return pose;
	}
}
