//
// Created by Plutex on 7/25/26.
//

#ifndef PLUENGINE_ANIMTRANSFORMBONENODE_H
#define PLUENGINE_ANIMTRANSFORMBONENODE_H

#include "PluEngine/AssetTypes/AnimationGraph/AnimGraphNode.h"
#include "PluEngine/AssetTypes/Animation/BoneRef.h"
#include "AnimTransformBoneNode.generated.h"

namespace Plu
{
	// How a channel (translation/rotation/scale) is applied on top of the incoming pose.
	PLU_ENUM(PyNamespace=Plu)
	enum class EBoneModifyMode : UInt8
	{
		Ignore,  // channel untouched
		Add,     // delta added/composed on top of the existing value
		Replace  // channel overwritten outright
	};

	// Space the Translation/Rotation/Scale values are authored in.
	PLU_ENUM(PyNamespace=Plu)
	enum class EBoneTransformSpace : UInt8
	{
		Local,     // parent-space, as the pose already is
		Component, // skeleton-space (root-relative), independent of any parent's animation
		World,     // world-space, via AnimEvalContext::ComponentToWorld
		// The posed frame of another skeleton node (ReferenceBone), pre-modification. Same
		// lag-free prop-following rationale as EIKGoalSpace::Bone — e.g. a hand orientation
		// authored relative to the other hand carrying the prop.
		Bone
	};

	// Modifies one bone's transform (translation / rotation / scale), each channel independently
	// Ignore/Add/Replace, authored in Local/Component/World space. Descendants are not touched
	// directly — they ride along because the result is written back as a LOCAL delta and
	// SkeletonPoseLayout::ComposeGlobals propagates it down the subtree downstream.
	PLU_STRUCT()
	struct PLUASSETTYPES_API AnimTransformBoneNode : AnimGraphNode
	{
		REFLECTION_BODY_ANIMTRANSFORMBONENODE()

		// Bone to modify. Not a pinnable type (see IsPinnableDataType) — details panel only.
		PLU_PROPERTY()
		BoneRef Bone;

		PLU_PROPERTY()
		EBoneTransformSpace Space = EBoneTransformSpace::Local;

		// Reference node for Space == Bone; ignored in the other spaces. Read from the incoming
		// pose's globals, before this node's own modification is applied.
		PLU_PROPERTY()
		BoneRef ReferenceBone;

		PLU_PROPERTY()
		Vec3 Translation = Vec3(0.0f);

		PLU_PROPERTY()
		EBoneModifyMode TranslationMode = EBoneModifyMode::Ignore;

		// Euler angles in DEGREES (engine convention elsewhere — GameObject rotation, GetForwardVector, …),
		// converted to a quaternion delta at evaluation time.
		PLU_PROPERTY()
		Vec3 Rotation = Vec3(0.0f);

		PLU_PROPERTY()
		EBoneModifyMode RotationMode = EBoneModifyMode::Ignore;

		PLU_PROPERTY()
		Vec3 Scale = Vec3(1.0f);

		PLU_PROPERTY()
		EBoneModifyMode ScaleMode = EBoneModifyMode::Ignore;

		// Fallback while the Alpha pin is unconnected — same pattern as AnimBlendNode::Alpha.
		PLU_PROPERTY()
		float Alpha = 1.0f;

		void BuildPins() override
		{
			AddPoseInput("Pose");
			AddPoseOutput("Result");
			BuildDataPinsFromReflection(); // adds "Translation"/"Rotation"/"Scale"/"Alpha" data input pins
		}

		String GetDisplayName() override;

		Pose Evaluate(AnimEvalContext& context) override;
	};
}

#endif //PLUENGINE_ANIMTRANSFORMBONENODE_H
