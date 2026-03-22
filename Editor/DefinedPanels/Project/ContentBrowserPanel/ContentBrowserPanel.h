//
// Created by Plutex on 1/10/26.
//

#ifndef PLUENGINE_CONTENTBROWSERPANEL_H
#define PLUENGINE_CONTENTBROWSERPANEL_H
#include "Panels/EditorPanel.h"
#include "ContentBrowserPanel.generated.h"

namespace Plu
{
	PLU_CLASS()
	class ContentBrowserPanel : public EditorPanel
	{
		REFLECTION_BODY_CONTENTBROWSERPANEL()
	private:
		PathW mSelectedFile;
		void FileNode(const PathW& path);
		void DirectoryNode(const PathW& path);
		void EntryNode(const PathW& start);

		DynamicArray<TypeInfo*> mAssetTypesForCreation;
	public:
		using EditorPanel::EditorPanel;

		String GetPanelName() override;
		void OnUpdate(float deltaTime) override;
		void OnHide() override;
		void OnShow() override;
	};
}

#endif //PLUENGINE_CONTENTBROWSERPANEL_H