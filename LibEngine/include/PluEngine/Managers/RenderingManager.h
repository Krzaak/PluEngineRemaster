//
// Created by Plutex on 2026-02-08.
//

#ifndef PLUENGINE_RENDERINGMANAGER_H
#define PLUENGINE_RENDERINGMANAGER_H
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "RenderingManager.generated.h"

namespace Plu
{
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

		TOwningPointer<std::thread> mRenderThread;
		std::atomic<bool> mIsRendererRunning = false;

		void RenderThreadEnter();
		void RenderThreadLoop();
		void RenderThreadExit();

		ImDrawData* mImGuiRenderData[2] = { nullptr, nullptr };
		std::atomic<bool> mIsRenderingImGui = false;
		std::atomic<Int4> mImGuiRenderIdx = -1;
	public:
		RenderingManager(ApplicationInfo* applicationInfo);
		virtual ~RenderingManager() override;

		//Requesting Resources
		void RequestTextureFromInfo(const TUsePointer<TextureInfo>& textureInfo);
		TUsePointer<Texture> GetTextureForInfo(const TUsePointer<TextureInfo>& textureInfo);
		void UnloadTextureForUUID(UInt64 uuid);

		void Initialize();
		void Tick(float deltaTime);
		void Shutdown();
	};
}

#endif //PLUENGINE_RENDERINGMANAGER_H
