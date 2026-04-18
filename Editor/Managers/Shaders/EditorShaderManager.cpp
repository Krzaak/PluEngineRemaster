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
#include "PluEngine/Application.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Shaders/ShaderProgram.h"
#include "PluEngine/Window/Window.h"

extern Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;
extern Plu::EditorAppContext* gEditorAppContext;
extern Plu::ApplicationInfo* gApplicationInfo;

Plu::PathW Plu::EditorShaderWriter::GetShaderCacheDirectory()
{
	return gEditorAppContext->EditorProjectManager->GetProjectCacheDirectory();
}

std::mutex shadersToRecompileMutex;
DynamicArray<Plu::Path> shadersToRecompile;

void Plu::EFSWShaderUpdateListener::handleFileAction(efsw::WatchID watchid, const std::string &dir,
	const std::string &filename, efsw::Action action, std::string oldFilename)
{
	switch (action) {
		case efsw::Action::Add:
			PLU_ERROR("Shader code addition when running is not implemented! You need to restart the Editor to see them");
			break;
		case efsw::Action::Delete:
			PLU_ERROR("Shader code deletion when Running is not implemented! Quitting");
			gApplicationInfo->AppWindow->Close();
			break;
		case efsw::Action::Modified:
		{
			std::lock_guard lock(shadersToRecompileMutex);
			String shaderPath = String(dir.c_str()) + filename.c_str();
			PLU_TRACE("Shader Code changed at {}", shaderPath.CStr());
			shadersToRecompile.PushBack(shaderPath);
			break;
		}
		case efsw::Action::Moved:
			break;
	}
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
			TUsePointer<ShaderProgram> shaderProgramUser = gEngineObjectManager->CreateObject(ShaderProgram::GetStaticClass());
			TOwningPointer<ShaderProgram> shaderProgram = gEngineObjectManager->GetObjectAsOwner<ShaderProgram>(shaderProgramUser->GetObjectHandle());
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
		}
		loaded = true;
		materialsToLoad.Clear();
	});
}

void Plu::EditorShaderManager::ShaderCodeScan()
{
	std::optional<nlohmann::json> jsonProjectShaders = DiskManager::LoadJson(mProjectManager->GetProjectCacheDirectory().ToString() + L"/ShaderCodeUuids.json");
	std::optional<nlohmann::json> jsonEngineShaders = DiskManager::LoadJson(EditorProjectManager::GetEngineAssetsPath().ToString() + L"/ShaderCodeUuids.json");

	gEditorAppContext->EditorPythonManager->RunScript(
		GetEngineResourcesDir().Append(L"PythonTools/").ToString() + StringW(L"ShaderCodeParser.py"),
		std::filesystem::current_path().wstring().c_str(),
		"--project " + gEditorAppContext->EditorProjectManager->GetProjectDirectory().ToString().ToNarrow() + " --engine " + EditorProjectManager::GetEngineAssetsPath().ToString().ToNarrow()
	);

	PathW scanDir = mProjectManager->GetProjectShadersDirectory();
	DynamicArray<std::pair<PathW, bool>> shaderCodes;
#ifdef PLU_PLATFORM_WINDOWS
	for (auto entry : std::filesystem::recursive_directory_iterator(scanDir.CStr())) {
#else
	for (const auto& entry : std::filesystem::recursive_directory_iterator(scanDir.ToString().ToNarrow().CStr())) {
#endif
		if (entry.is_regular_file() && (entry.path().extension() == PLU_SHADER_FRAG_EXT || entry.path().extension() == PLU_SHADER_VERT_EXT)) {
			shaderCodes.PushBack({entry.path().wstring().c_str(), false});
		}
	}
#ifdef PLU_PLATFORM_WINDOWS
	for (auto entry : std::filesystem::recursive_directory_iterator(EditorProjectManager::GetEngineAssetsPath().CStr())) {
#else
	for (const auto& entry : std::filesystem::recursive_directory_iterator(EditorProjectManager::GetEngineAssetsPath().ToString().ToNarrow().CStr())) {
#endif
		if (entry.is_regular_file() && (entry.path().extension() == PLU_SHADER_FRAG_EXT || entry.path().extension() == PLU_SHADER_VERT_EXT)) {
			shaderCodes.PushBack({entry.path().wstring().c_str(), true});
		}
	}
	for (const auto& path : shaderCodes)
	{
		EngineObjectHandle codeHandle = gEngineObjectManager->CreateObject<EditorShaderCode>();
		TOwningPointer<EditorShaderCode> newShaderCode = gEngineObjectManager->GetObjectAsOwner<EditorShaderCode>(codeHandle);
		newShaderCode->Init(path.first.CStr());
		std::optional<JSON> json = path.second ? jsonEngineShaders : jsonProjectShaders;
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
				if (path.second) {
					DiskManager::SaveJson(EditorProjectManager::GetEngineAssetsPath().ToString() + L"/ShaderCodeUuids.json", json.value());
				} else {
					DiskManager::SaveJson(mProjectManager->GetProjectCacheDirectory().ToString() + L"/ShaderCodeUuids.json", json.value());
				}
			}
		} else {
			PluUUID newUuid = PluUUID();
			mShaderCodes.Insert(newUuid, newShaderCode);
			newShaderCode->Uuid = newUuid;
			json = nlohmann::json();
			json.value()[newShaderCode->Name.CStr()] = newUuid.getUUID();
			if (path.second) {
				DiskManager::SaveJson(EditorProjectManager::GetEngineAssetsPath().ToString() + L"/ShaderCodeUuids.json", json.value());
			} else {
				DiskManager::SaveJson(mProjectManager->GetProjectCacheDirectory().ToString() + L"/ShaderCodeUuids.json", json.value());
			}
		}
	}
	mFileWatcher = new efsw::FileWatcher();
	mListener = new EFSWShaderUpdateListener();
	mEngineShadersWatchId = mFileWatcher->addWatch(EditorProjectManager::GetEngineAssetsPath().ToString().ToNarrow().CStr(), mListener);
	std::string error = efsw::Errors::Log::getLastErrorLog();
	if (!error.empty()) {
		PLU_ERROR("{}", error.c_str());
	}
	mProjectShadersWatchId = mFileWatcher->addWatch(gEditorAppContext->EditorProjectManager->GetProjectShadersDirectory().ToString().ToNarrow().CStr(), mListener);
	error = efsw::Errors::Log::getLastErrorLog();
	if (!error.empty()) {
		PLU_ERROR("{}", error.c_str());
	}
	if (mFileWatcher) {
		mFileWatcher->watch();
	}
}

void Plu::EditorShaderManager::InitShaders()
{
}

void Plu::EditorShaderManager::CheckForShaderChanges()
{
	std::lock_guard lock(shadersToRecompileMutex);
	if (shadersToRecompile.IsEmpty()) return;
	DynamicArray<TUsePointer<EditorShaderCode>> shaderCodes;
	for (auto path : shadersToRecompile) {
		for (auto code : mShaderCodes) {
			if (code.second->GetPath().ToString().ToNarrow() == path.ToString()) {
				if (!shaderCodes.Contains(code.second)) {
					shaderCodes.PushBack(code.second);
				}
				break;
			}
		}
	}
	PLU_TRACE("{} shader code changed", shaderCodes.Size());
	for (auto code : shaderCodes) {
		RecompileShaderCode(code);
	}
	shadersToRecompile.Clear();
}

void Plu::EditorShaderManager::RecompileShaderCode(TUsePointer<EditorShaderCode> shaderCode)
{
	for (auto program : mInitializedShaderPrograms) {
		if (program->GetFragmentShader()->Uuid == shaderCode->Uuid || program->GetVertexShader()->Uuid == shaderCode->Uuid) {
			program->UnloadProgram();
			program->Recompile();
		}
	}
}

void Plu::EditorShaderManager::ShaderCodeChanged(PathW shaderCodePath)
{
	shaderCodePath = shaderCodePath.GetNormalized();
	for (auto code : mShaderCodes) {
		if (code.second->GetPath() == shaderCodePath) {
			RecompileShaderCode(code.second);
			break;
		}
	}
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
