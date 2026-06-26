//
// Created by Plutex on 2026-02-08.
//

#ifndef PLUENGINE_RENDERINGMANAGER_H
#define PLUENGINE_RENDERINGMANAGER_H

#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "RenderingManager.generated.h"
#include "PluEngine/PluUUID.h"
#include "PluEngine/Threading/TripleBuffer.h"

struct ImDrawData;

namespace Plu
{
	class FrameBuffer;
	struct RenderSnapshot;
	struct ImGuiDrawSnapshot;
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

		// Main->Render handoff of one ImGui frame (deep-copied draw data). Writer is the
		// Main thread via SubmitImGuiDrawData(), reader is the render thread loop. Same
		// lock-free triple-buffer pattern as the RenderSnapshot path.
		TripleBuffer<ImGuiDrawSnapshot*> mImguiTripleBuffer;

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

		TUsePointer<FrameBuffer> RequestMainFrameBuffer();

		// Called from the Main thread (the app that builds the UI) after ImGui::Render().
		// Deep-copies the live draw data into the triple buffer and publishes it for the
		// render thread. The engine does not drive ImGui::NewFrame()/Render() itself - the
		// app owns its ImGui frame and simply hands the result over through this API.
		void SubmitImGuiDrawData(ImDrawData* drawData);

		void Initialize(TripleBuffer<RenderSnapshot*>* tripleBuffer);
		void Tick();
		void Shutdown();
	};
}

#endif //PLUENGINE_RENDERINGMANAGER_H
