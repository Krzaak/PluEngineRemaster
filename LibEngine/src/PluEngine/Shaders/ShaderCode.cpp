//
// Created by Plutex on 6/5/26.
//

#include "PluEngine/Shaders/ShaderCode.h"

void Plu::IShaderCode::HandleLineUniforms(String line)
{
    DynamicArray<String> info = line.Split(' ');
    const String& name = info[1];
    const String& type = info[0];
    int arraySize = 0;
    if (info.Size() > 2) {
        arraySize = info[2].ToInt();
    }
    if (type == "int") {
        TOwningPointer intUniform = CreateOwning<ShaderUniform<int>>();
        mUniforms.PushBack(intUniform);
        intUniform->Name = name;
        intUniform->Type = type;
        intUniform->ArraySize = arraySize;
    } else if (type == "float") {
        TOwningPointer floatUniform = CreateOwning<ShaderUniform<float>>();
        mUniforms.PushBack(floatUniform);
        floatUniform->Name = name;
        floatUniform->Type = type;
        floatUniform->ArraySize = arraySize;
    } else if (type == "vec3") {
        TOwningPointer vec3Uniform = CreateOwning<ShaderUniform<Vec3>>();
        mUniforms.PushBack(vec3Uniform);
        vec3Uniform->Name = name;
        vec3Uniform->Type = type;
        vec3Uniform->ArraySize = arraySize;
    } else if (type == "vec2") {
        TOwningPointer vec2Uniform = CreateOwning<ShaderUniform<Vec2>>();
        mUniforms.PushBack(vec2Uniform);
        vec2Uniform->Name = name;
        vec2Uniform->Type = type;
        vec2Uniform->ArraySize = arraySize;
    } else if (type == "vec4") {
        TOwningPointer vec4Uniform = CreateOwning<ShaderUniform<Vec4>>();
        mUniforms.PushBack(vec4Uniform);
        vec4Uniform->Name = name;
        vec4Uniform->Type = type;
        vec4Uniform->ArraySize = arraySize;
    } else if (type == "bool") {
        TOwningPointer boolUniform = CreateOwning<ShaderUniform<bool>>();
        mUniforms.PushBack(boolUniform);
        boolUniform->Name = name;
        boolUniform->Type = type;
        boolUniform->ArraySize = arraySize;
    } else if (type == "sampler2D") {
        TOwningPointer textureUniform = CreateOwning<ShaderUniform<TUsePointer<TextureInfo>>>();
        mUniforms.PushBack(textureUniform);
        textureUniform->Name = name;
        textureUniform->Type = type;
        textureUniform->ArraySize = arraySize;
    }
}
