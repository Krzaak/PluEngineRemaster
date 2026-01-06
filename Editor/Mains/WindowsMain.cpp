//
// Created by Plutex on 31.12.2025.
//

#include "PluEngine/Core.h"

#ifdef PLU_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../EditorApp.h"
#include "PluEngine/Application.h"
#include "PluEngine/Log.h"

int main(int argc, char** argv)
{
    Plu::Application* application = new Plu::PluEditor();
    application->Run();
    delete application;
    return 67;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    Plu::Application* application = new Plu::PluEditor();
    application->Run();
    delete application;
    return 67;
}

#endif