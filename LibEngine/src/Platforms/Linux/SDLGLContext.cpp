//
// Created by Plutex on 6/22/26.
//

#include "SDLGLContext.h"

#include "glad.h"
#include "imgui.h"

#ifdef PLU_PLATFORM_LINUX
void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity,
                            GLsizei length, const char* message, const void* userParam) {
    // Ignoruj nieistotne kody błędów
    if(id == 131185 || id == 131218 || id == 131204) return;

    PLU_CORE_ERROR("---------------");
    PLU_CORE_ERROR("Debug message ( {} ): {}", id,  message);

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:         PLU_CORE_ERROR("Severity: HIGH"); break;
        case GL_DEBUG_SEVERITY_MEDIUM:       PLU_CORE_ERROR("Severity: MEDIUM"); break;
        case GL_DEBUG_SEVERITY_LOW:          PLU_CORE_ERROR("Severity: LOW"); break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: PLU_CORE_ERROR("Severity: NOTIFICATION"); break;
    }
}

void Plu::SDLGLContext::InitGLContext(TUsePointer<IWindow> window, SDL_GLContext glContext)
{
#ifdef PLU_PLATFORM_LINUX
    // SDL3's SDL_GL_GetProcAddress returns SDL_FunctionPointer (void(*)(void)); GLAD wants
    // void*(*)(const char*). The loader-fn cast is the standard glue between the two.
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
        PLU_CORE_CRITICAL("Failed to load GLAD!");
        std::terminate();
    }
    PLU_CORE_ASSERT(SDL_GL_GetCurrentContext() != nullptr, "GL Context is null!");
#endif

    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Dzięki temu callback wykona się natychmiast
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }

    glViewport(0,0,1,1);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    IMGUI_CHECKVERSION();
}
#endif
