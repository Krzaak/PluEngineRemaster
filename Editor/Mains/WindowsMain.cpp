//
// Created by Plutex on 31.12.2025.
//

#include "PluEngine/Core.h"

#ifdef PLU_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ShellScalingApi.h>

#include "../EditorApp.h"
#include "PluEngine/Application.h"
#include "PluEngine/Log.h"



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    Plu::Application* application = new Plu::PluEditor();
    application->Run();
    delete application;
    return 0;
}

int main(int argc, char** argv)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    Plu::Application* application = new Plu::PluEditor();
    application->Run();
    delete application;
    return 0;
}

#endif