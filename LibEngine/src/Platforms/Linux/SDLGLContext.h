//
// Created by Plutex on 6/22/26.
//

#ifndef PLUENGINE_SDLGLCONTEXT_H
#define PLUENGINE_SDLGLCONTEXT_H

#ifdef PLU_PLATFORM_LINUX
#include <SDL3/SDL_video.h>


namespace Plu
{
    class IWindow;

    class SDLGLContext
    {
        public:
        static void InitGLContext(TUsePointer<IWindow> window, SDL_GLContext glContext);
    };
}
#endif


#endif //PLUENGINE_SDLGLCONTEXT_H
