//
// Created by Plutex on 7/19/26.
//

#ifndef PLUENGINE_INSTANCEDSTATICMESHCOMPONENT_H
#define PLUENGINE_INSTANCEDSTATICMESHCOMPONENT_H
#include "PluEngine/GameObject/WorldComponent.h"
#include "InstancedStaticMeshComponent.generated.h"
#include "PluEngine/Renderer/RenderingInterfaces.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"

namespace Plu
{
	PLU_STRUCT()
	struct PLU_API MeshInstanceTransform
	{
		REFLECTION_BODY_MESHINSTANCETRANSFORM()

		PLU_PROPERTY()
		Vec3 Location = Vec3(0.0f);
		PLU_PROPERTY()
		Vec3 Rotation = Vec3(0.0f); // Euler degrees, jak WorldComponent.
		PLU_PROPERTY()
		Vec3 Scale = Vec3(1.0f);
	};

	// Dziedziczy bezpośrednio z WorldComponent, NIE z StaticMeshComponent: PhysicsWorld testuje
	// IsDerivedOfOrSame(StaticMeshComponent) w trzech miejscach i zbudowałby jedno ciało kolizji
	// z transformu komponentu, ignorując wszystkie instancje. Osobna baza usuwa problem bez
	// dotykania fizyki, kosztem powtórzenia StaticMeshToDisplay/Material/CastsShadow/MeshBoundingBox.
	PLU_CLASS(PyExport)
	class PLU_API InstancedStaticMeshComponent : public WorldComponent
	{
		REFLECTION_BODY_INSTANCEDSTATICMESHCOMPONENT()
	private:
		// Cache macierzy świata per instancja, przebudowywany leniwie w GetInstanceWorldMatrices()
		// gdy Instances się zmieniło (mInstanceCacheDirty LUB zawartość różni się od mCachedInstances)
		// albo world matrix komponentu jest inny niż przy ostatniej przebudowie. WorldComponent nie ma
		// wirtualnego hooka na zmianę transformu (MarkWorldMatrixForRegeneration jest prywatne),
		// więc porównujemy GetWorldMatrix() wprost zamiast dodawać taki hook.
		//
		// mInstanceCacheDirty sam w sobie NIE wystarczy: łapie tylko mutacje przez AddInstance/
		// RemoveInstance/UpdateInstance/ClearInstances/SetInstances. Edycja z panelu detali (reflekcja
		// - ArrayTreeEditorControl w TypeTraits.cpp) oraz deserializacja sceny piszą prosto po bajtach
		// `Instances` przez PropertyInfo::GetPtr, z pominięciem tych setterów - bez porównania zawartości
		// takie edycje wyglądałyby jak "ignorowane" (wartość w panelu się zmienia, ale zrenderowane
		// instancje stoją w miejscu, bo cache nigdy się nie przebudowuje). MeshInstanceTransform jest
		// POD (3x Vec3, brak paddingu), więc memcmp całej tablicy jest tani i bezpieczny.
		DynamicArray<Matrix4> mCachedWorldMatrices;
		// Równoległa do mCachedWorldMatrices: transpose(inverse(world)) per instancja, liczona
		// w tej samej przebudowie cache'u — inverse() 4x4 per instancja per klatka jest za drogie
		// w RenderSnapshotBuilderze przy dużej liczbie instancji.
		DynamicArray<Matrix4> mCachedNormalMatrices;
		DynamicArray<MeshInstanceTransform> mCachedInstances;
		Matrix4 mCachedComponentWorldMatrix = Matrix4(0.0f);
		bool mInstanceCacheDirty = true;
	public:
		InstancedStaticMeshComponent() = default;
		~InstancedStaticMeshComponent() override = default;

		PLU_PROPERTY(Setter=SetStaticMesh, Getter=GetStaticMesh)
		TUsePointer<StaticMesh> StaticMeshToDisplay;

		PLU_PROPERTY()
		TUsePointer<MaterialInfo> Material;

		PLU_PROPERTY(PyExport)
		bool CastsShadow = true;

		BoundingBox MeshBoundingBox;
		// Twardy guard, ten sam kontrakt co StaticMeshComponent::MeshBoundingBoxComputed:
		// CreateBoundingBoxForStaticMesh chodzi po każdym wierzchołku, więc liczymy raz.
		bool MeshBoundingBoxComputed = false;

		// Transformy instancji względem komponentu (lokalne Location/Rotation/Scale).
		PLU_PROPERTY()
		DynamicArray<MeshInstanceTransform> Instances;

		PLU_FUNCTION(PyExport)
		TUsePointer<StaticMesh> GetStaticMesh();
		PLU_FUNCTION(PyExport)
		void SetStaticMesh(TUsePointer<StaticMesh> staticMesh);

		PLU_FUNCTION(PyExport)
		TUsePointer<MaterialInfo> GetMaterial();
		PLU_FUNCTION(PyExport)
		void SetMaterial(TUsePointer<MaterialInfo> material);

		PLU_FUNCTION(PyExport)
		Int32 AddInstance(Vec3 loc, Vec3 rot, Vec3 scale);
		// Swap-with-last (jak Unreal HISM) - O(1), ale NIE zachowuje stabilności indeksów: po
		// usunięciu, instancja spod ostatniego indeksu ląduje pod zwolnionym `index`.
		PLU_FUNCTION(PyExport)
		bool RemoveInstance(Int32 index);
		PLU_FUNCTION(PyExport)
		bool UpdateInstance(Int32 index, Vec3 loc, Vec3 rot, Vec3 scale);
		PLU_FUNCTION(PyExport)
		void ClearInstances();
		PLU_FUNCTION(PyExport)
		Int32 GetInstanceCount() const;
		PLU_FUNCTION(PyExport)
		void ReserveInstances(Int32 count);

		// Bulk, tylko C++: nie eksportować do Pythona dopóki nie zweryfikowano, że generator
		// bindingów radzi sobie z DynamicArray<PLU_STRUCT> (patrz plan instancingu).
		void SetInstances(const DynamicArray<MeshInstanceTransform>& instances);

		// Macierze świata per instancja (world matrix komponentu * lokalny transform instancji).
		// Jedyny producent mCachedWorldMatrices - przebudowuje cache gdy jest brudny.
		const DynamicArray<Matrix4>* GetInstanceWorldMatrices();
		// Macierze normalnych per instancja, równoległe (ten sam indeks) do GetInstanceWorldMatrices().
		// Wewnętrznie odświeża cache przez GetInstanceWorldMatrices().
		const DynamicArray<Matrix4>* GetInstanceNormalMatrices();
	};
}

#endif //PLUENGINE_INSTANCEDSTATICMESHCOMPONENT_H
