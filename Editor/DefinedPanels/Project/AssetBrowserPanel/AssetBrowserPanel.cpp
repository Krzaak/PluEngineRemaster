//
// Created by Plutex on 2026-03-28.
//

#include "AssetBrowserPanel.h"

#include "EditorAppContext.h"
#include "ImGuiFileDialog.h"
#include "EditorViewports/EditorViewportManager.h"
#include "Managers/Assets/EditorAssetCreator.h"
#include "Managers/Assets/EditorAssetImporter.h"
#include "Managers/Assets/EditorAssetManager.h"
#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/Application.h"
#include "PluEngine/Assets/AssetDescriptor.h"
#include "PluEngine/Assets/AssetLoader.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/Managers/AssetsManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Reflection/ClassPointer.h"
#include "UI/IconsFontAwesome7.h"

Plu::String Plu::AssetBrowserPanel::GetPanelName()
{
	switch (mActiveDirectory) {
	    case EAssetDirectory::EngineAssets:
        case EAssetDirectory::Assets:
	        return ICON_FA_FOLDER_CLOSED " Assets Browser###AssetBrowser";
            break;
        case EAssetDirectory::Scripts:
	        return ICON_FA_FOLDER_CLOSED " Scripts Browser###AssetBrowser";
            break;
        case EAssetDirectory::Shaders:
	        return ICON_FA_FOLDER_CLOSED " Shaders Browser###AssetBrowser";
            break;
    }
    return "Content Browser";
}

// AssetBrowserPanel.cpp

void Plu::AssetBrowserPanel::OnUpdate(float deltaTime)
{
    if (!BeginPanel()) { EndPanel(); return; }

    // ── root path na podstawie aktywnego directory ────────────────────────────
    auto GetRootPath = [this]() -> PathW
    {
        switch (mActiveDirectory)
        {
            case EAssetDirectory::Scripts: return mEditorAppContext->EditorProjectManager->GetProjectScriptsDirectory();
            case EAssetDirectory::Shaders: return mEditorAppContext->EditorProjectManager->GetProjectShadersDirectory();
            case EAssetDirectory::EngineAssets: return mEditorAppContext->EditorProjectManager->GetEngineAssetsPath();
            default:                       return mEditorAppContext->EditorProjectManager->GetProjectAssetsDirectory();
        }
    };

    static TUsePointer<EditorAssetCreator> assetCreator;

    if (ImGui::BeginPopupModal("Asset Creator: Type Selection")) {
        for (auto type : mAssetTypesForCreation) {
            if (ImGui::Selectable(type->TypeName.CStr()))
            {
                if (!assetCreator) {
                    assetCreator = mApplicationInfo->AppObjectManager->CreateObject(EditorAssetCreator::GetStaticClass());
                    assetCreator->Initialize(type, mApplicationInfo->AppAssetManager);
                    assetCreator->GetObjectEventDispatcher()->Subscribe("Finito", [this](void*) {
                        mApplicationInfo->AppObjectManager->DestroyObject(*assetCreator->GetEngineObjectHandle());
                        assetCreator = nullptr;
                    });
                }
                //mEditorAppContext->EditorAssetManager->CreateAsset(type, mEditorAppContext->EditorProjectManager->GetProjectAssetsDirectory());
            }
        }
        if (ImGui::Button("Create")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (assetCreator) assetCreator->RenderUI();
    //mEditorAppContext->EditorAssetManager->HandleAssetCreationUI();

    // ── toolbar ───────────────────────────────────────────────────────────────
    bool openCreator = false;
    if (ImGui::BeginPopupContextItem("AssetImport"))
    {
        if (ImGui::Selectable("Import Asset(s)")) {
            ImGuiFileDialog::Instance()->OpenDialog(
                "ImportAsset",
                "Select Asset",
                ".fbx, .obj, .png, .jpg",
                IGFD::FileDialogConfig(".", "","", 1, IGFDUserDatas(), ImGuiFileDialogFlags_Modal)
            );
        }
        if (ImGui::Selectable("Create Asset")) {
            mAssetTypesForCreation.Clear();
            for (auto type : *TypeRegistry::GetInstance()->GetTypeMap()) {
                if (!type.second->IsDerivedOf(IAssetData::GetStaticClass())) continue;
                mAssetTypesForCreation.PushBack(type.second);
                openCreator = true;
            }
        }
        ImGui::EndPopup();
    }
    if (openCreator) {
        ImGui::OpenPopup("Asset Creator: Type Selection");
    }

    static TUsePointer<EditorAssetImporter> assetImporter;

    if (ImGuiFileDialog::Instance()->Display("ImportAsset"))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            if (!assetImporter) {
                assetImporter = mApplicationInfo->AppObjectManager->CreateObject(EditorAssetImporter::GetStaticClass());
                assetImporter->Initialize({filePath.c_str()}, mApplicationInfo);
                assetImporter->GetObjectEventDispatcher()->Subscribe("Finito", [this](void*) {
                    mApplicationInfo->AppObjectManager->DestroyObject(*assetImporter->GetEngineObjectHandle());
                    assetImporter = nullptr;
                });
            }
        }

        ImGuiFileDialog::Instance()->Close();
    }

    if (assetImporter) assetImporter->RenderUI();

    if (ImGui::Button(ICON_FA_PLUS"")) {
        ImGui::OpenPopup("AssetImport");
    }
    ImGui::SameLine();

    constexpr const char* DirectoryLabels[] = { "Assets", "Scripts", "Shaders", "Engine Assets" };
    int activeIndex = static_cast<int>(mActiveDirectory);

    ImGui::SetNextItemWidth(110.0f * ImGui::GetFontSize() / 13);
    if (ImGui::Combo("##dirtype", &activeIndex, DirectoryLabels, IM_ARRAYSIZE(DirectoryLabels)))
    {
        mActiveDirectory = static_cast<EAssetDirectory>(activeIndex);
        mCurrentPath     = PathW();       // reset — inicjalizacja poniżej
        mSelectedPath    = PathW();
        mNavigationHistory.Clear();
    }

    if (mCurrentPath.IsEmpty())
        mCurrentPath = GetRootPath();

    ImGui::SameLine();

    const bool canGoBack = !mNavigationHistory.IsEmpty();
    if (!canGoBack) ImGui::BeginDisabled();
    if (ImGui::ArrowButton("##back", ImGuiDir_Left))
    {
        mCurrentPath = mNavigationHistory.Back();
        mNavigationHistory.PopBack();
    }
    if (!canGoBack) ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::Text("%ls", mCurrentPath.CStr());
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    WalkDirectory(std::filesystem::path(mCurrentPath.CStr()));

    EndPanel();
}

// ─────────────────────────────────────────────────────────────────────────────
void Plu::AssetBrowserPanel::WalkDirectory(const std::filesystem::path& dirPath)
{
    constexpr float CItemSize = 80.0f;
    constexpr float CPadding  = 6.0f;

    float ItemSize = CItemSize * ImGui::GetFontSize() / 13;
    float Padding  = CPadding * ImGui::GetFontSize() / 13;

    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const int   columns        = std::max(1, static_cast<int>(availableWidth / (ItemSize + Padding)));

    int col = 0;

    auto DrawAndAdvance = [&](const PathW& path, bool isDir)
    {
        DrawAssetItem(path, isDir);

        ++col;
        if (col < columns)
            ImGui::SameLine(0.0f, Padding);
        else
        {
            col = 0;
            ImGui::Spacing();
        }
    };

    for (const auto& entry : std::filesystem::directory_iterator(dirPath))
        if (entry.is_directory())
            DrawAndAdvance(PathW(entry.path().wstring().c_str()), true);

    for (const auto& entry : std::filesystem::directory_iterator(dirPath))
        if (!entry.is_directory())
            DrawAndAdvance(PathW(entry.path().wstring().c_str()), false);
}

// ─────────────────────────────────────────────────────────────────────────────

void Plu::AssetBrowserPanel::DrawAssetItem(const PathW& path, bool isDirectory)
{
    constexpr float CItemSize    = 80.0f;
    constexpr float CIconSize    = 44.0f;
    constexpr float CLabelHeight = 16.0f;
    constexpr float CPadding     = 6.0f;

    float fontSizeMultiplier = ImGui::GetFontSize() / 13;

    float ItemSize    = CItemSize * fontSizeMultiplier;
    float IconSize    = CIconSize * fontSizeMultiplier;
    float LabelHeight = CLabelHeight * fontSizeMultiplier;
    float Padding     = CPadding * fontSizeMultiplier;

    const float availableWidth  = ImGui::GetContentRegionAvail().x;
    const int   columns         = std::max(1, static_cast<int>(availableWidth / (ItemSize + Padding)));

    // wyciągnij samą nazwę pliku/folderu z pełnej ścieżki
    std::filesystem::path fsPath(path.CStr());
    const std::wstring labelW = fsPath.stem().wstring();

    const bool isSelected = (path == mSelectedPath);

    ImGui::PushID(path.ToString().ToNarrow().CStr());

    // ── bounding box kafelka ──────────────────────────────────────────────────
    ImVec2      itemMin  = ImGui::GetCursorScreenPos();
    ImVec2      itemMax  = ImVec2(itemMin.x + ItemSize, itemMin.y + ItemSize);
    ImDrawList* draw     = ImGui::GetWindowDrawList();
    const bool  hovered  = ImGui::IsMouseHoveringRect(itemMin, itemMax);

    // ── tło ───────────────────────────────────────────────────────────────────
    if (isSelected)
    {
        draw->AddRectFilled(itemMin, itemMax,
            IM_COL32(50, 110, 200, 70), 6.0f * fontSizeMultiplier);
        draw->AddRect(itemMin, itemMax,
            IM_COL32(80, 150, 255, 220), 6.0f * fontSizeMultiplier, 0, 1.5f);
    }
    else if (hovered)
    {
        draw->AddRectFilled(itemMin, itemMax,
            ImGui::ColorConvertFloat4ToU32(
                ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered)), 6.0f * fontSizeMultiplier);
    }

    // ── invisible button ─────────────────────────────────────────────────────
    ImGui::InvisibleButton("##item", ImVec2(ItemSize, ItemSize));

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        OnAssetClicked(path, isDirectory);

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        OnAssetDoubleClicked(path, isDirectory);

    if (ImGui::BeginPopupContextItem("##ctx"))
    {
        OnAssetContextMenu(path, isDirectory);
        ImGui::EndPopup();
    }

    // ── ikona ─────────────────────────────────────────────────────────────────
    const float iconOffX = itemMin.x + (ItemSize - IconSize) * 0.5f;
    const float iconOffY = itemMin.y + 6.0f;

    if (isDirectory)
    {
        // folder — prosty kształt z "uszkiem"
        draw->AddRectFilled(
            ImVec2(iconOffX,          iconOffY + 8.0f),
            ImVec2(iconOffX + IconSize, iconOffY + IconSize),
            IM_COL32(200, 160, 60, 200), 4.0f);
        draw->AddRectFilled(
            ImVec2(iconOffX,          iconOffY + 2.0f),
            ImVec2(iconOffX + IconSize * 0.5f, iconOffY + 12.0f),
            IM_COL32(200, 160, 60, 200), 2.0f);
    }
    else
    {
        // plik — szare pole; wstaw tutaj draw->AddImage(texID, ...) gdy masz thumbnail
        const bool hasAsset = mEditorAppContext->EditorAssetManager
                                  ->AssetExistsInPath(path.ToString().ToNarrow());
        const ImU32 iconColor = hasAsset
            ? IM_COL32(80, 120, 200, 180)   // znany asset — niebieski
            : IM_COL32(80,  80, 100, 120);  // nieznany — szary

        draw->AddRectFilled(
            ImVec2(iconOffX, iconOffY),
            ImVec2(iconOffX + IconSize, iconOffY + IconSize),
            iconColor, 4.0f * fontSizeMultiplier);
    }

    // ── label ─────────────────────────────────────────────────────────────────
    const float labelY = itemMin.y + ItemSize - LabelHeight - 4.0f;

    // oblicz szerokość tekstu żeby wyśrodkować
    const char* labelCStr = String::FromWide(labelW.c_str()).CStr();
    const float textWidth = ImGui::CalcTextSize(labelCStr).x;
    const float labelX = itemMin.x + std::max(0.0f, (ItemSize - textWidth) * 0.5f);

    draw->AddText(
        ImGui::GetFont(),
        ImGui::GetFontSize() - 1.0f,
        ImVec2(labelX, labelY),     // <-- labelX zamiast itemMin.x + 4.0f
        isSelected ? IM_COL32(140, 200, 255, 255)
                   : IM_COL32(180, 180, 180, 255),
        labelCStr);

    ImGui::PopID();
}

// ── callbacki ────────────────────────────────────────────────────────────────

void Plu::AssetBrowserPanel::OnAssetClicked(const PathW& path, bool isDirectory)
{
    mSelectedPath = path;   // IsSelected resolves automatycznie przy następnym frame
}

void Plu::AssetBrowserPanel::OnAssetDoubleClicked(const PathW& path, bool isDirectory)
{
    if (isDirectory)
    {
        mNavigationHistory.PushBack(mCurrentPath);  // zapisz poprzedni folder
        mCurrentPath = path;                          // wejdź w nowy
        mSelectedPath = PathW();                      // odznacz zaznaczenie
        return;
    }

    if (mEditorAppContext->EditorAssetManager->AssetExistsInPath(path.ToString().ToNarrow()))
    {
        TUsePointer<AssetDescriptor> assetDescriptor = mApplicationInfo->AppAssetManager->GetAssetDescriptor(path.ToString().ToNarrow());
        auto typeMap = TypeRegistry::GetInstance()->GetTypeMap();
        GameHashMap<String,TClassPointer<IEditorViewport>> viewportClasses;
        for (auto type : *typeMap) {
            if (type.second->IsDerivedOf(IEditorViewport::GetStaticClass())) {
                viewportClasses.Insert(type.first,type.second);
            }
        }
        String viewportClassName = assetDescriptor->AssetType->TypeName + "Viewport";
        if (!viewportClasses.Contains(viewportClassName)) {
            TUsePointer<IAssetLoader> assetLoader = mApplicationInfo->AppAssetManager->GetAssetLoader(assetDescriptor->AssetType);
            if (assetLoader) {
                if (assetLoader->GetAssetTypeViewportClass()) {
                    mEditorAppContext->EditorViewportManager->CreateViewport(path, assetLoader->GetAssetTypeViewportClass());
                    return;
                }
            }
            PLU_ERROR("No viewport class for {}", assetDescriptor->AssetType->TypeName.CStr());
            return;
        }
        TClassPointer<IEditorViewport> viewportClass = viewportClasses[viewportClassName];
        mEditorAppContext->EditorViewportManager->CreateViewport(path, viewportClass);
    }
}

void Plu::AssetBrowserPanel::OnAssetContextMenu(const PathW& path, bool isDirectory)
{
    if (mEditorAppContext->EditorAssetManager->AssetExistsInPath(path.ToString().ToNarrow()))
    {
        // TUsePointer<IEditorAssetObject> assetObject =
        //     mEditorAppContext->EditorAssetManager->GetAssetByPath(path);

        ImGui::MenuItem("Open");
        ImGui::MenuItem("Rename");
        ImGui::Separator();
        ImGui::MenuItem("Delete");
    }
    else if (isDirectory)
    {
        ImGui::MenuItem("New Folder");
        ImGui::MenuItem("Import Asset");
    }
}

void Plu::AssetBrowserPanel::OnHide()
{
}

void Plu::AssetBrowserPanel::OnShow()
{
}
