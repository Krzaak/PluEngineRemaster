//
// Created by Plutex on 1/1/26.
//

#ifndef PLUENGINE_EDITORPANELMANAGER_H
#define PLUENGINE_EDITORPANELMANAGER_H
#include <PluSTL_FWD.h>

#include "PluEngine/Application.h"
#include "PluEngine/Objects/EngineObject.h"
#include "PluEngine/Objects/EngineObjectHandle.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "EditorPanelManager.generated.h"

namespace Plu
{
	struct EditorAppContext;
	class EditorPanel;
	struct ApplicationInfo;

	PLU_CLASS()
	class EditorPanelManager : public EngineObject
	{
		REFLECTION_BODY_EDITORPANELMANAGER()
	private:
		DynamicArray<TOwningPointer<EditorPanel>> mPanels;
		ApplicationInfo* mApplicationInfo;
		EditorAppContext* mEditorAppContext;
	public:
		EditorPanelManager();
		void Init(ApplicationInfo* applicationInfo, EditorAppContext* editorAppContext);
		~EditorPanelManager() override;

		template<class T>
		TUsePointer<T> AddPanel();

		TUsePointer<EditorPanel> AddPanel(const TypeInfo* PanelClass);

		void Init();
		void OnUpdate(float deltaTime);
		void Shutdown();
	};

	template<class T>
	TUsePointer<T> EditorPanelManager::AddPanel()
	{
		static_assert(std::is_base_of_v<EditorPanel, T>, "T should be derived from EditorPanel");
		return AddPanel(T::GetStaticClass());
	}
}

#endif //PLUENGINE_EDITORPANELMANAGER_H