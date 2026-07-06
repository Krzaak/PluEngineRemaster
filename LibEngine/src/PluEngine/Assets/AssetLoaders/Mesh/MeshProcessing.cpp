//
// Created by Plutex on 7/6/26.
//

#include "PluEngine/Assets/AssetLoaders/Mesh/MeshProcessing.h"

#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>
#include <cstring>

namespace Plu::MeshProcessing
{
    // Pakowanie normal (x,y,z) do formatu 10_10_10_2
    UInt32 PackNormal(const Vec3& normal)
    {
        // Zakres dla signed 10-bit: [-512, 511]
        auto Pack10 = [](float v) -> UInt32
        {
            int i = static_cast<int>(v * 511.0f);
            if (i < -512) i = -512;
            if (i >  511) i =  511;
            // Rzutuj na unsigned — poprawnie zachowuje bit znaku
            return static_cast<UInt32>(i) & 0x3FF;
        };

        return (Pack10(normal.x) << 0)
             | (Pack10(normal.y) << 10)
             | (Pack10(normal.z) << 20);
    }

    // Pakowanie tangentu (x,y,z) + znaku bitangentu (handedness) do formatu 10_10_10_2.
    // sign musi być +1.0 lub -1.0; trafia do 2-bitowego pola w (GL_INT_2_10_10_10_REV).
    UInt32 PackTangent(const Vec3& tangent, float sign)
    {
        auto Pack10 = [](float v) -> UInt32
        {
            int i = static_cast<int>(v * 511.0f);
            if (i < -512) i = -512;
            if (i >  511) i =  511;
            return static_cast<UInt32>(i) & 0x3FF;
        };

        // w: +1 -> 0x1, -1 -> 0x3 (two's complement w 2 bitach)
        UInt32 w = (sign < 0.0f) ? 0x3u : 0x1u;

        return (Pack10(tangent.x) << 0)
             | (Pack10(tangent.y) << 10)
             | (Pack10(tangent.z) << 20)
             | (w << 30);
    }

    // Pakowanie UV do formatu 16-bit
    UInt16 PackUV(float uv)
    {
        // Clamp do zakresu [0, 1] żeby obsłużyć wartości spoza standardowego zakresu
        float clampedUV = (uv < 0.0f) ? 0.0f : ((uv > 1.0f) ? 1.0f : uv);

        // Skaluj do zakresu [0, 65535]
        return static_cast<UInt16>(clampedUV * 65535.0f);
    }

    // Pakowanie koloru RGBA do UInt32
    UInt32 PackColor(const aiColor4D& color)
    {
        UInt8 r = static_cast<UInt8>(color.r * 255.0f);
        UInt8 g = static_cast<UInt8>(color.g * 255.0f);
        UInt8 b = static_cast<UInt8>(color.b * 255.0f);
        UInt8 a = static_cast<UInt8>(color.a * 255.0f);

        return (r << 0) | (g << 8) | (b << 16) | (a << 24);
    }

    // Konwersja macierzy Assimp do GLM
    glm::mat4 AssimpToGLM(const aiMatrix4x4& mat)
    {
        return glm::mat4(
            mat.a1, mat.b1, mat.c1, mat.d1,
            mat.a2, mat.b2, mat.c2, mat.d2,
            mat.a3, mat.b3, mat.c3, mat.d3,
            mat.a4, mat.b4, mat.c4, mat.d4
        );
    }

    namespace
    {
        // Most między wewnętrznym loggerem Assimpa a logami silnika — bez tego
        // ostrzeżenia parserów (FBX/glTF itp.) przepadają bezpowrotnie.
        class AssimpLogBridge final : public Assimp::LogStream
        {
        public:
            explicit AssimpLogBridge(Assimp::Logger::ErrorSeverity severity) : mSeverity(severity) {}

            void write(const char* message) override
            {
                // Assimp dokleja '\n' do każdej linii — utnij, żeby nie dublować
                std::size_t len = std::strlen(message);
                while (len > 0 && (message[len - 1] == '\n' || message[len - 1] == '\r'))
                {
                    --len;
                }

                switch (mSeverity)
                {
                    case Assimp::Logger::Err:  PLU_CORE_ERROR("[Assimp] {:.{}}", message, len); break;
                    case Assimp::Logger::Warn: PLU_CORE_WARN("[Assimp] {:.{}}", message, len);  break;
                    default:                   PLU_CORE_INFO("[Assimp] {:.{}}", message, len);  break;
                }
            }

        private:
            Assimp::Logger::ErrorSeverity mSeverity;
        };
    }

    // Jednorazowo podpina strumienie Info/Warn/Err pod DefaultLogger Assimpa.
    // attachStream przejmuje własność wskaźników.
    void EnsureAssimpLoggerAttached()
    {
        if (!Assimp::DefaultLogger::isNullLogger())
        {
            return;
        }

        Assimp::DefaultLogger::create("", Assimp::Logger::NORMAL, 0);
        Assimp::Logger* logger = Assimp::DefaultLogger::get();
        logger->attachStream(new AssimpLogBridge(Assimp::Logger::Info), Assimp::Logger::Info);
        logger->attachStream(new AssimpLogBridge(Assimp::Logger::Warn), Assimp::Logger::Warn);
        logger->attachStream(new AssimpLogBridge(Assimp::Logger::Err),  Assimp::Logger::Err);
    }
}
