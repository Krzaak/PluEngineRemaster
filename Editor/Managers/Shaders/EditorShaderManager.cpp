//
// Created by Plutex on 1/20/26.
//

#include "EditorShaderManager.h"

#include <filesystem>

#include "adl_serializer.hpp"
#include "EditorAppContext.h"
#include "EditorShaderCode.h"
#include "Managers/Assets/EditorAssetManager.h"
#include "Managers/Assets/EditorAssetObject.h"
#include "Managers/Project/EditorProjectManager.h"
#include "Managers/Python/EditorPythonManager.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Shaders/ShaderProgram.h"

extern Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;
extern Plu::EditorAppContext* gEditorAppContext;

Plu::PathW Plu::EditorShaderWriter::GetShaderCacheDirectory()
{
	return gEditorAppContext->EditorProjectManager->GetProjectCacheDirectory();
}

Plu::EditorShaderManager::EditorShaderManager()
{
}

Plu::EditorShaderManager::~EditorShaderManager()
{
}

void Plu::EditorShaderManager::PreInit(TUsePointer<EditorProjectManager> editorProjectManager)
{
	SetGlobalShaderCacheWriter(gEngineObjectManager->CreateObject(EditorShaderWriter::GetStaticClass()));
	mProjectManager = editorProjectManager;
	static bool loaded = false;
	static DynamicArray<TUsePointer<EditorAssetObject<MaterialInfo>>> materialsToLoad;
	gEditorAppContext->EditorAssetManager->GetObjectEventDispatcher()->Subscribe("NewAsset", [this](void* data) {
		PathW* path = static_cast<PathW *>(data);
		TUsePointer<IEditorAssetObject> asset = gEditorAppContext->EditorAssetManager->GetAssetByPath(*path);
		if (asset->GetAssetType() == ShaderProgramInfo::GetStaticClass()->TypeName) {
			TUsePointer<EditorAssetObject<ShaderProgramInfo>> shaderAsset = DynamicCast<EditorAssetObject<ShaderProgramInfo>>(asset);
			if (!shaderAsset) return;
			PLU_INFO("Shader OK!");
			PluUUID vertexShaderUUID = shaderAsset->AssetInfo->VertexShaderUuid;
			PluUUID fragmentShaderUUID = shaderAsset->AssetInfo->FragmentShaderUuid;
			TOwningPointer<ShaderProgram> shaderProgram = gEngineObjectManager->CreateObject(ShaderProgram::GetStaticClass());
			shaderProgram->Uuid = shaderAsset->AssetInfo->Uuid;
			TUsePointer<IShaderCode> vertexShader = GetShaderCode(vertexShaderUUID);
			TUsePointer<IShaderCode> fragmentShader = GetShaderCode(fragmentShaderUUID);
			if (!fragmentShader || !vertexShader) {
				PLU_ERROR("Loading shader error: There is no Vertex or Fragment Shader!");
				return;
			}
			shaderProgram->SetFragmentShader(fragmentShader);
			shaderProgram->SetVertexShader(vertexShader);
			mShaderPrograms[shaderProgram->Uuid] = shaderProgram;
			if (!shaderProgram->BinaryExists()) {
				if (shaderProgram->Recompile()) {
					shaderProgram->UnloadProgram();
				}
			}
		} else if (asset->GetAssetType() == MaterialInfo::GetStaticClass()->TypeName) {
			TUsePointer material = DynamicCast<EditorAssetObject<MaterialInfo>>(asset);
			if (!material) return;
			if (!loaded) {
				materialsToLoad.PushBack(material);
				return;
			}
			if (mShaderPrograms.Contains(material->AssetInfo->shaderProgram)) {
				auto uniforms = *GetShaderProgram(material->AssetInfo->shaderProgram)->GetFragmentShader()->GetCodeUniforms();
				uniforms.Append(*GetShaderProgram(material->AssetInfo->shaderProgram)->GetVertexShader()->GetCodeUniforms());
				for (auto uniform : uniforms) {
					TOwningPointer<IShaderUniform>* found =  material->AssetInfo->MaterialParameters.FindIf([uniform](const TOwningPointer<IShaderUniform>& property)->bool {
						if (uniform->Name == property->Name && uniform->Type == property->Type) {
							return true;
						}
						return false;
					});
					if (found) continue;
					material->AssetInfo->MaterialParameters.PushBack(uniform);
				}
				GetShaderProgram(material->AssetInfo->shaderProgram)->GetVertexShader()->RenewUniforms();
				GetShaderProgram(material->AssetInfo->shaderProgram)->GetFragmentShader()->RenewUniforms();
			}
			for (auto prop : material->AssetInfo->MaterialParameters) {
				PLU_INFO("{}", prop->Name.CStr());
			}
		}
	});
	gEditorAppContext->EditorAssetManager->GetObjectEventDispatcher()->Subscribe("LoadedAssets", [this](void* data) {
		for (TUsePointer<EditorAssetObject<MaterialInfo>>& material: materialsToLoad) {
			if (mShaderPrograms.Contains(material->AssetInfo->shaderProgram)) {
				auto uniforms = *GetShaderProgram(material->AssetInfo->shaderProgram)->GetFragmentShader()->GetCodeUniforms();
				uniforms.Append(*GetShaderProgram(material->AssetInfo->shaderProgram)->GetVertexShader()->GetCodeUniforms());
				for (auto uniform : uniforms) {
					TOwningPointer<IShaderUniform>* found =  material->AssetInfo->MaterialParameters.FindIf([uniform](const TOwningPointer<IShaderUniform>& property)->bool {
						if (!property) return false;
						if (uniform->Name == property->Name && uniform->Type == property->Type) {
							return true;
						}
						return false;
					});
					if (found != material->AssetInfo->MaterialParameters.End()) continue;
					material->AssetInfo->MaterialParameters.PushBack(uniform);
				}
				GetShaderProgram(material->AssetInfo->shaderProgram)->GetVertexShader()->RenewUniforms();
				GetShaderProgram(material->AssetInfo->shaderProgram)->GetFragmentShader()->RenewUniforms();
			}
			for (auto prop : material->AssetInfo->MaterialParameters) {
				PLU_INFO("{}", prop->Name.CStr());
			}
		}
		loaded = true;
		materialsToLoad.Clear();
	});
}

void Plu::EditorShaderManager::ShaderCodeScan()
{
	std::optional<nlohmann::json> json = DiskManager::LoadJson(mProjectManager->GetProjectCacheDirectory().ToString() + L"/ShaderCodeUuids.json");

	gEditorAppContext->EditorPythonManager->RunScript(
		StringW::FromNarrow(String(PLU_PYTHON_TOOLS_DIR) + String("ShaderCodeParser.py")),
		std::filesystem::current_path().wstring().c_str(),
		"--project " + gEditorAppContext->EditorProjectManager->GetProjectDirectory().ToString().ToNarrow()
	);

	PathW scanDir = mProjectManager->GetProjectShadersDirectory();
#ifdef PLU_PLATFORM_WINDOWS
	for (auto entry : std::filesystem::recursive_directory_iterator(scanDir.CStr())) {
#else
	for (const auto& entry : std::filesystem::recursive_directory_iterator(scanDir.ToString().ToNarrow().CStr())) {
#endif
		if (entry.is_regular_file() && (entry.path().extension() == PLU_SHADER_FRAG_EXT || entry.path().extension() == PLU_SHADER_VERT_EXT)) {
			TOwningPointer<EditorShaderCode> newShaderCode = gEngineObjectManager->CreateObject(EditorShaderCode::GetStaticClass());
			newShaderCode->Init(entry.path().wstring().c_str());
			if (json.has_value()) {
				if (json.value().contains(newShaderCode->Name.CStr())) {
					UInt64 uuid = json.value()[newShaderCode->Name.CStr()].get<UInt64>();
					newShaderCode->Uuid = uuid;
					mShaderCodes.Insert(uuid, newShaderCode);
				} else {
					PluUUID newUuid = PluUUID();
					mShaderCodes.Insert(newUuid, newShaderCode);
					newShaderCode->Uuid = newUuid;
					json.value()[newShaderCode->Name.CStr()] = newUuid.getUUID();
					DiskManager::SaveJson(mProjectManager->GetProjectCacheDirectory().ToString() + L"/ShaderCodeUuids.json", json.value());
				}
			} else {
				PluUUID newUuid = PluUUID();
				mShaderCodes.Insert(newUuid, newShaderCode);
				newShaderCode->Uuid = newUuid;
				json = nlohmann::json();
				json.value()[newShaderCode->Name.CStr()] = newUuid.getUUID();
				DiskManager::SaveJson(mProjectManager->GetProjectCacheDirectory().ToString() + L"/ShaderCodeUuids.json", json.value());
			}
		}
	}
}

void Plu::EditorShaderManager::InitShaders()
{
}

Plu::TUsePointer<Plu::IShaderCode> Plu::EditorShaderManager::GetShaderCode(PluUUID uuid)
{
	return mShaderCodes[uuid];
}

Plu::TUsePointer<Plu::ShaderProgram> Plu::EditorShaderManager::GetShaderProgram(PluUUID uuid)
{
	return mShaderPrograms[uuid];
}

DynamicArray<Plu::TUsePointer<Plu::ShaderProgram>> * Plu::EditorShaderManager::GetRenderableShaderPrograms()
{
	return &mInitializedShaderPrograms;
}

void Plu::EditorShaderManager::LoadShader(PluUUID uuid)
{
	TUsePointer<ShaderProgram> program = GetShaderProgram(uuid);
	program->LoadFromBinary();
	if (program->IsLoaded()) {
		mInitializedShaderPrograms.PushBack(program);
	}
}
