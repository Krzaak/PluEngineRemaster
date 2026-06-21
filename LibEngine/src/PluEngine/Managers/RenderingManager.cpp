//
// Created by Plutex on 2026-02-08.
//

#include "PluEngine/Managers/RenderingManager.h"

#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "PluEngine/Application.h"
#include "PluEngine/Engine.h"
#include "PluEngine/Assets/AssetDescriptor.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/AssetTypes/Texture/Texture.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Renderer/GLTexture.h"
#include "PluEngine/Window/Window.h"
#include "PluEngine/Window/WindowManager.h"

void Plu::RenderingManager::RenderThreadEnter()
{
	PLU_CORE_TRACE("Render Thread Started");
	if (!mApplicationInfo) return;
	while (mIsRendererRunning) {
		RenderThreadLoop();
	}
	RenderThreadExit();
}

void Plu::RenderingManager::RenderThreadLoop()
{
}

void Plu::RenderingManager::RenderThreadExit()
{
	PLU_CORE_TRACE("Render Thread Exit");
}

void Plu::RenderingManager::OnStaticMeshRender(StaticMesh *staticMesh)
{
	if (mStaticMeshUsePerFrame.Contains(staticMesh->Uuid)) {
		mStaticMeshUsePerFrame[staticMesh->Uuid]++;
	} else {
		mStaticMeshUsePerFrame.Insert(staticMesh->Uuid, 1);
	}
}

Plu::RenderingManager::RenderingManager(ApplicationInfo *applicationInfo)
{
	mApplicationInfo = applicationInfo;
	PLU_CORE_TRACE("Rendering Manager Init");
}

Plu::RenderingManager::~RenderingManager()
{
	PLU_CORE_TRACE("Rendering Manager Destroy");
}

void Plu::RenderingManager::RequestTextureFromInfo(const TUsePointer<TextureInfo>& textureInfo)
{
	//Just straight up load the texture and forget. FOR NOW! :(
	if (!textureInfo) return;
	if (mTextures.Contains(textureInfo->Uuid)) return;
	TUsePointer<Texture> texture = mApplicationInfo->AppObjectManager->CreateObject(Texture::GetStaticClass());
	texture->CreateFromInfo(textureInfo.GetRaw(), false);
	mTextures.Insert(textureInfo->Uuid, mApplicationInfo->AppObjectManager->GetObjectAsOwner<Texture>(*texture->GetEngineObjectHandle()));
	mTextureUsePerFrame[textureInfo->Uuid] = 0;
	PLU_CORE_INFO("Texture {} Loaded!", textureInfo->Uuid.getUUID());
}

Plu::TUsePointer<Plu::Texture> Plu::RenderingManager::GetTextureForInfo(const TUsePointer<TextureInfo>& textureInfo)
{
	if (!textureInfo) return nullptr;
	if (mTextures.Contains(textureInfo->Uuid)) {
		mTextureUsePerFrame[textureInfo->Uuid]++;
		return mTextures[textureInfo->Uuid];
	}
	return nullptr;
}

void Plu::RenderingManager::UnloadTextureForUUID(UInt64 uuid)
{
	if (!mTextures.Contains(uuid)) return;
	TOwningPointer<Texture> texture = mTextures[uuid];
	texture->Destroy();
	mApplicationInfo->AppObjectManager->DestroyObject(*texture->GetEngineObjectHandle());
	mTextures.Remove(uuid);
	mTextureUsePerFrame.Remove(uuid);
	mTextureFramesWithNoUse.Remove(uuid);
	PLU_CORE_INFO("Unloaded {}", uuid);
}

void Plu::RenderingManager::RequestStaticMeshLoad(PluUUID uuid)
{
	TUsePointer<EngineAssetManager> assetManager = mApplicationInfo->AppAssetManager;
	if (!assetManager) return;
	if (!assetManager->AssetExists(uuid)) return;
	TUsePointer<AssetDescriptor> assetDesc = assetManager->GetAssetDescriptor(uuid);
	if (!assetDesc->AssetType->IsDerivedOfOrSame(StaticMesh::GetStaticClass())) return;
	TUsePointer<StaticMesh> staticMesh = assetManager->GetAssetData(assetDesc);
	if (staticMesh->IsLoaded) return;
	SetupStaticMeshGL(&staticMesh->StaticMeshData, staticMesh.GetRaw());
	mStaticMeshes.Insert(staticMesh->Uuid, staticMesh);
	PLU_CORE_INFO("Static Mesh {} Loaded!", staticMesh->Uuid.getUUID());
}

void Plu::RenderingManager::UnloadStaticMesh(PluUUID uuid)
{
	if (!mStaticMeshes.Contains(uuid)) return;
	CleanupStaticMeshGL(mStaticMeshes[uuid].GetRaw());
	mStaticMeshes.Remove(uuid);
	mStaticMeshFramesWithNoUse[uuid] = 0;
	PLU_CORE_INFO("Static Mesh {} Unloaded", uuid.getUUID());
}

void Plu::RenderingManager::Initialize()
{
	mIsRendererRunning = true;
	mRenderThread = CreateOwning<std::thread>(&RenderingManager::RenderThreadEnter, this);
	mRenderThread->detach();
}

void Plu::RenderingManager::Tick(float deltaTime)
{
	for (const auto& textureId : mTextureUsePerFrame) {
		int uses = mTextureUsePerFrame[textureId.first];
		if (uses == 0) {
			if (mTextureFramesWithNoUse.Contains(textureId.first)) {
				mTextureFramesWithNoUse[textureId.first]++;
			} else {
				mTextureFramesWithNoUse.Insert(textureId.first, 0);
			}
		}
	}
	for (const auto& textureId : mTextureUsePerFrame) {
		mTextureUsePerFrame[textureId.first] = 0;
	}
	for (std::pair<unsigned long, int> textureIDp: mTextureFramesWithNoUse) {
		if (textureIDp.second > 100) {
			UnloadTextureForUUID(textureIDp.first);
			mTextureFramesWithNoUse[textureIDp.first] = 0;
			break;
		}
	}

	//Meshes
	for (const auto& mesh : mStaticMeshUsePerFrame) {
		int uses = mStaticMeshUsePerFrame[mesh.first];
		if (uses == 0) {
			if (mStaticMeshFramesWithNoUse.Contains(mesh.first)) {
				mStaticMeshFramesWithNoUse[mesh.first]++;
			} else {
				mStaticMeshFramesWithNoUse.Insert(mesh.first, 0);
			}
		}
	}
	for (const auto& mesh : mStaticMeshUsePerFrame) {
		mStaticMeshUsePerFrame[mesh.first] = 0;
	}
	for (std::pair<unsigned long, int> mesh: mStaticMeshFramesWithNoUse) {
		if (mesh.second > 100) {
			UnloadStaticMesh(mesh.first);
			mStaticMeshFramesWithNoUse[mesh.first] = 0;
			break;
		}
	}
}

void Plu::RenderingManager::Shutdown()
{
	PLU_CORE_TRACE("Rendering Manager Shutdown");
	mIsRendererRunning = false;
	DynamicArray<UInt64> textureIds;
	for (const auto& texture : mTextures) {
		textureIds.PushBack(texture.first);
	}
	for (const auto& txt : textureIds) {
		UnloadTextureForUUID(txt);
	}	
}
