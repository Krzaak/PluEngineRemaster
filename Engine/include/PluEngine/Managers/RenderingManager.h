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
	//One of the few managers that isn't Editor or Runtime.
	//This manager keeps track of all textures, shaders and loaded meshes
	//Mostly textures are cared about right now
	PLU_CLASS(Abstract)
	class PLU_API RenderingManager final : public EngineObject
	{
		REFLECTION_BODY_RENDERINGMANAGER()
	private:
		ApplicationInfo* mApplicationInfo;
	public:
		RenderingManager(ApplicationInfo* applicationInfo);
		virtual ~RenderingManager() override;

		void Tick(float deltaTime);
		void Shutdown();
	};
}

#endif //PLUENGINE_RENDERINGMANAGER_H
