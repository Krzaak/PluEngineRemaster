//
// Created by Plutex on 2026-02-08.
//

#ifndef PLUENGINE_RENDERINGMANAGER_H
#define PLUENGINE_RENDERINGMANAGER_H

#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "RenderingManager.generated.h"
#include "PluEngine/PluUUID.h"

namespace Plu
{
	struct StaticMesh;
	class Texture;
	struct TextureInfo;
	//One of the few managers that isn't Editor or Runtime.
	//This manager keeps track of all textures, shaders and loaded meshes
	//Mostly textures are cared about right now
	PLU_CLASS(Abstract)
	class PLU_API RenderingManager final : public EngineObject
	{
		REFLECTION_BODY_RENDERINGMANAGER()
	private:
		ApplicationInfo* mApplicationInfo;

		GameHashMap<UInt64, TOwningPointer<Texture>> mTextures;
		GameHashMap<UInt64, int> mTextureFramesWithNoUse;
		GameHashMap<UInt64, int> mTextureUsePerFrame;

		GameHashMap<UInt64, TUsePointer<StaticMesh>> mStaticMeshes;
		GameHashMap<UInt64, int> mStaticMeshUsePerFrame;
		GameHashMap<UInt64, int> mStaticMeshFramesWithNoUse;

		TOwningPointer<std::thread> mRenderThread;
		std::atomic<bool> mIsRendererRunning = false;

		void RenderThreadEnter();
		void RenderThreadLoop();
		void RenderThreadExit();

		friend void DrawStaticMesh(const StaticMesh* staticMesh, RenderingManager* renderingManager);
		void OnStaticMeshRender(StaticMesh *staticMesh);
	public:
		RenderingManager(ApplicationInfo* applicationInfo);
		virtual ~RenderingManager() override;

		//Requesting Resources
		void RequestTextureFromInfo(const TUsePointer<TextureInfo>& textureInfo);
		TUsePointer<Texture> GetTextureForInfo(const TUsePointer<TextureInfo>& textureInfo);
		void UnloadTextureForUUID(UInt64 uuid);

		void RequestStaticMeshLoad(PluUUID uuid);
		void UnloadStaticMesh(PluUUID uuid);

		void Initialize();
		void Tick(float deltaTime);
		void Shutdown();
	};
}

#endif //PLUENGINE_RENDERINGMANAGER_H
