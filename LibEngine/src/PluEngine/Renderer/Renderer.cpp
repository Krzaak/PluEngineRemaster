//
// Created by Plutex on 6/22/26.
//

#include "PluEngine/Renderer/Renderer.h"

#include "PluEngine/Application.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Renderer/RenderThreading.h"
#include "PluEngine/Renderer/RenderUtils.h"
#include "PluEngine/Shaders/ShaderProgram.h"
#include "PluEngine/Window/Window.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/Managers/RenderingManager.h"
#include "PluEngine/Managers/ShadersManager.h"
#include "EngineAssets.h"
#include "PluEngine/Timer.h"

void Plu::Renderer::Initialize(ApplicationInfo *applicationInfo)
{
    mApplicationInfo = applicationInfo;
    EngineObjectHandle hdl = mApplicationInfo->AppObjectManager->CreateObject<FrameBuffer>();
    mMainBuffer = mApplicationInfo->AppObjectManager->GetObjectAsOwner<FrameBuffer>(hdl);
    TUsePointer<IWindow> window = mApplicationInfo->AppWindow;
    mMainBuffer->Create(window->GetWidth(), window->GetHeight(), mApplicationInfo->AppObjectManager, FrameBufferType::ColorDepth);

    // Mapy cieni kaskad tworzone eager — kontekst GL jest tu na wątku renderu, więc na
    // ścieżce klatki nie ma już per-klatkowego CreateObject (FBO o stałym rozmiarze).
    constexpr Int32 kShadowMapResolution = 4096;
    for (int c = 0; c < kCascadeCount; c++) {
        EngineObjectHandle hdl = mApplicationInfo->AppObjectManager->CreateObject<FrameBuffer>();
        TOwningPointer<FrameBuffer> fb = mApplicationInfo->AppObjectManager->GetObjectAsOwner<FrameBuffer>(hdl);
        fb->Create(kShadowMapResolution, kShadowMapResolution, mApplicationInfo->AppObjectManager, FrameBufferType::DepthOnly);
        mCascadeFrameBuffers.PushBack(fb);
    }
}

DynamicArray<Plu::ShadowCascadeData> Plu::Renderer::RenderShadowPass(Plu::RenderSnapshot *snapshot, const Matrix4& cameraView)
{
    PLU_PROFILE_SCOPE("Renderer::RenderShadowPass");
    DynamicArray<ShadowCascadeData> cascades;

    if (!snapshot->HasDirLight || mCascadeFrameBuffers.IsEmpty()) return cascades;

    // Shader głębi (tylko pozycja) dla map cieni — leniwa kompilacja na wątku renderu.
    TUsePointer<ShaderProgram> depthShader = mApplicationInfo->AppShaderManager->GetShaderProgram(EngineAssets::OnlyPositionShader);
    if (!depthShader) return cascades;
    if (!depthShader->IsLoaded()) {
        mApplicationInfo->AppShaderManager->LoadShader(EngineAssets::OnlyPositionShader);
        return cascades; // gotowe w kolejnej klatce
    }

    TUsePointer<IWindow> window = mApplicationInfo->AppWindow;
    const float aspect = static_cast<float>(window->GetWidth()) / static_cast<float>(window->GetHeight());
    const float fovRad = glm::radians(snapshot->CameraFOV);

    // Wyższa lambda (~0.9) zagęszcza pierwsze kaskady przy kamerze — ostrzejsze cienie blisko.
    DynamicArray<float> splits = GetCascadeSplits(kCascadeCount, kCameraNearClip, kShadowFarClip, 0.9f);
    cascades = GetCascadedLightMatrices(
        cameraView, fovRad, aspect,
        kCameraNearClip, kShadowFarClip,
        snapshot->DirLight.Direction,
        splits,
        static_cast<float>(mCascadeFrameBuffers[0]->GetWidth())
    );

    const UInt64 staticMeshCount = snapshot->StaticMeshRenderObjects.Size();
    for (int c = 0; c < kCascadeCount; c++) {
        depthShader->SetMatrix4Uniform("lightSpaceMatrix", cascades[c].viewProj);
        mCascadeFrameBuffers[c]->Clear(0.0f, 0.0f, 0.0f, 1.0f);
        mCascadeFrameBuffers[c]->Bind(); // ustawia glViewport na rozmiar mapy cienia

        for (UInt32 i = 0; i < staticMeshCount; i++) {
            StaticMeshRenderObject* renderObject = &snapshot->StaticMeshRenderObjects[i];
            if (!renderObject->CastsShadow) continue;
            TUsePointer<StaticMesh> staticMesh = mApplicationInfo->AppAssetManager->GetAssetDataNoLoad(renderObject->MeshUUID);
            if (!staticMesh || !staticMesh->IsLoaded) continue;
            depthShader->SetMatrix4Uniform("model", renderObject->ModelMatrix);
            DrawStaticMesh(staticMesh.GetRaw(), mApplicationInfo->AppRenderingManager.GetRaw());
        }

        mCascadeFrameBuffers[c]->Unbind();
    }

    return cascades;
}

void Plu::Renderer::RenderSnapshot(Plu::RenderSnapshot *snapshot)
{
    PLU_PROFILE_SCOPE("Renderer::RenderSnapshot");

    const Matrix4 view = glm::inverse(
        glm::translate(glm::mat4(1.0f), snapshot->CameraLocation) *
        glm::mat4_cast(glm::quat(glm::radians(snapshot->CameraRotation)))
    );

    // Pass 1: mapy głębi kaskad dla światła kierunkowego.
    DynamicArray<ShadowCascadeData> cascades = RenderShadowPass(snapshot, view);
    const bool hasShadows = !cascades.IsEmpty();

    // Pass 2: scena do głównego bufora.
    mMainBuffer->Clear();
    mMainBuffer->Bind();

    // Uniformy globalne (kamera, światło, kaskady, sloty teksturujące) ustawiane raz na klatkę
    // na liście aktywnych shaderów prowadzonej przez ShadersManager — Renderer nie trzyma już
    // własnej listy. Mapy cieni kaskad zajmują pierwsze kCascadeCount slotów (SetSlotsUsed PRZED
    // RenderFromMaterial, które startuje tekstury materiału za nimi). Uniformy nieobecne w danym
    // shaderze są no-opem (location == -1), więc ustawianie ich na wszystkich programach jest bezpieczne.
    DynamicArray<TUsePointer<ShaderProgram>>* activePrograms = mApplicationInfo->AppShaderManager->GetRenderableShaderPrograms();
    const UInt32 programCount = activePrograms ? activePrograms->Size() : 0;
    for (UInt32 p = 0; p < programCount; p++) {
        ShaderProgram* program = activePrograms->At(p).GetRaw();
        if (!program || !program->IsLoaded()) continue;

        program->SetMatrix4Uniform("view", view);
        program->SetMatrix4Uniform("projection", snapshot->CameraProjectionMatrix);
        program->SetVec3Uniform("cameraPos", snapshot->CameraLocation);

        if (snapshot->HasDirLight) {
            program->SetVec3Uniform("dirLightDir", snapshot->DirLight.Direction);
            program->SetVec4Uniform("dirLightColor", Vec4(snapshot->DirLight.Color, snapshot->DirLight.Intensity));
        }

        program->SetSlotsUsed(hasShadows ? kCascadeCount : 0);
        if (hasShadows) {
            for (int c = 0; c < kCascadeCount; c++) {
                String ci = String::FromInt(c);
                String matName = "cascadeLightSpaceMatrices["; matName += ci; matName += "]";
                String texName = "cascadeShadowMaps[";         texName += ci; texName += "]";
                String sptName = "cascadeSplitDistances[";     sptName += ci; sptName += "]";
                program->SetMatrix4Uniform(matName, cascades[c].viewProj);
                program->SetTextureUniform(texName, mCascadeFrameBuffers[c]->GetDepthTexture(), c);
                program->SetFloatUniform(sptName, cascades[c].splitDistance);
            }
            program->SetIntUniform("cascadeCount", kCascadeCount);
        }
    }

    UInt64 staticMeshCount = snapshot->StaticMeshRenderObjects.Size();
    for (UInt32 i = 0; i < staticMeshCount; i++) {
        StaticMeshRenderObject* renderObject = &snapshot->StaticMeshRenderObjects[i];
        TUsePointer<StaticMesh> staticMesh = mApplicationInfo->AppAssetManager->GetAssetDataNoLoad(renderObject->MeshUUID);
        TUsePointer<MaterialInfo> materialInfo = mApplicationInfo->AppAssetManager->GetAssetDataNoLoad(renderObject->MaterialUUID);
        if (!materialInfo || !staticMesh) continue;
        if (!staticMesh->IsLoaded) {
            mApplicationInfo->AppRenderingManager->RequestStaticMeshLoad(renderObject->MeshUUID);
        }
        TUsePointer<ShaderProgram> shaderProgram = mApplicationInfo->AppShaderManager->GetShaderProgram(materialInfo->shaderProgram);
        if (!shaderProgram || !shaderProgram->IsLoaded()) {
            // Leniwa kompilacja na render threadzie (analogicznie do RequestStaticMeshLoad dla meshy);
            // LoadShader rejestruje program w liście aktywnych ShadersManagera, więc w następnej
            // klatce dostanie uniformy globalne powyżej. Tej klatki mesh jest pomijany.
            mApplicationInfo->AppShaderManager->LoadShader(materialInfo->shaderProgram);
            continue;
        }

        // Per-mesh: tylko materiał (tekstury od slotu kCascadeCount) + model + rysowanie.
        shaderProgram->RenderFromMaterial(materialInfo.GetRaw(), mApplicationInfo->AppRenderingManager);
        shaderProgram->SetMatrix4Uniform("model", renderObject->ModelMatrix);
        DrawStaticMesh(staticMesh.GetRaw(), mApplicationInfo->AppRenderingManager.GetRaw());
    }

    mMainBuffer->Unbind();

    TUsePointer<IWindow> window = mApplicationInfo->AppWindow;
    mMainBuffer->BlitToScreen(window->GetWidth(), window->GetHeight());
}

void Plu::Renderer::Shutdown()
{
    for (UInt32 c = 0; c < mCascadeFrameBuffers.Size(); c++) {
        if (!mCascadeFrameBuffers[c]) continue;
        mApplicationInfo->AppObjectManager->DestroyObject(mCascadeFrameBuffers[c]->GetObjectHandle());
        mCascadeFrameBuffers[c]->Destroy();
        mCascadeFrameBuffers[c] = nullptr;
    }
    mCascadeFrameBuffers.Clear();

    mApplicationInfo->AppObjectManager->DestroyObject(mMainBuffer->GetObjectHandle());
    mMainBuffer->Destroy();
    mMainBuffer = nullptr;
}
