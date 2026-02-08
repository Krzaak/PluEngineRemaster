//
// Created by Plutex on 2026-02-08.
//

#include "PluEngine/Managers/RenderingManager.h"

Plu::RenderingManager::RenderingManager(ApplicationInfo *applicationInfo)
{
	mApplicationInfo = applicationInfo;
	PLU_CORE_TRACE("Rendering Manager Init");
}

Plu::RenderingManager::~RenderingManager()
{
	PLU_CORE_TRACE("Rendering Manager Destroy");
}

void Plu::RenderingManager::Tick(float deltaTime)
{
}

void Plu::RenderingManager::Shutdown()
{
	PLU_CORE_TRACE("Rendering Manager Shutdown");
}
