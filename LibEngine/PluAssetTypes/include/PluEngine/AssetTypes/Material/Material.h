//
// Created by Plutex on 25.01.2026.
//

#ifndef PLUENGINE_MATERIAL_H
#define PLUENGINE_MATERIAL_H

#include "PluEngine/Core.h"
#include "PluEngine/Core/IAssetData.h"
#include "PluEngine/AssetTypes/ShaderCode.h"
#include "Material.generated.h"

namespace Plu
{
    struct IShaderUniform;
    struct StaticMesh;

    PLU_STRUCT(PyExport)
    struct PLUASSETTYPES_API MaterialInfo : IAssetData
    {
        REFLECTION_BODY_MATERIALINFO()
    public:
        PLU_PROPERTY(UuidFor=ShaderProgramInfo)
        PluUUID shaderProgram;

        PLU_PROPERTY()
        DynamicArray<TOwningPointer<IShaderUniform>> MaterialParameters;
    };
}

#endif //PLUENGINE_MATERIAL_H
