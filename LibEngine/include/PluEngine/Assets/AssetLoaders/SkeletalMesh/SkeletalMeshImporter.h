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
        struct SkeletalMesh;
        struct Animation;

        bool ImportSkeletalMesh(Path skeletonPath, Path outDir, TUsePointer<EngineAssetManager> assetManager, SkeletalMeshImportOptions options);

        // Binary (de)serialization of a Skeleton tree. Overloads accept Path or PathW.
        bool SaveSkeleton(const Path& path, const Skeleton& skeleton);
        bool SaveSkeleton(const PathW& path, const Skeleton& skeleton);
        bool LoadSkeleton(const Path& path, Skeleton& outSkeleton);
        bool LoadSkeleton(const PathW& path, Skeleton& outSkeleton);

        // Binary (de)serialization of a SkeletalMesh (geometry + skinning + skeleton ref).
        // Load resolves the referenced skeleton by UUID through the asset manager.
        bool SaveSkeletalMesh(const Path& path, const SkeletalMesh& mesh);
        bool LoadSkeletalMesh(const Path& path, SkeletalMesh& outMesh, TUsePointer<EngineAssetManager> assetManager);

        // Binary (de)serialization of an Animation (tracks + keyframes + skeleton ref).
        // Load resolves the referenced skeleton by UUID through the asset manager and
        // rebinds each track's target node by name against that skeleton.
        bool SaveAnimation(const Path& path, const Animation& animation);
        bool SaveAnimation(const PathW& path, const Animation& animation);
        bool LoadAnimation(const Path& path, Animation& outAnimation, TUsePointer<EngineAssetManager> assetManager);
        bool LoadAnimation(const PathW& path, Animation& outAnimation, TUsePointer<EngineAssetManager> assetManager);
}

#endif //PLUENGINE_SKELETALMESHIMPORTER_H
