//
// Created by Plutex on 1/14/26.
//

#ifndef PLUENGINE_SCENESTRUCTUREPANEL_H
#define PLUENGINE_SCENESTRUCTUREPANEL_H
#include "EditorViewports/IEditorPanel.h"
#include "PluEngine/PluTypes.h"
#include "PluEngine/Objects/EngineObjectHandle.h"
#include "Array/Array.h"
#include "SceneStructurePanel.generated.h"

namespace Plu
{
	class GameObject;

	PLU_CLASS()
	class SceneStructurePanel : public IEditorPanel
	{
		REFLECTION_BODY_SCENESTRUCTUREPANEL()
	public:
		SceneStructurePanel() = default;
		~SceneStructurePanel() override = default;

		String GetPanelName() override;
		void OnClosed() override;
		void OnOpened() override;
		void OnUpdate(float deltaTime) override;

	private:
		/** Ustawienia popupu "Randomize Transforms". */
		struct RandomizeTransformsSettings
		{
			bool  RandomizeLocation = true;
			Vec3  LocationMin       = Vec3(-5.0f, 0.0f, -5.0f);
			Vec3  LocationMax       = Vec3(5.0f, 0.0f, 5.0f);
			bool  LocationRelative  = false;   // true = offset względem obecnej lokacji

			bool  RandomizeRotation = false;
			Vec3  RotationMin       = Vec3(0.0f);
			Vec3  RotationMax       = Vec3(0.0f, 360.0f, 0.0f);

			bool  RandomizeScale    = false;
			bool  UniformScale      = true;
			float UniformScaleMin   = 0.8f;
			float UniformScaleMax   = 1.2f;
			Vec3  ScaleMin          = Vec3(0.8f);
			Vec3  ScaleMax          = Vec3(1.2f);

			bool  UseFixedSeed      = false;
			int   Seed              = 1234;
		};

		/** Czy handle jest w zaznaczeniu; `-1` gdy nie ma. */
		[[nodiscard]] Int64 FindInSelection(const EngineObjectHandle& handle) const;
		[[nodiscard]] bool  IsSelected(const EngineObjectHandle& handle) const;

		/** Wyrzuca z zaznaczenia handle nieżywych obiektów i podchwytuje zaznaczenie zrobione poza panelem. */
		void SyncSelection(const DynamicArray<TUsePointer<GameObject>>& objects);

		void SelectSingle(const EngineObjectHandle& handle, Int64 index);
		void ToggleSelection(const EngineObjectHandle& handle, Int64 index);
		void SelectRangeTo(const DynamicArray<TUsePointer<GameObject>>& objects, Int64 index);

		/** Primary selection = to, co widzi reszta edytora (details panel, gizmo). */
		void SetPrimarySelection(const EngineObjectHandle& handle);

		void DrawRandomizeWindow();
		void ApplyRandomTransforms();

		/** Wchodzi w tryb edycji nazwy wiersza (podwójny klik albo "Rename" z menu). */
		void BeginRename(const EngineObjectHandle& handle, const String& currentName);
		/** Zatwierdza nazwę z bufora. Pustą i niezmienioną ignoruje — dzięki temu Esc
		 *  (ImGui przywraca wtedy starą wartość) nie brudzi assetu. */
		void CommitRename(const TUsePointer<GameObject>& object);

		DynamicArray<EngineObjectHandle> mSelectedObjects;
		EngineObjectHandle               mRenamingObject;
		bool                             mRenameFocusPending   = false;
		char                             mRenameBuffer[128]    = {};
		Int64                            mSelectionAnchor      = -1;
		bool                             mShowRandomizeWindow  = false;
		/** Stan zadokowania z poprzedniej klatki — obwódkę trzeba wypchnąć przed Begin(). */
		bool                             mRandomizeWindowDocked = false;
		RandomizeTransformsSettings      mRandomizeSettings;
	};
}

#endif //PLUENGINE_STATICMESHDETAILSPANEL_H
