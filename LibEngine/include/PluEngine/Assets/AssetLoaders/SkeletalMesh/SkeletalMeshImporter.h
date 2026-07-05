//
// Created by Plutex on 7/5/26.
//

#ifndef PLUENGINE_SKELETALMESHIMPORTER_H
#define PLUENGINE_SKELETALMESHIMPORTER_H
#include "PluEngine/Assets/EngineAssetManager.h"

namespace Plu
{
        struct SkeletalMeshImportOptions;
        struct Skeleton;

        bool ImportSkeletalMesh(Path skeletonPath, Path outDir, TUsePointer<EngineAssetManager> assetManager, SkeletalMeshImportOptions options);

        // Binary (de)serialization of a Skeleton tree. Overloads accept Path or PathW.
        bool SaveSkeleton(const Path& path, const Skeleton& skeleton);
        bool SaveSkeleton(const PathW& path, const Skeleton& skeleton);
        bool LoadSkeleton(const Path& path, Skeleton& outSkeleton);
        bool LoadSkeleton(const PathW& path, Skeleton& outSkeleton);
}

#endif //PLUENGINE_SKELETALMESHIMPORTER_H
