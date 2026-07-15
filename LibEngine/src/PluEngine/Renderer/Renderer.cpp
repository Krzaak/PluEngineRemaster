//
// Created by Plutex on 6/22/26.
//

#include "PluEngine/Renderer/Renderer.h"

#include <glad/glad.h>
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
#include "PluEngine/AssetTypes/SkeletalMesh/SkeletalMesh.h"

Plu::TUsePointer<Plu::FrameBuffer> Plu::Renderer::GetMainFrameBuffer()
{
    return mMainBuffer;
}

void Plu::Renderer::Initialize(ApplicationInfo *applicationInfo)
{
    mApplicationInfo = applicationInfo;
    EngineObjectHandle hdl = mApplicationInfo->AppObjectManager->CreateObject<FrameBuffer>();
    mMainBuffer = mApplicationInfo->AppObjectManager->GetObjectAsOwner<FrameBuffer>(hdl);
    TUsePointer<IWindow> window = mApplicationInfo->AppWindow;
    mMainBuffer->Create(window->GetWidth(), window->GetHeight(), mApplicationInfo->AppObjectManager, FrameBufferType::ColorDepth);

    // Mapy cieni kaskad tworzone eager — kontekst GL jest tu na wątku renderu, więc na
    // ścieżce klatki nie ma już per-klatkowego CreateObject (FBO o stałym rozmiarze).
    // Rozdzielczość per kaskada (kCascadeResolutions) + 16-bitowa głębia: ortho-projekcje
    // kaskad mają liniową głębię i ciasny zakres z, więc D16 wystarcza, a VRAM spada o połowę.
    for (int c = 0; c < kCascadeCount; c++) {
        EngineObjectHandle hdl = mApplicationInfo->AppObjectManager->CreateObject<FrameBuffer>();
        TOwningPointer<FrameBuffer> fb = mApplicationInfo->AppObjectManager->GetObjectAsOwner<FrameBuffer>(hdl);
        fb->Create(kCascadeResolutions[c], kCascadeResolutions[c], mApplicationInfo->AppObjectManager, FrameBufferType::DepthOnly, /*Use16BitDepth=*/true);
        mCascadeFrameBuffers.PushBack(fb);
    }

    // VAO/VBO debugowej geometrii fizyki — kontekst GL jest tu na wątku renderu.
    // Layout per wierzchołek: pos(3) + color(3), stride 6 floatów.
    glGenVertexArrays(1, &mDebugVao);
    glGenBuffers(1, &mDebugVbo);
    glBindVertexArray(mDebugVao);
    glBindBuffer(GL_ARRAY_BUFFER, mDebugVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    mSkeletalMatricesBuffer.Create(100);
}

DynamicArray<Plu::ShadowCascadeData> Plu::Renderer::RenderShadowPass(Plu::RenderSnapshot *snapshot, const Matrix4& cameraView)
{
    PLU_PROFILE_SCOPE("Renderer::RenderShadowPass");
    DynamicArray<ShadowCascadeData> cascades;

    if (!snapshot->HasDirLight || mCascadeFrameBuffers.IsEmpty()) {
        // No directional light this frame: clear the cascade depth maps so leftover content
        // (e.g. shadows baked during a previous PIE session) doesn't linger after exiting PIE.
        for (UInt32 c = 0; c < mCascadeFrameBuffers.Size(); c++) {
            mCascadeFrameBuffers[c]->Clear(0.0f, 0.0f, 0.0f, 1.0f);
        }
        // Mapy głębi właśnie straciły zawartość — cache round-robina przestaje im odpowiadać.
        mCascadeCache.Clear();
        return cascades;
    }

    // Shader głębi (tylko pozycja) dla map cieni — leniwa kompilacja na wątku renderu.
    TUsePointer<ShaderProgram> depthShader = mApplicationInfo->AppShaderManager->GetShaderProgram(EngineAssets::OnlyPositionShader);
    if (!depthShader) return cascades;
    if (!depthShader->IsLoaded()) {
        mApplicationInfo->AppShaderManager->LoadShader(EngineAssets::OnlyPositionShader);
        return cascades; // gotowe w kolejnej klatce
    }

    // Skinowany wariant shadera głębi dla skeletal meshy — ładowany niezależnie: jeśli jeszcze nie
    // gotowy, pomijamy tylko cienie skeletalne (static-owe i tak lecą), a nie cały pass.
    TUsePointer<ShaderProgram> skeletalDepthShader = mApplicationInfo->AppShaderManager->GetShaderProgram(EngineAssets::OnlyPositionSkeletalShader);
    const bool skeletalDepthReady = skeletalDepthShader && skeletalDepthShader->IsLoaded();
    if (skeletalDepthShader && !skeletalDepthShader->IsLoaded()) {
        mApplicationInfo->AppShaderManager->LoadShader(EngineAssets::OnlyPositionSkeletalShader);
    }

    TUsePointer<IWindow> window = mApplicationInfo->AppWindow;
    const float aspect = static_cast<float>(window->GetWidth()) / static_cast<float>(window->GetHeight());
    const float fovRad = glm::radians(snapshot->CameraFOV);

    // Wyższa lambda zagęszcza pierwsze kaskady przy kamerze — ostrzejsze cienie blisko.
    // 5 kaskad + 0.99 daje splity ~1.1 / 3.6 / 14 / 62 / 300 m: pierwsza kaskada kończy się
    // ~1 m od kamery (teksel ~1 mm — ostre cienie małych obiektów tuż przed nosem), a układ
    // dalekich kaskad zmienia się kosmetycznie (62 m zamiast 66 m).
    DynamicArray<float> splits = GetCascadeSplits(kCascadeCount, kCameraNearClip, kShadowFarClip, 0.99f);
    DynamicArray<float> resolutions;
    resolutions.Reserve(kCascadeCount);
    for (int c = 0; c < kCascadeCount; c++) {
        resolutions.PushBack(static_cast<float>(mCascadeFrameBuffers[c]->GetWidth()));
    }
    cascades = GetCascadedLightMatrices(
        cameraView, fovRad, aspect,
        kCameraNearClip, kShadowFarClip,
        snapshot->DirLight.Direction,
        splits,
        resolutions
    );

    // Round-robin dalekich kaskad: kaskady od kFirstStaggeredCascade odświeżamy naprzemiennie
    // co drugą klatkę. Pominięta kaskada zachowuje starą mapę głębi, więc do samplowania w
    // głównym passie musi iść macierz z klatki, w której tę mapę wyrenderowano (mCascadeCache)
    // — świeża macierz podąża za kamerą i rozjechałaby się z zawartością mapy. Zmiana kierunku
    // światła unieważnia stare mapy w całości, wtedy renderujemy wszystkie kaskady.
    const Vec3 lightDir = glm::normalize(snapshot->DirLight.Direction);
    const bool cacheValid = mCascadeCache.Size() == static_cast<UInt32>(kCascadeCount)
                         && glm::dot(lightDir, mCascadeCacheLightDir) > 0.9999f;

    const UInt64 staticMeshCount   = snapshot->StaticMeshRenderObjects.Size();
    const UInt64 skeletalMeshCount = snapshot->SkeletalMeshRenderObjects.Size();

    // Front-face culling tylko na czas map cieni: do bufora głębi trafiają TYLNE ściany
    // obiektów, więc próg self-shadowingu (acne) przesuwa się na niewidoczną, odwróconą od
    // kamery stronę geometrii — najskuteczniejszy zabieg na acne płaskich/prostopadłych
    // powierzchni. Culling jest globalnie wyłączony (główny pass renderuje obie strony),
    // więc po passie przywracamy stan. Uwaga: dla otwartej/jednostronnej geometrii (pojedyncze
    // quady) może dać light-leak — wtedy normal-offset w Shadow.frag łagodzi przypadki brzegowe.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    // Slope-scaled depth bias po stronie CASTERA: liczony per-trójkąt w jednostkach precyzji
    // bufora głębi, więc nie skaluje się z rozmiarem teksela kaskady i nie przesuwa cienia
    // w bok (w przeciwieństwie do normal-offsetu w Shadow.frag). Dzięki temu ten sam bias
    // leczy acne dużych powierzchni w dalekich kaskadach, nie zjadając cieni małych obiektów.
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.5f, 4.0f);

    for (int c = 0; c < kCascadeCount; c++) {
        // Harmonogram round-robina: bliskie kaskady co klatkę; dalekie naprzemiennie
        // ((mShadowFrameIndex + c) % 2 — przy dwóch dalekich kaskadach renderuje się
        // dokładnie jedna per klatka). Bez ważnego cache'u wszystko musi iść od zera.
        const bool renderThisFrame = !cacheValid
                                  || c < kFirstStaggeredCascade
                                  || (mShadowFrameIndex + static_cast<UInt64>(c)) % 2 == 0;
        if (!renderThisFrame) {
            cascades[c] = mCascadeCache[c]; // mapa głębi jest stara — sampluj jej macierzą
            continue;
        }

        mCascadeFrameBuffers[c]->Clear(0.0f, 0.0f, 0.0f, 1.0f);
        mCascadeFrameBuffers[c]->Bind(); // ustawia glViewport na rozmiar mapy cienia

        // Static meshe — nieskinowany shader głębi.
        depthShader->SetMatrix4Uniform("lightSpaceMatrix", cascades[c].viewProj);
        for (UInt32 i = 0; i < staticMeshCount; i++) {
            StaticMeshRenderObject* renderObject = &snapshot->StaticMeshRenderObjects[i];
            if (!renderObject->CastsShadow) continue;
            TUsePointer<StaticMesh> staticMesh = mApplicationInfo->AppAssetManager->GetAssetDataNoLoad(renderObject->MeshUUID);
            if (!staticMesh || !staticMesh->IsLoaded) continue;
            depthShader->SetMatrix4Uniform("model", renderObject->ModelMatrix);
            DrawStaticMesh(staticMesh.GetRaw(), mApplicationInfo->AppRenderingManager.GetRaw());
        }

        // Skeletal meshe — skinowany shader głębi z tą samą paletą kości co główny pass,
        // dzięki czemu cień podąża za animacją. Paleta idzie przez SSBO (binding 0), jak w
        // BasicVertSkeletal.vert. Bufor jest współdzielony między obiektami, więc upload leci
        // per-obiekt (i per-kaskada, bo kolejny obiekt nadpisuje jego zawartość).
        if (skeletalDepthReady) {
            skeletalDepthShader->SetMatrix4Uniform("lightSpaceMatrix", cascades[c].viewProj);
            for (UInt32 i = 0; i < skeletalMeshCount; i++) {
                SkeletalMeshRenderObject* renderObject = &snapshot->SkeletalMeshRenderObjects[i];
                if (!renderObject->CastsShadow) continue;
                TUsePointer<SkeletalMesh> skeletalMesh = mApplicationInfo->AppAssetManager->GetAssetDataNoLoad(renderObject->MeshUUID);
                if (!skeletalMesh || !skeletalMesh->IsLoaded) continue;

                // skin = global * offset (identycznie jak w RenderSnapshot).
                DynamicArray<Matrix4> skeletalMatrices;
                skeletalMatrices.Reserve(renderObject->Bones.Size());
                for (auto bone : renderObject->Bones) {
                    skeletalMatrices.PushBack(bone.second * bone.first);
                }

                mSkeletalMatricesBuffer.BindBase(0);
                if (mSkeletalMatricesBuffer.GetCount() < skeletalMatrices.Size()) {
                    mSkeletalMatricesBuffer.SetData(skeletalMatrices);
                } else {
                    mSkeletalMatricesBuffer.Update(skeletalMatrices);
                }

                skeletalDepthShader->SetMatrix4Uniform("model", renderObject->ModelMatrix);
                DrawSkeletalMesh(skeletalMesh.GetRaw(), mApplicationInfo->AppRenderingManager.GetRaw());
                mSkeletalMatricesBuffer.Unbind();
            }
        }

        mCascadeFrameBuffers[c]->Unbind();
    }

    // Przywróć stan cullingu i polygon offsetu do domyślnego dla głównego passa.
    glDisable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.0f, 0.0f);
    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);

    // Zapamiętaj stan tej klatki dla round-robina: pominięte kaskady mają już w `cascades`
    // swoje stare macierze, więc cache po prostu odzwierciedla to, czym samplujemy mapy.
    mCascadeCache = cascades;
    mCascadeCacheLightDir = lightDir;
    mShadowFrameIndex++;

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
        if (!program) continue;
        // Hot reload: main-thread zgłosił zmianę źródła, tu (na render threadzie z kontekstem GL)
        // rekompilujemy. Recompile przy błędzie kompilacji zostawia stary program załadowany.
        if (program->ConsumeRecompileRequest()) {
            program->Recompile();
        }
        if (!program->IsLoaded()) continue;

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

    UInt64 skeletalMeshCount = snapshot->SkeletalMeshRenderObjects.Size();
    for (UInt32 i = 0; i < skeletalMeshCount; i++) {
        SkeletalMeshRenderObject* renderObject = &snapshot->SkeletalMeshRenderObjects[i];
        TUsePointer<SkeletalMesh> skeletalMesh = mApplicationInfo->AppAssetManager->GetAssetDataNoLoad(renderObject->MeshUUID);
        TUsePointer<MaterialInfo> materialInfo = mApplicationInfo->AppAssetManager->GetAssetDataNoLoad(renderObject->MaterialUUID);
        if (!materialInfo || !skeletalMesh) continue;
        if (!skeletalMesh->IsLoaded) {
            mApplicationInfo->AppRenderingManager->RequestSkeletalMeshLoad(renderObject->MeshUUID);
        }
        TUsePointer<ShaderProgram> shaderProgram = mApplicationInfo->AppShaderManager->GetShaderProgram(materialInfo->shaderProgram);
        if (!shaderProgram || !shaderProgram->IsLoaded()) {
            // Leniwa kompilacja na render threadzie (analogicznie do RequestStaticMeshLoad dla meshy);
            // LoadShader rejestruje program w liście aktywnych ShadersManagera, więc w następnej
            // klatce dostanie uniformy globalne powyżej. Tej klatki mesh jest pomijany.
            mApplicationInfo->AppShaderManager->LoadShader(materialInfo->shaderProgram);
            continue;
        }

        // Materiał na programie bez skinningu (brak bloku SSBO "BoneMatrices" w vertex shaderze)
        // rysuje skeletal mesh zamrożony w bind pose — po cichu. Ostrzegamy raz per program.
        if (!shaderProgram->HasBoneMatricesBlock() && !mWarnedNonSkeletalPrograms.Contains(materialInfo->shaderProgram.getUUID())) {
            mWarnedNonSkeletalPrograms.Insert(materialInfo->shaderProgram.getUUID());
            PLU_CORE_WARN("Skeletal mesh {} uses material {} whose shader program {} has no 'BoneMatrices' SSBO block — "
                          "no skinning, mesh will stay in bind pose. Use a program with a skeletal vertex shader (e.g. BasicVertSkeletal.vert).",
                          renderObject->MeshUUID.getUUID(), renderObject->MaterialUUID.getUUID(), materialInfo->shaderProgram.getUUID());
        }

        // Per-mesh: tylko materiał (tekstury od slotu kCascadeCount) + model + rysowanie.
        shaderProgram->RenderFromMaterial(materialInfo.GetRaw(), mApplicationInfo->AppRenderingManager);

        DynamicArray<Matrix4> skeletalMatrices;
        skeletalMatrices.Reserve(renderObject->Bones.Size());
        for (auto bone : renderObject->Bones) {
            // {offset, global}: skin = global * offset. The reverse also yields identity in
            // bind pose (offset == global⁻¹), so a swap here only breaks animated poses.
            skeletalMatrices.PushBack(bone.second * bone.first);
        }

        mSkeletalMatricesBuffer.BindBase(0);
        if (mSkeletalMatricesBuffer.GetCount() < skeletalMatrices.Size()) {
            mSkeletalMatricesBuffer.SetData(skeletalMatrices);
        } else {
            mSkeletalMatricesBuffer.Update(skeletalMatrices);
        }

        shaderProgram->SetMatrix4Uniform("model", renderObject->ModelMatrix);
        DrawSkeletalMesh(skeletalMesh.GetRaw(), mApplicationInfo->AppRenderingManager.GetRaw());
        mSkeletalMatricesBuffer.Unbind();
    }

#ifdef PLU_ENGINE_EDITOR_BUILD
    // Pass 3: debugowa geometria fizyki (linie + punkty) do tego samego bufora.
    RenderDebugGeometry(snapshot, snapshot->CameraProjectionMatrix * view);
#endif

    mMainBuffer->Unbind();
}

void Plu::Renderer::RenderDebugGeometry(Plu::RenderSnapshot *snapshot, const Matrix4 &viewProj)
{
    if (snapshot->DebugLineVerts.IsEmpty() && snapshot->DebugPointVerts.IsEmpty()) return;

    PLU_PROFILE_SCOPE("Renderer::RenderDebugGeometry");

    TUsePointer<ShaderProgram> shader = mApplicationInfo->AppShaderManager->GetShaderProgram(EngineAssets::DebugLine);
    if (!shader) return;
    if (!shader->IsLoaded()) {
        // Leniwa kompilacja na render threadzie (parytet z passem materiałów/cieni); rysowanie
        // pojawi się w kolejnej klatce, gdy shader będzie gotowy.
        mApplicationInfo->AppShaderManager->LoadShader(shader->Uuid);
        return;
    }

    shader->SetMatrix4Uniform("uViewProj", viewProj);

    glBindVertexArray(mDebugVao);
    glBindBuffer(GL_ARRAY_BUFFER, mDebugVbo);

    if (!snapshot->DebugLineVerts.IsEmpty()) {
        glBufferData(GL_ARRAY_BUFFER, snapshot->DebugLineVerts.Size() * sizeof(float),
                     snapshot->DebugLineVerts.Data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(snapshot->DebugLineVerts.Size() / 6));
    }

    if (!snapshot->DebugPointVerts.IsEmpty()) {
        glPointSize(snapshot->DebugPointSize);
        glBufferData(GL_ARRAY_BUFFER, snapshot->DebugPointVerts.Size() * sizeof(float),
                     snapshot->DebugPointVerts.Data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(snapshot->DebugPointVerts.Size() / 6));
        glPointSize(1.0f);
    }

    glBindVertexArray(0);
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
    mCascadeCache.Clear();

    if (mDebugVao) { glDeleteVertexArrays(1, &mDebugVao); mDebugVao = 0; }
    if (mDebugVbo) { glDeleteBuffers(1, &mDebugVbo); mDebugVbo = 0; }

    mApplicationInfo->AppObjectManager->DestroyObject(mMainBuffer->GetObjectHandle());
    mMainBuffer->Destroy();
    mMainBuffer = nullptr;
}
