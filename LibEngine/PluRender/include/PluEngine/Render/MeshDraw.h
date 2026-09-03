//
// Created by Plutex on 8/12/26.
//

#ifndef PLUENGINE_MESHDRAW_H
#define PLUENGINE_MESHDRAW_H
#include "PluEngine/Core.h"

namespace Plu
{
    struct StaticMesh;
    struct SkeletalMesh;
    class RenderingManager;

    // Issue the draw call for a mesh and record the use with RenderingManager. These are render
    // operations, not properties of the asset: they bind GL state and talk to the rendering
    // manager, so they live here rather than beside the mesh structs.
    //
    // Render thread only.
    PLURENDER_API void DrawStaticMesh(const StaticMesh* staticMesh, RenderingManager* renderingManager);
    PLURENDER_API void DrawStaticMeshInstanced(const StaticMesh* staticMesh, RenderingManager* renderingManager, UInt32 instanceCount);
    PLURENDER_API void DrawSkeletalMesh(const SkeletalMesh* skeletalMesh, RenderingManager* renderingManager);
}

#endif //PLUENGINE_MESHDRAW_H
