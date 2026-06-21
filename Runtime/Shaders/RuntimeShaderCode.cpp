//
// Created by Plutex on 6/5/26.
//

#include "RuntimeShaderCode.h"

#include <regex>

#include "PluEngine/PluUtils.h"

Plu::RuntimeShaderCode::RuntimeShaderCode(String lineFromArchive)
{
    String::SizeType semicolon = lineFromArchive.Find(';');
    String uuidStr = lineFromArchive.Substring(0, semicolon);
    PluUUID uuid = uuidStr.ToInt<UInt64>();
    Uuid = uuid;
    Name = uuidStr;
    mCode = lineFromArchive.Substring(semicolon + 1);
    // Decode escaped newlines used to preserve preprocessor directives during packing
    std::string code = mCode.CStr();
    code = std::regex_replace(code, std::regex(R"(\\n)"), "\n");
    mCode = code.c_str();
    RuntimeShaderCode::RenewUniforms();
}

Plu::String Plu::RuntimeShaderCode::GetCode()
{
    return mCode;
}

DynamicArray<Plu::TOwningPointer<Plu::IShaderUniform>> * Plu::RuntimeShaderCode::GetCodeUniforms()
{
    return &mUniforms;
}

void Plu::RuntimeShaderCode::RenewUniforms()
{
    mUniforms.Clear();
    PathW uniformsPath = GetExePath().GetParentPath().ToString() + L"/u" + Uuid.toString().ToWide();

#ifdef PLU_PLATFORM_WINDOWS
    if (std::filesystem::exists(uniformsPath.CStr()))
    {
        std::ifstream file(uniformsPath.CStr());
#else
    if (std::filesystem::exists(uniformsPath.ToString().ToNarrow().CStr()))
    {
        std::ifstream file(uniformsPath.ToString().ToNarrow().CStr());
#endif
        std::string line;
        while (std::getline(file, line)) {
            String linePlu = line.c_str();
            HandleLineUniforms(linePlu);
        }
        PLU_TRACE("Uniforms loaded!");
    } else {
        PLU_ERROR("Uniforms for shader {} don't exist!", Name.CStr());
    }
}
