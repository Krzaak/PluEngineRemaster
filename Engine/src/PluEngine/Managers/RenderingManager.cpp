//
// Created by Plutex on 2026-02-08.
//

#include "PluEngine/Managers/RenderingManager.h"

#include "PluEngine/Application.h"
#include "PluEngine/AssetTypes/Texture/Texture.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Renderer/GLTexture.h"

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
	TOwningPointer<Texture> texture = mApplicationInfo->AppObjectManager->CreateObject(Texture::GetStaticClass());
	texture->CreateFromInfo(textureInfo.GetRaw(), false);
	mTextures.Insert(textureInfo->Uuid, texture);
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
	TOwningPointer<Texture> texture = mTextures[uuid];
	texture->Destroy();
	mApplicationInfo->AppObjectManager->DestroyObject(*texture->GetEngineObjectHandle());
	mTextures.Remove(uuid);
	mTextureUsePerFrame.Remove(uuid);
	mTextureFramesWithNoUse.Remove(uuid);
	PLU_CORE_INFO("Unloaded {}", uuid);
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
}

void Plu::RenderingManager::Shutdown()
{
	PLU_CORE_TRACE("Rendering Manager Shutdown");
}
