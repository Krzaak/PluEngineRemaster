//
// Created by Plutex on 2026-02-09.
//

#ifndef PLUENGINE_GLTEXTURE_H
#define PLUENGINE_GLTEXTURE_H
#include "PluEngine/Objects/EngineObject.h"
#include <glad/glad.h>
#include "PluEngine/AssetTypes/Texture/Texture.h"
#include "GLTexture.generated.h"

namespace Plu
{
    PLU_CLASS()
    class PLU_API Texture : public EngineObject
    {
        REFLECTION_BODY_TEXTURE()
    public:
        Texture();
        virtual ~Texture() override;

        // Prevent copying
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        // Allow moving
        Texture(Texture&& Other) noexcept;
        Texture& operator=(Texture&& Other) noexcept;

        // Create texture from TextureInfo
        bool CreateFromInfo(TextureInfo* Info, bool GenerateMipmaps = true);

        // Create empty texture
        bool Create(Int32 Width, Int32 Height, Int32 Channels, bool GenerateMipmaps = true);

        // Create depth or depth-stencil texture (for framebuffer attachment).
        // Use16Bit: GL_DEPTH_COMPONENT16 zamiast 32F — połowa VRAM, wystarczające dla
        // ortho-projekcji o ciasnym zakresie z (mapy cieni CSM). Ignorowane przy WithStencil.
        bool CreateDepth(Int32 Width, Int32 Height, bool WithStencil = false, bool Use16Bit = false);

        // Create a layered depth texture (GL_TEXTURE_2D_ARRAY) — one slice per layer, all
        // sharing a single texture object. This is the shape shadow maps want: the CSM cascades
        // become one array the shader samples with a layer index instead of N samplers bound to
        // N units (indexing a sampler array with a non-uniform expression is formally UB).
        // Attach an individual layer to a framebuffer with FrameBuffer::CreateWithDepthTextureLayer.
        // Also usable for spot/point shadow atlases and for material texture arrays.
        bool CreateDepthArray(Int32 Width, Int32 Height, Int32 Layers, bool Use16Bit = false);

        // Streaming support
        bool StreamMipLevel(Int32 MipLevel, const unsigned char* Data);
        bool StreamMipLevel(Int32 MipLevel, Int32 Width, Int32 Height, const unsigned char* Data);
        void AllocateMipLevels(Int32 NumLevels);

        // MipMap control
        void SetBaseMipLevel(Int32 Level);
        void SetMaxMipLevel(Int32 Level);
        [[nodiscard]] Int32 GetBaseMipLevel() const { return BaseMipLevel; }
        [[nodiscard]] Int32 GetMaxMipLevel() const { return MaxMipLevel; }
        [[nodiscard]] Int32 GetMipLevelCount() const { return MipLevelCount; }

        // Calculate mip dimensions
        [[nodiscard]] Int32 GetMipWidth(Int32 MipLevel) const;
        [[nodiscard]] Int32 GetMipHeight(Int32 MipLevel) const;

        // Bind/Unbind
        void Bind(GLuint TextureUnit = 0) const;
        void Unbind() const;

        // Getters
        [[nodiscard]] GLuint GetID() const { return TextureID; }
        [[nodiscard]] Int32 GetWidth() const { return Width; }
        [[nodiscard]] Int32 GetHeight() const { return Height; }
        [[nodiscard]] Int32 GetChannels() const { return Channels; }
        [[nodiscard]] bool IsValid() const { return TextureID != 0; }
        [[nodiscard]] bool IsDepth() const { return bIsDepth; }
        // GL texture target this object was created with (GL_TEXTURE_2D unless it is an array).
        [[nodiscard]] GLenum GetTarget() const { return Target; }
        [[nodiscard]] Int32 GetLayerCount() const { return Layers; }
        [[nodiscard]] bool IsArray() const { return Target == GL_TEXTURE_2D_ARRAY; }

        // Texture parameters
        void SetWrapMode(GLenum WrapS, GLenum WrapT);
        void SetFilterMode(GLenum MinFilter, GLenum MagFilter);

        // Cleanup
        void Destroy();

        //Saving
        void SaveTexture(Path path);

    private:
        GLuint TextureID;
        // GL target of this texture. Every generic operation (Bind/SetWrapMode/SetFilterMode/
        // Destroy) goes through it, so a 2D texture and a 2D array behave identically. The
        // operations that are inherently 2D (mip streaming, SaveTexture, CreateFromInfo) assert
        // on it instead of silently doing the wrong thing.
        GLenum Target;
        Int32 Width;
        Int32 Height;
        Int32 Layers;   // 1 for GL_TEXTURE_2D, layer count for GL_TEXTURE_2D_ARRAY
        Int32 Channels;
        Int32 BaseMipLevel;
        Int32 MaxMipLevel;
        Int32 MipLevelCount;
        bool bIsDepth;

        [[nodiscard]] GLenum GetInternalFormat() const;
        [[nodiscard]] GLenum GetFormat() const;
        [[nodiscard]] Int32 CalculateMipLevelCount() const;
    };
}

#endif //PLUENGINE_GLTEXTURE_H