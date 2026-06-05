//
// Created by Plutex on 1/20/26.
//

#include "EditorShaderManager.h"

#include <filesystem>

#include "adl_serializer.hpp"
#include "EditorAppContext.h"
#include "EditorShaderCode.h"
#include "Managers/Assets/EditorAssetManager.h"
#include "Managers/Project/EditorProjectManager.h"
#include "Managers/Python/EditorPythonManager.h"
#include "PluEngine/Application.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/Assets/AssetDescriptor.h"
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
	gEditorAppContext->EditorAssetManager->GetObjectEventDispatcher()->Subscribe("LoadAssetDescriptor", [this](void* data) {
		UInt64* uuid = static_cast<UInt64 *>(data);
		TUsePointer<AssetDescriptor> asset = gEditorAppContext->EditorAssetManager->GetAssetDescriptor(*uuid);
		if (!asset) return;
		if (asset->AssetType == ShaderProgramInfo::GetStaticClass()) {
			TUsePointer<ShaderProgramInfo> shaderAsset = StaticCast<ShaderProgramInfo>(gEditorAppContext->EditorAssetManager->GetAssetData(asset));
			if (!shaderAsset) return;
			PluUUID vertexShaderUUID = shaderAsset->VertexShaderUuid;
			PluUUID fragmentShaderUUID = shaderAsset->FragmentShaderUuid;
			TUsePointer<ShaderProgram> shaderProgramUser = gEngineObjectManager->CreateObject(ShaderProgram::GetStaticClass());
			TOwningPointer<ShaderProgram> shaderProgram = gEngineObjectManager->GetObjectAsOwner<ShaderProgram>(shaderProgramUser->GetObjectHandle());
			shaderProgram->Uuid = shaderAsset->Uuid;
			TUsePointer<IShaderCode> vertexShader = GetShaderCode(vertexShaderUUID);
			TUsePointer<IShaderCode> fragmentShader = GetShaderCode(fragmentShaderUUID);
			if (!fragmentShader || !vertexShader) {
				if (!fragmentShader && !vertexShader) {
					PLU_ERROR("Loading shader error: There is no Vertex and Fragment Shader!");
					return;
				}
				if (!fragmentShader) {
					PLU_ERROR("Loading shader error: There is no Fragment Shader!");
				} else {
					PLU_ERROR("Loading shader error: There is no Vertex Shader!");
				}
				return;
			}
			PLU_INFO("Shader OK! UUID {}", shaderAsset->Uuid.getUUID());
			shaderProgram->SetFragmentShader(fragmentShader);
			shaderProgram->SetVertexShader(vertexShader);
			mShaderPrograms[shaderProgram->Uuid] = shaderProgram;
			if (!shaderProgram->BinaryExists()) {
				if (shaderProgram->Recompile()) {
					shaderProgram->UnloadProgram();
				}
			}
		}
	});
	gEditorAppContext->EditorAssetManager->GetObjectEventDispatcher()->Subscribe("LoadedAssetData", [this](void* data) {
		UInt64* uuid = static_cast<UInt64 *>(data);
		TUsePointer<AssetDescriptor> asset = gEditorAppContext->EditorAssetManager->GetAssetDescriptor(*uuid);
		if (asset->AssetType == MaterialInfo::GetStaticClass()) {
			TUsePointer material = StaticCast<MaterialInfo>(gApplicationInfo->AppAssetManager->GetAssetData(asset));
			if (!material) return;
			AddMaterialToLoad(material);
			HandleMaterialLoading();
		}
	});
}

void Plu::EditorShaderManager::ShaderCodeScan()
{
	std::optional<nlohmann::json> jsonProjectShaders;
	std::optional<nlohmann::json> jsonEngineShaders = DiskManager::LoadJson(EditorProjectManager::GetEngineAssetsPath().ToString() + L"/ShaderCodeUuids.json");

	if (mProjectManager->IsAnyProjectOpen()) {
		jsonProjectShaders = DiskManager::LoadJson(mProjectManager->GetProjectCacheDirectory().ToString() + L"/ShaderCodeUuids.json");
		gEditorAppContext->EditorPythonManager->RunScript(
		GetEngineResourcesDir().Append(L"PythonTools/").ToString() + StringW(L"ShaderCodeParser.py"),
		std::filesystem::current_path().wstring().c_str(),
		"--project " + gEditorAppContext->EditorProjectManager->GetProjectDirectory().ToString().ToNarrow() + " --engine " + EditorProjectManager::GetEngineAssetsPath().ToString().ToNarrow()
	);
	}

	DynamicArray<std::pair<PathW, bool>> shaderCodes;
	if (mProjectManager->IsAnyProjectOpen()) {
		PathW scanDir = mProjectManager->GetProjectShadersDirectory();
#ifdef PLU_PLATFORM_WINDOWS
		for (auto entry : std::filesystem::recursive_directory_iterator(scanDir.CStr())) {
#else
		for (const auto& entry : std::filesystem::recursive_directory_iterator(scanDir.ToString().ToNarrow().CStr())) {
#endif
			if (entry.is_regular_file() && (entry.path().extension() == PLU_SHADER_FRAG_EXT || entry.path().extension() == PLU_SHADER_VERT_EXT)) {
				shaderCodes.PushBack({entry.path().wstring().c_str(), false});
			}
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
				if (ShaderCodeExists(uuid)) {
					gEngineObjectManager->DestroyObject(codeHandle);
				} else {
					mShaderCodes.Insert(uuid, newShaderCode);
				}
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
	if (!mFileWatcher) {
		mFileWatcher = new efsw::FileWatcher();
		mListener = new EFSWShaderUpdateListener();
		mEngineShadersWatchId = mFileWatcher->addWatch(EditorProjectManager::GetEngineAssetsPath().ToString().ToNarrow().CStr(), mListener, true);
		std::string error = efsw::Errors::Log::getLastErrorLog();
		if (!error.empty()) {
			PLU_ERROR("{}", error.c_str());
		}
		mProjectShadersWatchId = mFileWatcher->addWatch(gEditorAppContext->EditorProjectManager->GetProjectShadersDirectory().ToString().ToNarrow().CStr(), mListener, true);
		error = efsw::Errors::Log::getLastErrorLog();
		if (!error.empty()) {
			PLU_ERROR("{}", error.c_str());
		}
		if (mFileWatcher) {
			mFileWatcher->watch();
		}
	}
}

void Plu::EditorShaderManager::InitShaders()
{
}

void Plu::EditorShaderManager::PrepareShaderCodesForDistribution(Path dir)
{
	Path assetsDir = dir.ToString() + "/ProjectDist";
	std::filesystem::create_directory(assetsDir.CStr());
	std::ofstream outFile((assetsDir.ToString() + "/Shaders.txt").CStr(), std::ios::trunc);
	for (const auto& shaderCode : mShaderCodes) {
		String formattedCode = shaderCode.second->Uuid.toString() + ";";
		formattedCode += PrepareCodeForDistribution(shaderCode.second->GetCode());
		outFile << formattedCode.CStr() << std::endl;
	}
	outFile.close();
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
		String args = "--project " + gEditorAppContext->EditorProjectManager->GetProjectDirectory().ToString().ToNarrow();
		args += " --engine " + EditorProjectManager::GetEngineAssetsPath().ToString().ToNarrow();
		args += " --file " + code->GetPath().ToString().ToNarrow();
		gEditorAppContext->EditorPythonManager->RunScript(
			GetEngineResourcesDir().Append(L"PythonTools/").ToString() + StringW(L"ShaderCodeParser.py"),
			std::filesystem::current_path().wstring().c_str(),
			 args
		);
		RecompileShaderCode(code);
	}
	shadersToRecompile.Clear();
}

void Plu::EditorShaderManager::RecompileShaderCode(TUsePointer<EditorShaderCode> shaderCode)
{
	for (auto programUn : mShaderPrograms) {
		TUsePointer<ShaderProgram> program = programUn.second;
		if (program->GetFragmentShader()->Uuid == shaderCode->Uuid || program->GetVertexShader()->Uuid == shaderCode->Uuid) {
			if (bool isInitialized = mInitializedShaderPrograms.Contains(program)) {
				program->UnloadProgram();
				program->Recompile();
			}
			//Now materials
			shaderCode->RenewUniforms();
			for (auto mat : gEditorAppContext->EditorAssetManager->GetAllAssetDescriptorsOfType(MaterialInfo::GetStaticClass())) {
				TUsePointer<MaterialInfo> materialObject = gEditorAppContext->EditorAssetManager->GetAssetData(mat);
				if (materialObject->shaderProgram == program->Uuid) {
					PLU_TRACE("{}", mat->AssetName.CStr());
					materialObject->MaterialParameters.Clear();
					AddMaterialToLoad(materialObject);
				}
			}
			HandleMaterialLoading();
		}
	}
}

void Plu::EditorShaderManager::AddMaterialToLoad(TUsePointer<MaterialInfo> material)
{
	mMaterialsToLoad.PushBack(material);
}

void Plu::EditorShaderManager::HandleMaterialLoading()
{
	for (TUsePointer<MaterialInfo>& material : mMaterialsToLoad) {
		if (mShaderPrograms.Contains(material->shaderProgram)) {
			auto uniforms = *GetShaderProgram(material->shaderProgram)->GetFragmentShader()->GetCodeUniforms();
			uniforms.Append(*GetShaderProgram(material->shaderProgram)->GetVertexShader()->GetCodeUniforms());
			for (auto uniform : uniforms) {
				TOwningPointer<IShaderUniform>* found =  material->MaterialParameters.FindIf([uniform](const TOwningPointer<IShaderUniform>& property)->bool {
					if (!property) return false;
					if (uniform->Name == property->Name && uniform->Type == property->Type) {
						return true;
					}
					return false;
				});
				if (found != material->MaterialParameters.End()) continue;
				material->MaterialParameters.PushBack(uniform);
			}
			GetShaderProgram(material->shaderProgram)->GetVertexShader()->RenewUniforms();
			GetShaderProgram(material->shaderProgram)->GetFragmentShader()->RenewUniforms();
		} else {
			PLU_ERROR("No shader program with UUID {} for material {}", material->shaderProgram.getUUID(), material->Uuid.getUUID());
		}
	}
	mMaterialsToLoad.Clear();
}

Plu::TUsePointer<Plu::IShaderCode> Plu::EditorShaderManager::GetShaderCode(PluUUID uuid)
{
	if (ShaderCodeExists(uuid)) {
		return mShaderCodes[uuid];
	}
	PLU_ERROR("No shader code with UUID {}", uuid.getUUID());
	return nullptr;
}

bool Plu::EditorShaderManager::ShaderCodeExists(PluUUID uuid)
{
	return mShaderCodes.Contains(uuid);
}

Plu::TUsePointer<Plu::ShaderProgram> Plu::EditorShaderManager::GetShaderProgram(PluUUID uuid)
{
	if (!mShaderPrograms.Contains(uuid)) {
		PLU_ERROR("No such shader program found with UUID {}", uuid.getUUID());
		return nullptr;
	}
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
