//
// Created by Plutex on 2026-02-10.
//

#ifndef PLUENGINE_GLFRAMEBUFFER_H
#define PLUENGINE_GLFRAMEBUFFER_H

#include "PluEngine/Objects/EngineObject.h"
#include <glad/glad.h>
#include "GLTexture.h"
#include "GLFrameBuffer.generated.h"

namespace Plu
{
    PLU_ENUM(PyNamespace=Plu)
    enum class FrameBufferType
    {
        Color,              // Color attachment only
        ColorDepth,         // Color + Depth-Stencil
        DepthOnly,          // Depth attachment only
        DepthStencil        // Depth-Stencil attachment only
    };

    PLU_CLASS()
    class PLU_API FrameBuffer : public EngineObject
    {
        REFLECTION_BODY_FRAMEBUFFER()
    private:
        TUsePointer<EngineObjectManager> mEngineObjectManager;
    public:
        FrameBuffer();
        virtual ~FrameBuffer() override;

        // Prevent copying
        FrameBuffer(const FrameBuffer&) = delete;
        FrameBuffer& operator=(const FrameBuffer&) = delete;

        // Allow moving
        FrameBuffer(FrameBuffer&& Other) noexcept;
        FrameBuffer& operator=(FrameBuffer&& Other) noexcept;

        // Create framebuffer with color attachment.
        // Use16BitDepth: tekstura głębi jako D16 zamiast D32F (patrz Texture::CreateDepth) —
        // dotyczy tylko DepthOnly (DepthStencil ma stały format D24S8).
        bool Create(Int32 Width, Int32 Height, TUsePointer<EngineObjectManager> engineObjectManager, FrameBufferType Type = FrameBufferType::ColorDepth, bool Use16BitDepth = false);

        // Create framebuffer with existing texture
        bool CreateWithTexture(TOwningPointer<Texture> ColorAttachment, TUsePointer<EngineObjectManager> engineObjectManager, FrameBufferType Type = FrameBufferType::ColorDepth);

        // Create depth-only framebuffer
        bool CreateDepthOnly(Int32 Width, Int32 Height, TUsePointer<EngineObjectManager> engineObjectManager);

        // Create a depth-only framebuffer rendering into ONE layer of an existing depth texture
        // array (Texture::CreateDepthArray). The array is shared: this framebuffer does not own
        // it and never destroys or resizes it — several layer framebuffers point at the same
        // texture (that is how the CSM cascades share one shadow map array).
        // Size comes from the texture; Resize() is not supported on this kind of framebuffer.
        bool CreateWithDepthTextureLayer(TOwningPointer<Texture> DepthArray, Int32 Layer, TUsePointer<EngineObjectManager> engineObjectManager);

        // Bind/Unbind
        void Bind() const;
        void Unbind() const;

        // Bind for reading
        void BindRead() const;
        void BindDraw() const;

        // Resize framebuffer
        bool Resize(Int32 NewWidth, Int32 NewHeight);

        // Getters
        [[nodiscard]] GLuint GetID() const { return FrameBufferID; }
        [[nodiscard]] TUsePointer<Texture> GetColorTexture() const { return ColorTexture; }
        [[nodiscard]] TUsePointer<Texture> GetDepthTexture() const { return DepthTexture; }
        [[nodiscard]] GLuint GetDepthStencilID() const { return DepthStencilRBO; }
        [[nodiscard]] Int32 GetWidth() const { return Width; }
        [[nodiscard]] Int32 GetHeight() const { return Height; }
        [[nodiscard]] FrameBufferType GetType() const { return Type; }
        [[nodiscard]] bool IsValid() const { return FrameBufferID != 0; }
        [[nodiscard]] bool IsComplete() const;

        // Blit to another framebuffer or screen
        void BlitTo(FrameBuffer* Target, GLenum Filter = GL_LINEAR) const;
        void BlitToScreen(Int32 ScreenWidth, Int32 ScreenHeight, GLenum Filter = GL_LINEAR) const;

        // Clear framebuffer
        void Clear(float R = 0.0f, float G = 0.0f, float B = 0.0f, float A = 1.0f) const;

        // Cleanup
        void Destroy();

    private:
        GLuint FrameBufferID;
        GLuint DepthStencilRBO;
        TOwningPointer<Texture> ColorTexture;
        TOwningPointer<Texture> DepthTexture;
        Int32 Width;
        Int32 Height;
        // Layer of DepthTexture this framebuffer renders into, or -1 when the depth attachment
        // is a plain 2D texture. >= 0 means the depth texture is a shared array (see
        // CreateWithDepthTextureLayer) — such a framebuffer must never resize or destroy it.
        Int32 DepthTextureLayer;
        FrameBufferType Type;
        bool OwnsColorTexture;
        bool OwnsDepthTexture;
        bool Use16BitDepth;

        bool CreateDepthStencilRenderBuffer();
        void DestroyDepthStencilRenderBuffer();
        bool CreateDepthTexture();
    };
}

#endif //PLUENGINE_GLFRAMEBUFFER_H
