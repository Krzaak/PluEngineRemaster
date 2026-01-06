#include <cstring>
#include "PluEngine/Renderer/FrameBuffer.h"
#include "glad/glad.h"
using namespace Plu;

static void throwIfGLError(const std::string &where) {
    GLenum e = glGetError();
    if (e != GL_NO_ERROR) {
        throw std::runtime_error(where + " - GL error: " + std::to_string(e));
    }
}

FrameBuffer::FrameBuffer()
= default;

void FrameBuffer::Init(const FramebufferDesc &desc)
{
    desc_ = desc;
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    createAttachments();
    checkStatus();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FrameBuffer::~FrameBuffer() {
    destroy();
}

FrameBuffer::FrameBuffer(FrameBuffer&& other) noexcept {
    fbo_ = other.fbo_;
    colorTex_ = other.colorTex_;
    colorRbo_ = other.colorRbo_;
    depthRbo_ = other.depthRbo_;
    desc_ = other.desc_;
    other.fbo_ = other.colorTex_ = other.colorRbo_ = other.depthRbo_ = 0;
}

FrameBuffer& FrameBuffer::operator=(FrameBuffer&& other) noexcept {
    if (this != &other) {
        destroy();
        fbo_ = other.fbo_;
        colorTex_ = other.colorTex_;
        colorRbo_ = other.colorRbo_;
        depthRbo_ = other.depthRbo_;
        desc_ = other.desc_;
        other.fbo_ = other.colorTex_ = other.colorRbo_ = other.depthRbo_ = 0;
    }
    return *this;
}

void FrameBuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, desc_.width, desc_.height);
}

void FrameBuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::createAttachments() {
    // Kolor
    GLenum attachment = GL_COLOR_ATTACHMENT0;
    if (desc_.useTexture && desc_.samples == 0) {
        glGenTextures(1, &colorTex_);
        glBindTexture(GL_TEXTURE_2D, colorTex_);
        glTexImage2D(GL_TEXTURE_2D, 0, desc_.colorInternalFormat, desc_.width, desc_.height, 0, desc_.colorFormat, desc_.colorType, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // jeśli kolor to depth, podpinamy jako GL_DEPTH_ATTACHMENT
        if (desc_.colorFormat == GL_DEPTH_COMPONENT ||
            desc_.colorInternalFormat == GL_DEPTH_COMPONENT ||
            desc_.colorInternalFormat == GL_DEPTH_COMPONENT16 ||
            desc_.colorInternalFormat == GL_DEPTH_COMPONENT24 ||
            desc_.colorInternalFormat == GL_DEPTH_COMPONENT32F)
        {
            attachment = GL_DEPTH_ATTACHMENT;
        }

        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, colorTex_, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        // renderbuffer for color (either because user wanted rbo or multisample)
        glGenRenderbuffers(1, &colorRbo_);
        glBindRenderbuffer(GL_RENDERBUFFER, colorRbo_);
        if (desc_.samples > 0) {
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, desc_.samples, desc_.colorInternalFormat, desc_.width, desc_.height);
        } else {
            glRenderbufferStorage(GL_RENDERBUFFER, desc_.colorInternalFormat, desc_.width, desc_.height);
        }
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRbo_);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    // Depth / Stencil
    if (desc_.hasDepth) {
        glGenRenderbuffers(1, &depthRbo_);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRbo_);
        GLenum dsFormat = desc_.hasStencil ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT24;
        if (desc_.samples > 0) {
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, desc_.samples, dsFormat, desc_.width, desc_.height);
        } else {
            glRenderbufferStorage(GL_RENDERBUFFER, dsFormat, desc_.width, desc_.height);
        }
        if (desc_.hasStencil) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRbo_);
        } else {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRbo_);
        }
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    // Ustawienia draw buffers (jeżeli w przyszłości rozszerzymy do kilku kolorów)
    if (attachment == GL_DEPTH_ATTACHMENT)
    {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    } else
    {
        GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, drawBuffers);   
    }
}

void FrameBuffer::destroy() {
    if (colorTex_) { glDeleteTextures(1, &colorTex_); colorTex_ = 0; }
    if (colorRbo_) { glDeleteRenderbuffers(1, &colorRbo_); colorRbo_ = 0; }
    if (depthRbo_) { glDeleteRenderbuffers(1, &depthRbo_); depthRbo_ = 0; }
    if (fbo_) { glDeleteFramebuffers(1, &fbo_); fbo_ = 0; }
}

void FrameBuffer::checkStatus() const {
    GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (st != GL_FRAMEBUFFER_COMPLETE) {
        std::string msg = "Framebuffer incomplete: ";
        switch (st) {
            case GL_FRAMEBUFFER_UNDEFINED: msg += "UNDEFINED"; break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: msg += "INCOMPLETE_ATTACHMENT"; break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: msg += "MISSING_ATTACHMENT"; break;
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: msg += "INCOMPLETE_DRAW_BUFFER"; break;
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: msg += "INCOMPLETE_READ_BUFFER"; break;
            case GL_FRAMEBUFFER_UNSUPPORTED: msg += "UNSUPPORTED"; break;
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: msg += "INCOMPLETE_MULTISAMPLE"; break;
            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: msg += "INCOMPLETE_LAYER_TARGETS"; break;
            default: msg += "UNKNOWN"; break;
        }
        throw std::runtime_error(msg);
    }
}

void FrameBuffer::resize(int width, int height) {
    if (height == 0 || width == 0) return;
    if (width == desc_.width && height == desc_.height) return;
    desc_.width = width;
    desc_.height = height;

    // Zniszcz i utwórz na nowo (prostota)
    if (fbo_ == 0) {
        glGenFramebuffers(1, &fbo_);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    // Usuń poprzednie attachments z GPU (ale nie generujemy nowych nazw które by kolidowały)
    if (colorTex_) { glDeleteTextures(1, &colorTex_); colorTex_ = 0; }
    if (colorRbo_) { glDeleteRenderbuffers(1, &colorRbo_); colorRbo_ = 0; }
    if (depthRbo_) { glDeleteRenderbuffers(1, &depthRbo_); depthRbo_ = 0; }

    createAttachments();
    checkStatus();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::blitTo(FrameBuffer const* target, GLbitfield mask, GLenum filter) const {
    GLint srcFbo = static_cast<GLint>(fbo_);
    GLint dstFbo = target ? static_cast<GLint>(target->fbo_) : 0;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFbo);

    GLint srcW = desc_.width, srcH = desc_.height;
    GLint dstW, dstH;
    
    if (target) {
        dstW = target->desc_.width;
        dstH = target->desc_.height;
    } else {
        // Dla default framebuffer, pobierz aktualny viewport
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        dstW = viewport[2];
        dstH = viewport[3];
    }

    glBlitFramebuffer(0, 0, srcW, srcH, 0, 0, dstW, dstH, mask, filter);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

std::vector<unsigned char> FrameBuffer::readPixelsAsBytes(GLenum format, GLenum type) const {
    if (format == 0) format = desc_.colorFormat;
    if (type == 0) type = desc_.colorType;

    size_t pixelSize = 4; // conservative for RGBA8
    if (format == GL_RGB) pixelSize = 3;

    std::vector<unsigned char> buf((size_t)desc_.width * desc_.height * pixelSize);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, desc_.width, desc_.height, format, type, buf.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return buf;
}

// ---------------- Usage example (zawarte tutaj jako komentarz) ----------------
//
// FrameBuffer::Desc d;
// d.width = 1024; d.height = 768; d.samples = 4; d.useTexture = false; d.hasStencil = true;
// FrameBuffer msFbo(d); // multisampled FBO (kolor w RBO)
//
// // Renderować: msFbo.bind(); ... rysuj ...; msFbo.unbind();
// // Blit do pojedynczego FBO z texture, żeby użyć w shaderze:
// FrameBuffer::Desc downDesc = d; downDesc.samples = 0; downDesc.useTexture = true; downDesc.width = 1024; downDesc.height = 768;
// FrameBuffer resolved(downDesc);
// msFbo.blitTo(&resolved, GL_COLOR_BUFFER_BIT, GL_NEAREST);
// // teraz resolved.colorTexture() ma teksturę z wynikami
//
// // prostszy przypadek — bez multisample:
// FrameBuffer::Desc desc; desc.width=800; desc.height=600; desc.useTexture=true; desc.hasDepth=true;
// FrameBuffer fbo(desc);
// fbo.bind(); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // rysuj
// fbo.unbind();
// GLuint tex = fbo.colorTexture(); // użyj w shaderze

// Uwaga: w projekcie, zamiast GLEW możesz użyć GLAD/GL3W — nagłówki i inicjalizacja zależą od Ciebie.

