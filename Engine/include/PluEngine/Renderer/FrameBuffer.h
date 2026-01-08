#pragma once

#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP


#include <glad/glad.h>
#include <stdexcept>
#include <vector>
#include <string>
#include <iostream>

#include "FrameBuffer.generated.h"
#include "PluEngine/Objects/EngineObject.h"

namespace Plu
{
    struct FramebufferDesc {
        int width = 800;
        int height = 600;
        int samples = 0; // 0 = no multisample, >0 = multisample
        bool useTexture = true; // kolor -> texture (do shaderów) albo renderbuffer
        bool hasDepth = true; // depth (true => creates depth or depth-stencil)
        bool hasStencil = false; // if true and hasDepth => create depth-stencil
        GLenum colorInternalFormat = GL_RGBA8; // GL_RGBA16F, etc.
        GLenum colorFormat = GL_RGBA; // GL_RGBA, GL_RGB
        GLenum colorType = GL_UNSIGNED_BYTE; // GL_FLOAT for HDR
    };

    PLU_CLASS()
    class PLU_API FrameBuffer : public EngineObject
    {
        REFLECTION_BODY_FRAMEBUFFER()
        public:

        FrameBuffer();
        void Init(const FramebufferDesc& desc);
        ~FrameBuffer() override;


        // Nie kopiowalny
        FrameBuffer(const FrameBuffer&) = delete;
        FrameBuffer& operator=(const FrameBuffer&) = delete;
        // Przenoszalny
        FrameBuffer(FrameBuffer&& other) noexcept;
        FrameBuffer& operator=(FrameBuffer&& other) noexcept;


        void bind() const;
        static void unbind();


        [[nodiscard]] GLuint id() const { return fbo_; }
        [[nodiscard]] GLuint colorTexture() const { return colorTex_; } // 0 if not used
        [[nodiscard]] GLuint colorRenderbuffer() const { return colorRbo_; } // 0 if not used
        [[nodiscard]] GLuint depthRenderbuffer() const { return depthRbo_; }


        void resize(int width, int height);


        // Blit (kopiowanie) zawartości do innego FBO lub do ekranu (targetFBO = 0)
        void blitTo(FrameBuffer const* target, GLbitfield mask = GL_COLOR_BUFFER_BIT, GLenum filter = GL_NEAREST) const;


        // Odczyt pikseli z kolorowego attachmentu. Bufor musi być texture/renderbuffer z typem pasującym do desc_.
        // format i type domyślnie pasują do desc_.colorFormat/Type
        [[nodiscard]] std::vector<unsigned char> readPixelsAsBytes(GLenum format = 0, GLenum type = 0) const;


        // Sprawdź status FBO — rzuca std::runtime_error jeśli niepełny
        void checkStatus() const;


        [[nodiscard]] int width() const { return desc_.width; }
        [[nodiscard]] int height() const { return desc_.height; }
        [[nodiscard]] bool multisampled() const { return desc_.samples > 0; }


        private:
        FramebufferDesc desc_;
        GLuint fbo_ = 0;
        GLuint colorTex_ = 0; // jeśli useTexture==true i samples==0
        GLuint colorRbo_ = 0; // jeśli useTexture==false lub samples>0
        GLuint depthRbo_ = 0; // depth or depth-stencil renderbuffer


        void createAttachments();
        void destroy();
    };
#endif
}
