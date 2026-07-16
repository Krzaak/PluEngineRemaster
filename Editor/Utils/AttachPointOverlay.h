//
// Created by Plutex on 7/16/26.
//

#ifndef PLUENGINE_ATTACHPOINTOVERLAY_H
#define PLUENGINE_ATTACHPOINTOVERLAY_H

#include <cfloat>
#include <functional>

#include "imgui.h"
#include "glm/geometric.hpp"
#include "PluEngine/PluTypes.h"
#include "PluEngine/Assets/AssetDescriptor.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/AssetTypes/Skeleton/Skeleton.h"

namespace Plu
{
	// Marks the asset that actually owns `skeleton`'s attach points dirty.
	//
	// Use this instead of PanelChangedAsset()/ViewportChangedAsset() for any attach point edit: a
	// SkeletalMesh only *references* its skeleton by UUID (SaveSkeletalMeshImpl writes the UUID,
	// not the tree), so dirtying the host viewport's asset from SkeletalMeshViewport would flag
	// the mesh, and saving the mesh would never write the attach points out. In SkeletonViewport
	// this resolves to the viewport's own asset, so one call is right in both hosts.
	inline void MarkSkeletonAssetDirty(TUsePointer<EngineAssetManager> assetManager, const Skeleton* skeleton)
	{
		if (!assetManager || !skeleton) return;
		if (TUsePointer<AssetDescriptor> descriptor = assetManager->GetAssetDescriptor(skeleton->Uuid))
			assetManager->MarkAssetDirty(descriptor);
	}

	// Draws one skeleton attach point over a 3D preview (SkeletonViewportPanel's projected
	// preview, SkeletalMeshViewportPanel's bone overlay): a diamond at its origin plus short
	// RGB axis stubs so its rotation is readable without selecting it.
	//
	// `world` is the attach point's world matrix (parent node global * SkeletonAttachPoint::
	// GetLocalMatrix()), `axisLength` the axis stub length in the same world units — pick it
	// from the previewed skeleton's size so the marker reads the same at any authoring scale.
	// `project` maps world space to screen pixels and returns false behind the camera.
	//
	// Returns the squared pixel distance from `mouse` to the marker's origin, or FLT_MAX when
	// it isn't on screen, so callers can pick the closest marker on click.
	inline float DrawAttachPointMarker(ImDrawList* drawList, const Matrix4& world, float axisLength,
	                                   bool selected, const std::function<bool(const Vec3&, ImVec2&)>& project,
	                                   ImVec2 mouse)
	{
		const Vec3 originWorld = Vec3(world[3]);
		ImVec2 origin;
		if (!project(originWorld, origin)) return FLT_MAX;

		const ImU32 axisCols[3] = {
			IM_COL32(230, 80, 80, 255), IM_COL32(80, 220, 80, 255), IM_COL32(90, 140, 255, 255)
		};
		for (int axis = 0; axis < 3; ++axis)
		{
			const Vec3 basis = Vec3(world[axis]);
			if (glm::length(basis) < 1e-6f) continue;
			ImVec2 tip;
			if (project(originWorld + glm::normalize(basis) * axisLength, tip))
				drawList->AddLine(origin, tip, axisCols[axis], 1.5f);
		}

		const ImU32 markerCol = selected ? IM_COL32(90, 200, 255, 255) : IM_COL32(255, 150, 60, 255);
		const float r = selected ? 6.0f : 4.5f;
		drawList->AddQuadFilled(ImVec2(origin.x, origin.y - r), ImVec2(origin.x + r, origin.y),
		                        ImVec2(origin.x, origin.y + r), ImVec2(origin.x - r, origin.y), markerCol);
		if (selected)
			drawList->AddQuad(ImVec2(origin.x, origin.y - r - 3.0f), ImVec2(origin.x + r + 3.0f, origin.y),
			                  ImVec2(origin.x, origin.y + r + 3.0f), ImVec2(origin.x - r - 3.0f, origin.y),
			                  markerCol, 1.5f);

		const float dx = mouse.x - origin.x, dy = mouse.y - origin.y;
		return dx * dx + dy * dy;
	}
}

#endif //PLUENGINE_ATTACHPOINTOVERLAY_H
