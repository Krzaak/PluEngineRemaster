//
// Created by Plutex on 7/3/26.
//

#ifndef PLUENGINE_OPENGLRENDERSTATE_H
#define PLUENGINE_OPENGLRENDERSTATE_H

#include "PluEngine/Core.h"

namespace Plu
{
    // Domyślny stan OpenGL kontekstu renderera: depth test, blending, polygon mode oraz
    // debug output (jeśli kontekst jest debugowy). Stan GL jest per-kontekst, więc
    // Initialize() musi się wykonać na wątku, na którym kontekst jest bieżący —
    // RenderingManager woła to na render threadzie zaraz po MakeGLContextCurrent().
    // Wcześniej ten setup żył tylko w linuksowym SDLGLContext, przez co na Windowsie
    // depth test nigdy nie był włączany (modele renderowały się bez sortowania głębi).
    class PLU_API OpenGLRenderState
    {
    public:
        void Initialize();

        [[nodiscard]] bool IsInitialized() const { return mInitialized; }

    private:
        bool mInitialized = false;
    };
}

#endif //PLUENGINE_OPENGLRENDERSTATE_H
