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
    // mShaderPrograms (render-owned TOwningPointer<ShaderProgram>) jest opróżniane na wątku renderu
    // w ReleaseRenderResources() (wołane z RenderingManager::RenderThreadExit przed ReleaseGLContext),
    // więc tutaj nie ma już owning-refów do zwolnienia — destruktor managera na main threadzie nie
    // odpala asertu thread-affinity TOwningPointera. Gdyby ReleaseRenderResources nie zostało wywołane
    // (np. ścieżka bez wątku renderu), Clear poniżej i tak zwolniłby je tu — ShaderProgram::~ nie dotyka GL.
    mShaderProgramsUsers.Clear();
    mShaderPrograms.Clear();
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
        // ShaderProgramInfo: nic tu nie tworzymy ani nie kompilujemy. Sam ShaderProgram (zasób GL)
        // jest tworzony i kompilowany leniwie na render threadzie (GetShaderProgram + LoadShader).
        // Wystarczy, że dane assetu zostaną załadowane na main threadzie (robi to gałąź materiału
        // przez GetAssetData), aby render thread mógł je odczytać przez GetAssetDataNoLoad.
        if (assetDescriptor->AssetType == MaterialInfo::GetStaticClass()) {
            PLU_INFO("Material Load Initiated!");
            TUsePointer<MaterialInfo> materialInfo = mAssetManager->GetAssetData(assetDescriptor);
            if (!materialInfo) return;
            // Ładujemy/cache'ujemy dane ShaderProgramInfo na main threadzie. Render thread odczyta je
            // potem przez GetAssetDataNoLoad i leniwie skompiluje shader.
            TUsePointer<ShaderProgramInfo> shaderProgramInfo = mAssetManager->GetAssetData(materialInfo->shaderProgram);
            if (!shaderProgramInfo) return;
            // Uniformy materiału pochodzą z kodu shaderów (IShaderCode, main-owned, czysto CPU) —
            // nie z obiektu ShaderProgram (ten żyje wyłącznie na render threadzie).
            TUsePointer<IShaderCode> vertexShaderCode = GetShaderCode(shaderProgramInfo->VertexShaderUuid);
            TUsePointer<IShaderCode> fragmentShaderCode = GetShaderCode(shaderProgramInfo->FragmentShaderUuid);
            if (!vertexShaderCode || !fragmentShaderCode) {
                PLU_ERROR("Material Load Error! No vertex or fragment shader code found!");
                return;
            }
            auto uniforms = *fragmentShaderCode->GetCodeUniforms();
            uniforms.Append(*vertexShaderCode->GetCodeUniforms());
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
            vertexShaderCode->RenewUniforms();
            fragmentShaderCode->RenewUniforms();
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
    // Render-thread get-or-create. ShaderProgram (zasób GL) jest tworzony i ownowany przez render
    // thread; ta metoda NIE robi GL (kompilacja jest w LoadShader). Wołana z render threadu
    // (Renderer, fizyczne renderery).
    if (mShaderPrograms.Contains(uuid)) {
        return mShaderPrograms[uuid];
    }
    // GetAssetDataNoLoad jest render-safe (zwraca cache, brak asertu main-thread). Dane SP info są
    // ładowane na main threadzie przy ładowaniu materiału; jeśli jeszcze ich nie ma, spróbujemy
    // ponownie w kolejnej klatce (jak meshe/tekstury).
    TUsePointer<ShaderProgramInfo> shaderProgramInfo = mAssetManager->GetAssetDataNoLoad(uuid);
    if (!shaderProgramInfo) {
        // Engine built-in shaders (e.g. OnlyPositionShader for shadows, DebugLine) aren't
        // referenced by any material, so their data is never warmed on the main thread by the
        // material-load event. Post a deferred load request; it'll be in cache next frame.
        mAssetManager->RequestAssetDataLoad(uuid);
        return nullptr;
    }
    TUsePointer<IShaderCode> vertexShaderCode = GetShaderCode(shaderProgramInfo->VertexShaderUuid);
    TUsePointer<IShaderCode> fragmentShaderCode = GetShaderCode(shaderProgramInfo->FragmentShaderUuid);
    if (!vertexShaderCode || !fragmentShaderCode) {
        PLU_ERROR("Shader Program {} has no vertex or fragment shader code!", uuid.getUUID());
        return nullptr;
    }
    EngineObjectHandle shaderHandle = mObjectManager->CreateObject<ShaderProgram>();
    TOwningPointer<ShaderProgram> shaderProgram = mObjectManager->GetObjectAsOwner<ShaderProgram>(shaderHandle);
    shaderProgram->Uuid = shaderProgramInfo->Uuid;
    shaderProgram->SetVertexShader(vertexShaderCode);
    shaderProgram->SetFragmentShader(fragmentShaderCode);
    mShaderPrograms[uuid] = shaderProgram;
    return mShaderPrograms[uuid];
}

void Plu::RuntimeShaderManager::LoadShader(PluUUID uuid)
{
    // Render-thread: faktyczna kompilacja / wczytanie z cache binarnego (GL). Wołane na render
    // threadzie (Renderer leniwie, fizyczne renderery).
    TUsePointer<ShaderProgram> program = GetShaderProgram(uuid);
    if (!program) return;
    if (program->IsLoaded()) return;
    program->LoadFromBinary();
    if (program->IsLoaded()) {
        mShaderProgramsUsers.PushBack(program);
    }
}

void Plu::RuntimeShaderManager::ReleaseRenderResources()
{
    // Render-thread cleanup. mShaderPrograms holds render-owned TOwningPointer<ShaderProgram>
    // (created on the render thread in GetShaderProgram). Releasing them HERE — on the owning
    // thread, before the GL context is released — avoids the TOwningPointer thread-affinity
    // assert that would otherwise fire when the manager is later destroyed on the main thread.
    // UnloadProgram (glDeleteProgram) also needs the live GL context on the render thread.
    mShaderProgramsUsers.Clear(); // non-owning observers; drop before destroying owners
    for (auto& entry : mShaderPrograms) {
        TOwningPointer<ShaderProgram>& program = entry.second;
        if (!program) continue;
        mObjectManager->DestroyObject(program->GetObjectHandle());
        program->UnloadProgram();
    }
    mShaderPrograms.Clear(); // final owning Release runs on the render thread → no assert
}
