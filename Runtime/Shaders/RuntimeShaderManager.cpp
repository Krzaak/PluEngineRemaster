//
// Created by Plutex on 6/5/26.
//

#include "RuntimeShaderManager.h"
#include "RuntimeShaderCode.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/Assets/AssetDescriptor.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/Shaders/ShaderProgram.h"

Plu::PathW Plu::RuntimeShaderWriter::GetShaderCacheDirectory()
{
    return GetExePath().GetParentPath().ToString();
}

Plu::RuntimeShaderManager::RuntimeShaderManager()
{
}

Plu::RuntimeShaderManager::~RuntimeShaderManager()
{
}

void Plu::RuntimeShaderManager::InitAssetEvents(TUsePointer<EngineAssetManager> assetManager, TUsePointer<EngineObjectManager> objectManager)
{
    mAssetManager = assetManager;
    mObjectManager = objectManager;

    SetGlobalShaderCacheWriter(mObjectManager->CreateObject(RuntimeShaderWriter::GetStaticClass()));

    mAssetManager->GetObjectEventDispatcher()->Subscribe("LoadedAssetData", [this](void* data) {
        UInt64* uuid = static_cast<UInt64*>(data);
        TUsePointer<AssetDescriptor> assetDescriptor = mAssetManager->GetAssetDescriptor(*uuid);
        if (!assetDescriptor) return;
        if (assetDescriptor->AssetType == ShaderProgramInfo::GetStaticClass()) {
            PLU_INFO("Shader Load Initiated!");
            TUsePointer<ShaderProgramInfo> shaderProgramInfo = mAssetManager->GetAssetData(assetDescriptor);
            EngineObjectHandle shaderHandle = mObjectManager->CreateObject<ShaderProgram>();
            TOwningPointer<ShaderProgram> shaderProgram =  mObjectManager->GetObjectAsOwner<ShaderProgram>(shaderHandle);
            shaderProgram->Uuid = shaderProgramInfo->Uuid;
            TUsePointer<IShaderCode> vertexShaderCode = GetShaderCode(shaderProgramInfo->VertexShaderUuid);
            TUsePointer<IShaderCode> fragmentShaderCode = GetShaderCode(shaderProgramInfo->FragmentShaderUuid);
            if (!vertexShaderCode || !fragmentShaderCode) {
                PLU_ERROR("Shader Load Error! No vertex or fragment shader found!");
                return;
            }
            shaderProgram->SetVertexShader(vertexShaderCode);
            shaderProgram->SetFragmentShader(fragmentShaderCode);
            mShaderPrograms[shaderProgram->Uuid] = shaderProgram;
            if (!shaderProgram->BinaryExists()) {
                if (shaderProgram->Recompile()) {
                    shaderProgram->UnloadProgram();
                }
            }
        } else if (assetDescriptor->AssetType == MaterialInfo::GetStaticClass()) {
            PLU_INFO("Material Load Initiated!");
            TUsePointer<MaterialInfo> materialInfo = mAssetManager->GetAssetData(assetDescriptor);
            if (!mShaderPrograms.Contains(materialInfo->shaderProgram)) {
                //This tries to load program. This can fail so that why there an extra test after
                GetShaderProgram(materialInfo->shaderProgram);
                if (!mShaderPrograms.Contains(materialInfo->shaderProgram)) {
                    return;
                }
            }
            TUsePointer<ShaderProgram> shaderProgram = mShaderPrograms[materialInfo->shaderProgram];
            auto uniforms = *shaderProgram->GetFragmentShader()->GetCodeUniforms();
            uniforms.Append(*shaderProgram->GetVertexShader()->GetCodeUniforms());
            for (auto uniform : uniforms) {
                TOwningPointer<IShaderUniform>* found =  materialInfo->MaterialParameters.FindIf([uniform](const TOwningPointer<IShaderUniform>& property)->bool {
                    if (!property) return false;
                    if (uniform->Name == property->Name && uniform->Type == property->Type) {
                        return true;
                    }
                    return false;
                });
                if (found != materialInfo->MaterialParameters.End()) continue;
                materialInfo->MaterialParameters.PushBack(uniform);
            }
            shaderProgram->GetVertexShader()->RenewUniforms();
            shaderProgram->GetFragmentShader()->RenewUniforms();
        }
    });
}

void Plu::RuntimeShaderManager::ShaderCodeScan()
{
    Path pathToSelf = GetExePath().GetParentPath().ToString().ToNarrow();
    pathToSelf += "/Shaders.txt";
    std::ifstream ifs(pathToSelf.CStr());
    std::string line;
    while (std::getline(ifs, line)) {
        TOwningPointer<RuntimeShaderCode> newShaderCode = CreateOwning<RuntimeShaderCode>(line.c_str());
        mShaderCodes.Insert(newShaderCode->Uuid, newShaderCode);
        PLU_TRACE("New Shader Code Registered! UUID {}", newShaderCode->Uuid.getUUID());
    }
}

bool Plu::RuntimeShaderManager::ShaderCodeExists(PluUUID uuid)
{
    return mShaderCodes.Contains(uuid);
}

DynamicArray<Plu::TUsePointer<Plu::ShaderProgram>> * Plu::RuntimeShaderManager::GetRenderableShaderPrograms()
{
    return &mShaderProgramsUsers;
}

Plu::TUsePointer<Plu::IShaderCode> Plu::RuntimeShaderManager::GetShaderCode(PluUUID uuid)
{
    if (ShaderCodeExists(uuid)) {
        return mShaderCodes[uuid];
    }
    return nullptr;
}

Plu::TUsePointer<Plu::ShaderProgram> Plu::RuntimeShaderManager::GetShaderProgram(PluUUID uuid)
{
    mAssetManager->GetAssetData(uuid);
    if (mShaderPrograms.Contains(uuid)) {
        return mShaderPrograms[uuid];
    }
    return nullptr;
}

void Plu::RuntimeShaderManager::LoadShader(PluUUID uuid)
{
    TUsePointer<ShaderProgram> program = GetShaderProgram(uuid);
    program->LoadFromBinary();
    if (program->IsLoaded()) {
        mShaderProgramsUsers.PushBack(program);
    }
}
