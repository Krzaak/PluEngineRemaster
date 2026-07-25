//
// Created by Plutex on 2026-06-20.
//

#ifndef PLUENGINE_PROFILERPANEL_H
#define PLUENGINE_PROFILERPANEL_H
#include "Panels/EditorPanel.h"
#include "ProfilerPanel.generated.h"

namespace Plu
{
	PLU_CLASS()
	class ProfilerPanel : public EditorPanel
	{
		REFLECTION_BODY_PROFILERPANEL()
	public:
		using EditorPanel::EditorPanel;

		String GetPanelName() override;
		void OnUpdate(float deltaTime) override;
		void OnHide() override;
		void OnShow() override;

	private:
		// Filtr wątku: pusty = pokaż wszystkie. Trzymany jako nazwa (nie indeks), bo lista
		// wątków rośnie w trakcie działania i indeks wskazywałby wtedy na inny wpis.
		String mThreadFilter;

		// Asks for a destination and writes the current registry there as CSV.
		void ExportCsv();

		// Result of the last export, shown under the toolbar so the user can see where the file
		// went (or that it failed) without watching the console.
		String mExportStatus;
	};
}

#endif //PLUENGINE_PROFILERPANEL_H
