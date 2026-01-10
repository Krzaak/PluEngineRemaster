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
		void FileNode(PathW path);
		void DirectoryNode(PathW path);
		void EntryNode(PathW start);
	public:
		using EditorPanel::EditorPanel;

		void OnUpdate(float deltaTime) override;
		void OnHide() override;
		void OnShow() override;
	};
}

#endif //PLUENGINE_CONTENTBROWSERPANEL_H