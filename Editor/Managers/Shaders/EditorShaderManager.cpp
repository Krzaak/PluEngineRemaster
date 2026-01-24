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
#include "PluEngine/PluPaths.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Shaders/ShaderProgram.h"

extern Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;
extern Plu::EditorAppContext* gEditorAppContext;

Plu::EditorShaderManager::EditorShaderManager()
{
}

Plu::EditorShaderManager::~EditorShaderManager()
{
}

void Plu::EditorShaderManager::PreInit(TUsePointer<EditorProjectManager> editorProjectManager)
{
	mProjectManager = editorProjectManager;
	gEditorAppContext->EditorAssetManager->GetObjectEventDispatcher()->Subscribe("NewAsset", [this](void* data) {
		PathW* path = static_cast<PathW *>(data);
		TUsePointer<IEditorAssetObject> asset = gEditorAppContext->EditorAssetManager->GetAssetByPath(*path);
		TUsePointer<EditorAssetObject<ShaderProgramInfo>> shaderAsset = DynamicCast<EditorAssetObject<ShaderProgramInfo>>(asset);
		if (!shaderAsset) return;
		PLU_INFO("Shader OK!");
		PluUUID vertexShaderUUID = shaderAsset->AssetInfo.VertexShaderUuid;
		PluUUID fragmentShaderUUID = shaderAsset->AssetInfo.FragmentShaderUuid;
		TOwningPointer<ShaderProgram> shaderProgram = gEngineObjectManager->CreateObject(ShaderProgram::GetStaticClass());
		shaderProgram->Uuid = shaderAsset->AssetInfo.Uuid;
		shaderProgram->SetFragmentShader(GetShaderCode(fragmentShaderUUID));
		shaderProgram->SetVertexShader(GetShaderCode(vertexShaderUUID));
		mShaderPrograms[shaderProgram->Uuid] = shaderProgram;
	});
}

void Plu::EditorShaderManager::ShaderCodeScan()
{
	std::optional<nlohmann::json> json = DiskManager::LoadJson(mProjectManager->GetProjectShadersDirectory().ToString() + L"/ShaderCodeUuids.json");

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
				if (json.value().contains(newShaderCode->GetPath().GetStem().ToNarrow().CStr())) {
					mShaderCodes.Insert(json.value()[newShaderCode->GetPath().GetStem().ToNarrow().CStr()], newShaderCode);
				} else {
					PluUUID newUuid = PluUUID();
					mShaderCodes.Insert(newUuid, newShaderCode);
					json.value()[newShaderCode->GetPath().GetStem().ToNarrow().CStr()] = newUuid.getUUID();
					DiskManager::SaveJson(mProjectManager->GetProjectShadersDirectory().ToString() + L"/ShaderCodeUuids.json", json.value());
				}
			} else {
				PluUUID newUuid = PluUUID();
				mShaderCodes.Insert(newUuid, newShaderCode);
				json = nlohmann::json();
				json.value()[newShaderCode->GetPath().GetStem().ToNarrow().CStr()] = newUuid.getUUID();
				DiskManager::SaveJson(mProjectManager->GetProjectShadersDirectory().ToString() + L"/ShaderCodeUuids.json", json.value());
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
