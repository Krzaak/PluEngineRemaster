//
// Created by Plutex on 2026-02-09.
//

#include "PluEngine/Renderer/GLTexture.h"
#include <algorithm>

namespace Plu
{
    void CheckGLError(const char* where)
    {
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
        {
            const char* error;
            switch(err)
            {
                case GL_INVALID_ENUM: error = "INVALID_ENUM"; break;
                case GL_INVALID_VALUE: error = "INVALID_VALUE"; break;
                case GL_INVALID_OPERATION: error = "INVALID_OPERATION"; break;
                case GL_OUT_OF_MEMORY: error = "OUT_OF_MEMORY"; break;
                default: error = "UNKNOWN"; break;
            }
            PLU_CORE_ERROR("OpenGL Error at %s: %s (0x%x)\n", where, error, err);
        }
    }

    Texture::Texture()
        : TextureID(0)
        , Width(0)
        , Height(0)
        , Channels(0)
        , BaseMipLevel(0)
        , MaxMipLevel(1000)
        , MipLevelCount(1)
    {
    }

    Texture::~Texture()
    {
        Destroy();
    }

    Texture::Texture(Texture&& Other) noexcept
        : TextureID(Other.TextureID)
        , Width(Other.Width)
        , Height(Other.Height)
        , Channels(Other.Channels)
        , BaseMipLevel(Other.BaseMipLevel)
        , MaxMipLevel(Other.MaxMipLevel)
        , MipLevelCount(Other.MipLevelCount)
    {
        Other.TextureID = 0;
        Other.Width = 0;
        Other.Height = 0;
        Other.Channels = 0;
        Other.BaseMipLevel = 0;
        Other.MaxMipLevel = 1000;
        Other.MipLevelCount = 1;
    }

    Texture& Texture::operator=(Texture&& Other) noexcept
    {
        if (this != &Other)
        {
            Destroy();

            TextureID = Other.TextureID;
            Width = Other.Width;
            Height = Other.Height;
            Channels = Other.Channels;
            BaseMipLevel = Other.BaseMipLevel;
            MaxMipLevel = Other.MaxMipLevel;
            MipLevelCount = Other.MipLevelCount;

            Other.TextureID = 0;
            Other.Width = 0;
            Other.Height = 0;
            Other.Channels = 0;
            Other.BaseMipLevel = 0;
            Other.MaxMipLevel = 1000;
            Other.MipLevelCount = 1;
        }
        return *this;
    }

    bool Texture::CreateFromInfo(TextureInfo* Info, bool GenerateMipmaps)
    {
        if (Info->Data == nullptr || Info->Width <= 0 || Info->Height <= 0)
        {
            return false;
        }

        Width = Info->Width;
        Height = Info->Height;
        Channels = Info->Channels;

        glGenTextures(1, &TextureID);
        glBindTexture(GL_TEXTURE_2D, TextureID);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // Set default texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        if (!GenerateMipmaps)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Bez MIPMAP
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        // Upload texture data
        glTexImage2D(GL_TEXTURE_2D, 0, GetInternalFormat(), Width, Height, 0,
                     GetFormat(), GL_UNSIGNED_BYTE, Info->Data);

        CheckGLError("After glTexImage2D");

        if (GenerateMipmaps)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
            MipLevelCount = CalculateMipLevelCount();
        }
        else
        {
            MipLevelCount = 1;
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        PLU_CORE_INFO("Texture Created with Major Success!");
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        return true;
    }

    bool Texture::Create(Int32 InWidth, Int32 InHeight, Int32 InChannels, bool GenerateMipmaps)
    {
        Width = InWidth;
        Height = InHeight;
        Channels = InChannels;

        glGenTextures(1, &TextureID);
        glBindTexture(GL_TEXTURE_2D, TextureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (GenerateMipmaps)
        {
            MipLevelCount = CalculateMipLevelCount();
            for (Int32 Level = 0; Level < MipLevelCount; ++Level)
            {
                Int32 MipWidth = GetMipWidth(Level);
                Int32 MipHeight = GetMipHeight(Level);
                glTexImage2D(GL_TEXTURE_2D, Level, GetInternalFormat(), MipWidth, MipHeight, 0,
                             GetFormat(), GL_UNSIGNED_BYTE, nullptr);
            }
        }
        else
        {
            MipLevelCount = 1;
            glTexImage2D(GL_TEXTURE_2D, 0, GetInternalFormat(), Width, Height, 0,
                         GetFormat(), GL_UNSIGNED_BYTE, nullptr);
        }

        glBindTexture(GL_TEXTURE_2D, 0);

        return true;
    }

    bool Texture::StreamMipLevel(Int32 MipLevel, const unsigned char* Data)
    {
        if (!IsValid() || Data == nullptr || MipLevel < 0 || MipLevel >= MipLevelCount)
        {
            return false;
        }

        Int32 MipWidth = GetMipWidth(MipLevel);
        Int32 MipHeight = GetMipHeight(MipLevel);

        glBindTexture(GL_TEXTURE_2D, TextureID);
        glTexSubImage2D(GL_TEXTURE_2D, MipLevel, 0, 0, MipWidth, MipHeight,
                        GetFormat(), GL_UNSIGNED_BYTE, Data);
        glBindTexture(GL_TEXTURE_2D, 0);

        return true;
    }

    bool Texture::StreamMipLevel(Int32 MipLevel, Int32 MipWidth, Int32 MipHeight, const unsigned char* Data)
    {
        if (!IsValid() || Data == nullptr || MipLevel < 0)
        {
            return false;
        }

        glBindTexture(GL_TEXTURE_2D, TextureID);

        // Reallocate if dimensions don't match
        if (MipLevel == 0)
        {
            Width = MipWidth;
            Height = MipHeight;
        }

        glTexImage2D(GL_TEXTURE_2D, MipLevel, GetInternalFormat(), MipWidth, MipHeight, 0,
                     GetFormat(), GL_UNSIGNED_BYTE, Data);

        glBindTexture(GL_TEXTURE_2D, 0);

        return true;
    }

    void Texture::AllocateMipLevels(Int32 NumLevels)
    {
        if (!IsValid() || NumLevels < 1)
        {
            return;
        }

        MipLevelCount = NumLevels;

        glBindTexture(GL_TEXTURE_2D, TextureID);

        for (Int32 Level = 0; Level < MipLevelCount; ++Level)
        {
            Int32 MipWidth = GetMipWidth(Level);
            Int32 MipHeight = GetMipHeight(Level);
            glTexImage2D(GL_TEXTURE_2D, Level, GetInternalFormat(), MipWidth, MipHeight, 0,
                         GetFormat(), GL_UNSIGNED_BYTE, nullptr);
        }

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture::SetBaseMipLevel(Int32 Level)
    {
        if (!IsValid() || Level < 0)
        {
            return;
        }

        BaseMipLevel = Level;

        glBindTexture(GL_TEXTURE_2D, TextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, BaseMipLevel);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture::SetMaxMipLevel(Int32 Level)
    {
        if (!IsValid() || Level < 0)
        {
            return;
        }

        MaxMipLevel = Level;

        glBindTexture(GL_TEXTURE_2D, TextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MaxMipLevel);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    Int32 Texture::GetMipWidth(Int32 MipLevel) const
    {
        return std::max(1, Width >> MipLevel);
    }

    Int32 Texture::GetMipHeight(Int32 MipLevel) const
    {
        return std::max(1, Height >> MipLevel);
    }

    void Texture::Bind(GLuint TextureUnit) const
    {
        glActiveTexture(GL_TEXTURE0 + TextureUnit);
        glBindTexture(GL_TEXTURE_2D, TextureID);
    }

    void Texture::Unbind() const
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture::SetWrapMode(GLenum WrapS, GLenum WrapT)
    {
        if (!IsValid())
        {
            return;
        }

        glBindTexture(GL_TEXTURE_2D, TextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, WrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, WrapT);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture::SetFilterMode(GLenum MinFilter, GLenum MagFilter)
    {
        if (!IsValid())
        {
            return;
        }

        glBindTexture(GL_TEXTURE_2D, TextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, MinFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, MagFilter);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture::Destroy()
    {
        if (TextureID != 0)
        {
            glDeleteTextures(1, &TextureID);
            TextureID = 0;
            Width = 0;
            Height = 0;
            Channels = 0;
            BaseMipLevel = 0;
            MaxMipLevel = 1000;
            MipLevelCount = 1;
        }
    }

    GLenum Texture::GetInternalFormat() const
    {
        switch (Channels)
        {
            case 1: return GL_R8;
            case 2: return GL_RG8;
            case 3: return GL_RGB8;   // ⭐ Zamiast GL_RGB
            case 4: return GL_RGBA8;  // ⭐ Zamiast GL_RGBA
            default: return GL_RGBA8;
        }
    }

    GLenum Texture::GetFormat() const
    {
        switch (Channels)
        {
            case 1: return GL_RED;
            case 2: return GL_RG;
            case 3: return GL_RGB;
            case 4: return GL_RGBA;
            default: return GL_RGBA;
        }
    }

    Int32 Texture::CalculateMipLevelCount() const
    {
        Int32 MaxDim = std::max(Width, Height);
        Int32 Levels = 1;

        while (MaxDim > 1)
        {
            MaxDim >>= 1;
            ++Levels;
        }

        return Levels;
    }
}