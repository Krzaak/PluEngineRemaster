//
// Created by Plutex on 2026-02-28.
//

#ifndef PLUENGINE_WINDOWMANAGER_H
#define PLUENGINE_WINDOWMANAGER_H

#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "WindowManager.generated.h"
#include "PluEngine/Application.h"

namespace Plu
{
	struct WindowProperties;
	class IWindow;
	PLU_CLASS()
	class PLU_API WindowsManager : public EngineObject
	{
		REFLECTION_BODY_WINDOWSMANAGER()
	private:
		DynamicArray<TOwningPointer<IWindow>> mWindows;
		TUsePointer<EngineObjectManager> mEngineObjectManager;
		ApplicationInfo* mApplicationInfo;
	public:
		WindowsManager();
		~WindowsManager() override;

		void Init(const TUsePointer<EngineObjectManager> &objectManager, ApplicationInfo* applicationInfo);
		void AddWindow(const WindowProperties& windowProperties);
		void UpdateEvents() const;

		TUsePointer<IWindow> GetFirstWindow();
	};
}

#endif //PLUENGINE_WINDOWMANAGER_H