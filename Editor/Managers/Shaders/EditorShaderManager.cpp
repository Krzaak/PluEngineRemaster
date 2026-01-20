//
// Created by Plutex on 1/20/26.
//

#include "EditorShaderManager.h"

#include <filesystem>

#include "adl_serializer.hpp"
#include "EditorShaderCode.h"
#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"

extern Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;

Plu::EditorShaderManager::EditorShaderManager()
{
}

Plu::EditorShaderManager::~EditorShaderManager()
{
}

void Plu::EditorShaderManager::PreInit(TUsePointer<EditorProjectManager> editorProjectManager)
{
	mProjectManager = editorProjectManager;
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
			newShaderCode->Init(StringW::FromNarrow(entry.path().string().c_str()));
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
