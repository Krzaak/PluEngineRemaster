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
		DynamicArray<WindowProperties> mWindowsToAdd;
		DynamicArray<int> mWindowsToRemove;
	public:
		WindowsManager();
		~WindowsManager() override;

		void Init(const TUsePointer<EngineObjectManager> &objectManager, ApplicationInfo* applicationInfo);
		void AddWindow(const WindowProperties& windowProperties);
		void UpdateEvents() const;
		void ProcessNewWindows();
		void CloseWindow(int windowID);

		TUsePointer<IWindow> GetFirstWindow();
		[[nodiscard]] UInt64 GetWindowsAmount() const;
		TUsePointer<IWindow> GetWindowAt(UInt64 index);

	};
}

#endif //PLUENGINE_WINDOWMANAGER_H