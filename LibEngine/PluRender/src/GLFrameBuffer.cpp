//
// Created by Plutex on 2026-02-10.
//

#include "PluEngine/Render/GLFrameBuffer.h"

#include "PluEngine/Core/Objects/EngineObjectManager.h"

namespace Plu
{
    FrameBuffer::FrameBuffer()
        : FrameBufferID(0)
        , DepthStencilRBO(0)
        , ColorTexture(nullptr)
        , DepthTexture(nullptr)
        , Width(0)
        , Height(0)
        , DepthTextureLayer(-1)
        , Type(FrameBufferType::ColorDepth)
        , OwnsColorTexture(false)
        , OwnsDepthTexture(false)
        , Use16BitDepth(false)
    {
    }

    FrameBuffer::~FrameBuffer()
    {
        Destroy();
    }

    FrameBuffer::FrameBuffer(FrameBuffer&& Other) noexcept
        : FrameBufferID(Other.FrameBufferID)
        , DepthStencilRBO(Other.DepthStencilRBO)
        , ColorTexture(Other.ColorTexture)
        , DepthTexture(Other.DepthTexture)
        , Width(Other.Width)
        , Height(Other.Height)
        , DepthTextureLayer(Other.DepthTextureLayer)
        , Type(Other.Type)
        , OwnsColorTexture(Other.OwnsColorTexture)
        , OwnsDepthTexture(Other.OwnsDepthTexture)
        , Use16BitDepth(Other.Use16BitDepth)
    {
        Other.FrameBufferID = 0;
        Other.DepthStencilRBO = 0;
        Other.ColorTexture = nullptr;
        Other.DepthTexture = nullptr;
        Other.Width = 0;
        Other.Height = 0;
        Other.DepthTextureLayer = -1;
        Other.Type = FrameBufferType::ColorDepth;
        Other.OwnsColorTexture = false;
        Other.OwnsDepthTexture = false;
        Other.Use16BitDepth = false;
    }

    FrameBuffer& FrameBuffer::operator=(FrameBuffer&& Other) noexcept
    {
        if (this != &Other)
        {
            Destroy();

            FrameBufferID = Other.FrameBufferID;
            DepthStencilRBO = Other.DepthStencilRBO;
            ColorTexture = Other.ColorTexture;
            DepthTexture = Other.DepthTexture;
            Width = Other.Width;
            Height = Other.Height;
            DepthTextureLayer = Other.DepthTextureLayer;
            Type = Other.Type;
            OwnsColorTexture = Other.OwnsColorTexture;
            OwnsDepthTexture = Other.OwnsDepthTexture;
            Use16BitDepth = Other.Use16BitDepth;

            Other.FrameBufferID = 0;
            Other.DepthStencilRBO = 0;
            Other.ColorTexture = nullptr;
            Other.DepthTexture = nullptr;
            Other.Width = 0;
            Other.Height = 0;
            Other.DepthTextureLayer = -1;
            Other.Type = FrameBufferType::ColorDepth;
            Other.OwnsColorTexture = false;
            Other.OwnsDepthTexture = false;
            Other.Use16BitDepth = false;
        }
        return *this;
    }

    bool FrameBuffer::Create(Int32 InWidth, Int32 InHeight, TUsePointer<EngineObjectManager> engineObjectManager, FrameBufferType InType, bool InUse16BitDepth)
    {
        if (InWidth <= 0 || InHeight <= 0)
        {
            PLU_CORE_ERROR("FrameBuffer::Create - Invalid dimensions: %dx%d", InWidth, InHeight);
            return false;
        }

        Width = InWidth;
        Height = InHeight;
        Type = InType;
        Use16BitDepth = InUse16BitDepth;
        mEngineObjectManager = engineObjectManager;

        // Create framebuffer
        glGenFramebuffers(1, &FrameBufferID);
        glBindFramebuffer(GL_FRAMEBUFFER, FrameBufferID);

        // Create color attachment if needed
        if (Type == FrameBufferType::Color || Type == FrameBufferType::ColorDepth)
        {
            TUsePointer<Texture> textureUser = mEngineObjectManager->CreateObject(Texture::GetStaticClass());
            ColorTexture = mEngineObjectManager->GetObjectAsOwner<Texture>(*textureUser->GetEngineObjectHandle());
            if (!ColorTexture->Create(Width, Height, 4, false))
            {
                PLU_CORE_ERROR("FrameBuffer::Create - Failed to create color texture");
                Destroy();
                return false;
            }
            OwnsColorTexture = true;

            // Attach color texture
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 
                                   ColorTexture->GetID(), 0);
        }
        else
        {
            // For depth-only framebuffers, disable color output
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
        }

        // Create depth attachment based on type
        if (Type == FrameBufferType::ColorDepth)
        {
            // Use renderbuffer for color+depth
            if (!CreateDepthStencilRenderBuffer())
            {
                PLU_CORE_ERROR("FrameBuffer::Create - Failed to create depth-stencil buffer");
                Destroy();
                return false;
            }
        }
        else if (Type == FrameBufferType::DepthOnly || Type == FrameBufferType::DepthStencil)
        {
            // Use texture for depth-only
            if (!CreateDepthTexture())
            {
                PLU_CORE_ERROR("FrameBuffer::Create - Failed to create depth texture");
                Destroy();
                return false;
            }
        }

        // Check if framebuffer is complete
        if (!IsComplete())
        {
            PLU_CORE_ERROR("FrameBuffer::Create - Framebuffer is not complete");
            Destroy();
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        PLU_CORE_INFO("FrameBuffer created successfully: {}x{} (Type: {})", Width, Height, ToString(Type).CStr());

        return true;
    }

    bool FrameBuffer::CreateWithTexture(TOwningPointer<Texture> InColorAttachment, TUsePointer<EngineObjectManager> engineObjectManager, FrameBufferType InType)
    {
        if (InColorAttachment == nullptr || !InColorAttachment->IsValid())
        {
            PLU_CORE_ERROR("FrameBuffer::CreateWithTexture - Invalid texture");
            return false;
        }

        Width = InColorAttachment->GetWidth();
        Height = InColorAttachment->GetHeight();
        ColorTexture = InColorAttachment;
        OwnsColorTexture = false;
        Type = InType;
        mEngineObjectManager = engineObjectManager;

        // Create framebuffer
        glGenFramebuffers(1, &FrameBufferID);
        glBindFramebuffer(GL_FRAMEBUFFER, FrameBufferID);

        // Attach color texture
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 
                               ColorTexture->GetID(), 0);

        // Create depth attachment based on type
        if (Type == FrameBufferType::ColorDepth)
        {
            if (!CreateDepthStencilRenderBuffer())
            {
                PLU_CORE_ERROR("FrameBuffer::CreateWithTexture - Failed to create depth-stencil buffer");
                Destroy();
                return false;
            }
        }

        // Check if framebuffer is complete
        if (!IsComplete())
        {
            PLU_CORE_ERROR("FrameBuffer::CreateWithTexture - Framebuffer is not complete");
            Destroy();
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        PLU_CORE_INFO("FrameBuffer created with external texture: %dx%d", Width, Height);

        return true;
    }

    bool FrameBuffer::CreateDepthOnly(Int32 InWidth, Int32 InHeight, TUsePointer<EngineObjectManager> engineObjectManager)
    {
        return Create(InWidth, InHeight, engineObjectManager, FrameBufferType::DepthOnly);
    }

    bool FrameBuffer::CreateWithDepthTextureLayer(TOwningPointer<Texture> InDepthArray, Int32 InLayer, TUsePointer<EngineObjectManager> engineObjectManager)
    {
        if (InDepthArray == nullptr || !InDepthArray->IsValid())
        {
            PLU_CORE_ERROR("FrameBuffer::CreateWithDepthTextureLayer - Invalid texture");
            return false;
        }
        if (!InDepthArray->IsArray() || !InDepthArray->IsDepth())
        {
            PLU_CORE_ERROR("FrameBuffer::CreateWithDepthTextureLayer - Texture is not a depth array (create it with Texture::CreateDepthArray)");
            return false;
        }
        if (InLayer < 0 || InLayer >= InDepthArray->GetLayerCount())
        {
            PLU_CORE_ERROR("FrameBuffer::CreateWithDepthTextureLayer - Layer {} out of range (texture has {} layers)",
                           InLayer, InDepthArray->GetLayerCount());
            return false;
        }

        Width = InDepthArray->GetWidth();
        Height = InDepthArray->GetHeight();
        DepthTexture = InDepthArray;
        DepthTextureLayer = InLayer;
        // The array is shared between all the layer framebuffers — none of them owns it.
        OwnsDepthTexture = false;
        Type = FrameBufferType::DepthOnly;
        mEngineObjectManager = engineObjectManager;

        glGenFramebuffers(1, &FrameBufferID);
        glBindFramebuffer(GL_FRAMEBUFFER, FrameBufferID);

        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, DepthTexture->GetID(), 0, DepthTextureLayer);

        // Depth-only: no colour output at all.
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        if (!IsComplete())
        {
            PLU_CORE_ERROR("FrameBuffer::CreateWithDepthTextureLayer - Framebuffer is not complete");
            Destroy();
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        PLU_CORE_INFO("FrameBuffer created for depth array layer {}: {}x{}", DepthTextureLayer, Width, Height);

        return true;
    }

    void FrameBuffer::Bind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, FrameBufferID);
        glViewport(0, 0, Width, Height);
    }

    void FrameBuffer::Unbind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void FrameBuffer::BindRead() const
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, FrameBufferID);
    }

    void FrameBuffer::BindDraw() const
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FrameBufferID);
    }

    bool FrameBuffer::Resize(Int32 NewWidth, Int32 NewHeight)
    {
        if (NewWidth <= 0 || NewHeight <= 0)
        {
            PLU_CORE_ERROR("FrameBuffer::Resize - Invalid dimensions: %dx%d", NewWidth, NewHeight);
            return false;
        }

        if (NewWidth == Width && NewHeight == Height)
        {
            return true; // Already the right size
        }

        // A layer framebuffer only borrows one slice of a shared texture array — resizing it
        // would mean reallocating the whole array behind every other layer's back. The owner
        // (Renderer::RecreateShadowResources) rebuilds the array and all its layer framebuffers.
        if (DepthTextureLayer >= 0)
        {
            PLU_CORE_ASSERT(false, "FrameBuffer::Resize is not supported on a depth-array layer framebuffer");
            PLU_CORE_ERROR("FrameBuffer::Resize - Not supported on a depth-array layer framebuffer");
            return false;
        }

        Width = NewWidth;
        Height = NewHeight;

        // Recreate color texture if we own it
        if (OwnsColorTexture && ColorTexture != nullptr)
        {
            ColorTexture->Destroy();
            if (!ColorTexture->Create(Width, Height, 4, false))
            {
                PLU_CORE_ERROR("FrameBuffer::Resize - Failed to recreate color texture");
                return false;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, FrameBufferID);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 
                                   ColorTexture->GetID(), 0);
        }

        // Recreate depth texture if we own it
        if (OwnsDepthTexture && DepthTexture != nullptr)
        {
            DepthTexture->Destroy();

            bool WithStencil = (Type == FrameBufferType::DepthStencil);
            if (!DepthTexture->CreateDepth(Width, Height, WithStencil, Use16BitDepth))
            {
                PLU_CORE_ERROR("FrameBuffer::Resize - Failed to recreate depth texture");
                return false;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, FrameBufferID);
            GLenum Attachment = WithStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
            glFramebufferTexture2D(GL_FRAMEBUFFER, Attachment, GL_TEXTURE_2D,
                                   DepthTexture->GetID(), 0);
        }

        // Recreate depth-stencil renderbuffer if present
        if (DepthStencilRBO != 0)
        {
            DestroyDepthStencilRenderBuffer();
            if (!CreateDepthStencilRenderBuffer())
            {
                PLU_CORE_ERROR("FrameBuffer::Resize - Failed to recreate depth-stencil buffer");
                return false;
            }
        }

        // Check if framebuffer is still complete
        if (!IsComplete())
        {
            PLU_CORE_ERROR("FrameBuffer::Resize - Framebuffer is not complete after resize");
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return true;
    }

    bool FrameBuffer::IsComplete() const
    {
        if (!IsValid())
        {
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, FrameBufferID);
        GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (Status != GL_FRAMEBUFFER_COMPLETE)
        {
            const char* ErrorMsg = "Unknown error";
            switch (Status)
            {
                case GL_FRAMEBUFFER_UNDEFINED:
                    ErrorMsg = "Framebuffer undefined";
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                    ErrorMsg = "Incomplete attachment";
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                    ErrorMsg = "Missing attachment";
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                    ErrorMsg = "Incomplete draw buffer";
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                    ErrorMsg = "Incomplete read buffer";
                    break;
                case GL_FRAMEBUFFER_UNSUPPORTED:
                    ErrorMsg = "Framebuffer unsupported";
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                    ErrorMsg = "Incomplete multisample";
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
                    ErrorMsg = "Incomplete layer targets";
                    break;
            }
            PLU_CORE_ERROR("FrameBuffer incomplete: %s (0x%x)", ErrorMsg, Status);
            return false;
        }

        return true;
    }

    void FrameBuffer::BlitTo(FrameBuffer* Target, GLenum Filter) const
    {
        if (!IsValid())
        {
            return;
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, FrameBufferID);
        
        if (Target != nullptr && Target->IsValid())
        {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Target->GetID());
            glBlitFramebuffer(0, 0, Width, Height, 
                            0, 0, Target->GetWidth(), Target->GetHeight(),
                            GL_COLOR_BUFFER_BIT, Filter);
        }
        else
        {
            // Blit to default framebuffer (screen)
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(0, 0, Width, Height, 
                            0, 0, Width, Height,
                            GL_COLOR_BUFFER_BIT, Filter);
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    void FrameBuffer::BlitToScreen(Int32 ScreenWidth, Int32 ScreenHeight, GLenum Filter) const
    {
        if (!IsValid())
        {
            return;
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, FrameBufferID);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        
        glBlitFramebuffer(0, 0, Width, Height, 
                        0, 0, ScreenWidth, ScreenHeight,
                        GL_COLOR_BUFFER_BIT, Filter);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }

    void FrameBuffer::Clear(float R, float G, float B, float A) const
    {
        if (!IsValid())
        {
            return;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, FrameBufferID);
        
        GLbitfield ClearFlags = 0;
        
        // Clear color if we have color attachment
        if (Type == FrameBufferType::Color || Type == FrameBufferType::ColorDepth)
        {
            glClearColor(R, G, B, A);
            ClearFlags |= GL_COLOR_BUFFER_BIT;
        }
        
        // Clear depth/stencil if we have it
        if (Type == FrameBufferType::ColorDepth || 
            Type == FrameBufferType::DepthOnly || 
            Type == FrameBufferType::DepthStencil)
        {
            ClearFlags |= GL_DEPTH_BUFFER_BIT;
            
            if (Type == FrameBufferType::DepthStencil || Type == FrameBufferType::ColorDepth)
            {
                ClearFlags |= GL_STENCIL_BUFFER_BIT;
            }
        }
        
        if (ClearFlags != 0)
        {
            glClear(ClearFlags);
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void FrameBuffer::Destroy()
    {
        if (FrameBufferID != 0)
        {
            glDeleteFramebuffers(1, &FrameBufferID);
            FrameBufferID = 0;
        }

        DestroyDepthStencilRenderBuffer();

        if (OwnsColorTexture && ColorTexture != nullptr)
        {
            mEngineObjectManager->DestroyObject(*ColorTexture->GetEngineObjectHandle());
            ColorTexture = nullptr;
        }

        if (OwnsDepthTexture && DepthTexture != nullptr)
        {
            mEngineObjectManager->DestroyObject(*DepthTexture->GetEngineObjectHandle());
        }
        // Release the reference either way. A borrowed attachment (an external colour texture,
        // or one layer of a shared depth array) must NOT be destroyed here — its owner does that.
        ColorTexture = nullptr;
        DepthTexture = nullptr;

        Width = 0;
        Height = 0;
        DepthTextureLayer = -1;
        Type = FrameBufferType::ColorDepth;
        OwnsColorTexture = false;
        OwnsDepthTexture = false;
    }

    bool FrameBuffer::CreateDepthStencilRenderBuffer()
    {
        glGenRenderbuffers(1, &DepthStencilRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, DepthStencilRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, Width, Height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, 
                                 GL_RENDERBUFFER, DepthStencilRBO);

        return true;
    }

    void FrameBuffer::DestroyDepthStencilRenderBuffer()
    {
        if (DepthStencilRBO != 0)
        {
            glDeleteRenderbuffers(1, &DepthStencilRBO);
            DepthStencilRBO = 0;
        }
    }

    bool FrameBuffer::CreateDepthTexture()
    {
        TUsePointer<Texture> txtUser = mEngineObjectManager->CreateObject(Texture::GetStaticClass());
        DepthTexture = mEngineObjectManager->GetObjectAsOwner<Texture>(*txtUser->GetEngineObjectHandle());

        bool WithStencil = (Type == FrameBufferType::DepthStencil);
        if (!DepthTexture->CreateDepth(Width, Height, WithStencil, Use16BitDepth))
        {
            PLU_CORE_ERROR("FrameBuffer::CreateDepthTexture - Failed to create depth texture");
            return false;
        }

        OwnsDepthTexture = true;

        GLenum Attachment = WithStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
        glFramebufferTexture2D(GL_FRAMEBUFFER, Attachment, GL_TEXTURE_2D,
                               DepthTexture->GetID(), 0);

        return true;
    }
}
