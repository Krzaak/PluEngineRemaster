//
// Created by Plutex on 12/30/25.
//

#include "PluEngine/Renderer/Renderer.h"

#include <iostream>
#include <chrono>
#include <SDL_video.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "PluEngine/Application.h"
#include "PluEngine/Objects/EngineObjectHandle.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Renderer/FrameBuffer.h"

#ifdef PLU_PLATFORM_LINUX
#include "glad.h"
#include "Platforms/Linux/GlfwWindow.h"
#include <GLFW/glfw3.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_sdl2.h>
#include <imgui/backends/imgui_impl_sdlrenderer2.h>
#elif defined(PLU_PLATFORM_WINDOWS)
#include "Platforms/Windows/WindowsWindow.h"
#include "glad/glad_wgl.h"
#include "imgui/backends/imgui_impl_win32.h"
#endif

#include "PluEngine/Core.h"
#include "PluEngine/Log.h"

using namespace Plu;

Renderer::Renderer() : mApplication(nullptr)
{
}

void Renderer::Init(Application *application)
{
	mApplication = application;
}

Renderer::~Renderer()
{
}

void Renderer::Init(const TUsePointer<IWindow>& appWindow)
{
#ifdef PLU_PLATFORM_LINUX
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		PLU_CORE_CRITICAL("Failed to initialize GLAD with glfw");
		if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
			PLU_CORE_CRITICAL("Failed to initialize GLAD with sdl");
			PLU_CORE_ASSERT(false, "Failed to initialize GLAD")
			return;
		}
		WindowProvider = LinuxWindowType::SDL2;
	} else {
		WindowProvider = LinuxWindowType::GLFW;
	}
	String linuxWindowInfo = "Window On Linux loaded with: ";
	linuxWindowInfo += ToString(WindowProvider);
	PLU_CORE_INFO(linuxWindowInfo.CStr());
	PLU_CORE_ASSERT(WindowProvider != LinuxWindowType::Unknown, "Window cannot be Unknown on Linux!")
#endif

	glViewport(0,0,1,1);
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	PLU_CORE_INFO("ImGui initialized");

	ImGuiStyle& style = ImGui::GetStyle();
        //style.ScaleAllSizes(mainScale);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0,0,0,0);
        style.WindowRounding = 12.0f;
        style.ChildRounding = 12.0f;
        style.FrameRounding = 8.0f;
        style.PopupRounding = 10.0f;
        style.ScrollbarRounding = 12.0f;
        style.GrabRounding = 6.0f;
        style.TabRounding = 8.0f;

        style.WindowBorderSize = 0.0f;
        style.FrameBorderSize = 0.0f;
        style.PopupBorderSize = 0.0f;

        style.WindowPadding = ImVec2(12, 12);
        style.FramePadding = ImVec2(8, 6);

        ImVec4* colors = style.Colors;

        // Szklane, lekko mleczne tła
        colors[ImGuiCol_WindowBg]           = ImVec4(0.12f, 0.12f, 0.12f, 0.60f);
        colors[ImGuiCol_ChildBg]            = ImVec4(0.12f, 0.12f, 0.12f, 0.40f);
        colors[ImGuiCol_PopupBg]            = ImVec4(0.10f, 0.10f, 0.10f, 0.70f);

        // Kolory kontrolne
        colors[ImGuiCol_FrameBg]            = ImVec4(0.20f, 0.20f, 0.20f, 0.30f);
        colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.25f, 0.25f, 0.25f, 0.55f);
        colors[ImGuiCol_FrameBgActive]      = ImVec4(0.30f, 0.30f, 0.30f, 0.75f);

        colors[ImGuiCol_Button]             = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
        colors[ImGuiCol_ButtonHovered]      = ImVec4(0.30f, 0.30f, 0.30f, 0.65f);
        colors[ImGuiCol_ButtonActive]       = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);

        // Taby / DockSpace
        colors[ImGuiCol_Tab]                = ImVec4(0.20f, 0.20f, 0.20f, 0.60f);
        colors[ImGuiCol_TabHovered]         = ImVec4(0.45f, 0.45f, 0.45f, 0.80f);
        colors[ImGuiCol_TabActive]          = ImVec4(0.35f, 0.35f, 0.35f, 0.85f);

        // Tekst
        colors[ImGuiCol_Text]               = ImVec4(1, 1, 1, 1);
        colors[ImGuiCol_TextDisabled]       = ImVec4(1, 1, 1, 0.40f);

        colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
        colors[ImGuiCol_TabDimmed] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

#ifdef PLU_PLATFORM_LINUX
	switch (WindowProvider) {
		case LinuxWindowType::Unknown:
			break;
		case LinuxWindowType::SDL2:
		{
			SDL_Window* windowHandle = static_cast<SDL_Window *>(appWindow->GetWindowHandle());
			SDL_GLContext* glContext = static_cast<SDL_GLContext*>(appWindow->GetGLContext());
			ImGui_ImplSDL2_InitForOpenGL(windowHandle, glContext);
			ImGui_ImplOpenGL3_Init("#version 450");
			PLU_CORE_WARN("SDL2 and OpenGL ImGui");
			break;
		}
		case LinuxWindowType::GLFW:
		{
			GLFWwindow* windowHandle = static_cast<GLFWwindow*>(appWindow->GetWindowHandle());
			ImGui_ImplGlfw_InitForOpenGL(windowHandle, true);
			ImGui_ImplOpenGL3_Init("#version 450");
			PLU_CORE_WARN("GLFW and OpenGL ImGui");
			break;
		}
		default: ;
	}
#elif defined(PLU_PLATFORM_WINDOWS)
	HWND windowHandle = static_cast<HWND>(appWindow->GetWindowHandle());
	ImGui_ImplWin32_Init(windowHandle);
	ImGui_ImplOpenGL3_Init("#version 450");
	PLU_CORE_WARN("Windows and OpenGL ImGui");
#endif

	FramebufferDesc mainBufferDesc;
	mainBufferDesc.height = mApplication->GetAppWindow()->GetHeight();
	mainBufferDesc.width = mApplication->GetAppWindow()->GetWidth();
	const EngineObjectHandle mainBufferHandle = mApplication->GetAppObjectManager()->CreateObject<FrameBuffer>();
	mMainBuffer = mApplication->GetAppObjectManager()->GetObjectAsOwner<FrameBuffer>(mainBufferHandle);
	mMainBuffer->Init(mainBufferDesc);
}

void Renderer::OnUpdate(float deltaTime)
{
	mMainBuffer->bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

	static double period = 0.000000003f;
	double sineWave = (std::sin(period * std::chrono::high_resolution_clock::now().time_since_epoch().count()) + 1) / 2.0f;
	glClearColor(sineWave, sineWave, sineWave, 1.0f);
	FrameBuffer::unbind();

	ImGui_ImplOpenGL3_NewFrame();
#ifdef PLU_PLATFORM_LINUX
	if (WindowProvider == LinuxWindowType::SDL2) {
		ImGui_ImplSDL2_NewFrame();
	} else if (WindowProvider == LinuxWindowType::GLFW) {
		ImGui_ImplGlfw_NewFrame();
	}
#elif defined(PLU_PLATFORM_WINDOWS)
	ImGui_ImplWin32_NewFrame();
#endif
	ImGui::NewFrame();

	mApplication->OnImGuiRender();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::OnShutdown()
{
#ifdef PLU_PLATFORM_LINUX
	if (WindowProvider == LinuxWindowType::SDL2) {
		ImGui_ImplSDL2_Shutdown();
	} else if (WindowProvider == LinuxWindowType::GLFW) {
		ImGui_ImplGlfw_Shutdown();
	}
#elif defined(PLU_PLATFORM_WINDOWS)
	ImGui_ImplWin32_Shutdown();
#endif
	ImGui_ImplOpenGL3_Shutdown();
	ImGui::DestroyContext();
}
