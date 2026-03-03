//
// Created by Plutex on 1/5/26.
//

#ifndef PLUENGINE_SDLWINDOW_H
#define PLUENGINE_SDLWINDOW_H

#include "PluEngine/Core.h"

#ifdef PLU_PLATFORM_LINUX

#include "PluEngine/Window/Window.h"
#include <SDL2/SDL.h>
#include "SdlWindow.generated.h"

namespace Plu
{
	PLU_CLASS()
	class SDLWindow final : public IWindow
	{
		REFLECTION_BODY_SDLWINDOW()
	public:
		static void InitSDL();
		static void HandleSDLEvents();
		static void SetCursorVisibility(bool visible);

		explicit SDLWindow();
		~SDLWindow() override;

		void Init() override;
		void OnUpdate(float deltaTime) override;
		void OnEventSDL(SDL_Event* e);
		void Shutdown() override;

		bool IsRunning() override;
		void Close() override;
		int GetWindowID();

		int GetWidth() override;
		int GetHeight() override;

		void Minimize() override;
		void Maximize() override;
		bool IsMinimized() override;
		bool IsMaximized() override;

		bool IsVSyncEnabled() override;
		void SetVSyncEnabled(bool enabled) override;

		void* GetWindowHandle() override;
		void *GetGLContext() override;
		void MakeGLContextCurrent() override;
		void SwapBuffers() override;

		void SetWindowTitle(String title) override;

	private:
		SDL_Window* mWindow = nullptr;
		SDL_GLContext mGLContext = nullptr;

		int mWindowID = -1;

		bool mRunning = false;
		bool mVSyncEnabled = true;
	};

}

#endif

#endif //PLUENGINE_SDLWINDOW_H