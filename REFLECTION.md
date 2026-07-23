# Reflection System — PluEngine

Makra refleksji są no-opami w C++ (`#define PLU_CLASS(...)`) — działają tylko jako adnotacje dla `ReflectionGeneratorRegex.py`, który parsuje je przez libclang i generuje pliki `*.generated.h` do `ReflectionCache/`.

> **Zawsze uruchom generator po zmianie jakiegokolwiek makra refleksji.**
> ```bash
> cd PythonTools && python ReflectionGeneratorRegex.py
> # Tylko jeden plik (szybciej):
> python ReflectionGeneratorRegex.py --file path/to/MyClass.h
> ```

---

## PLU_CLASS(...)

Oznacza klasę do refleksji. Wymagane jest makro `REFLECTION_BODY_CLASSNAME()` wewnątrz definicji klasy.

| Specifier | Opis |
|---|---|
| `Abstract` | Klasa abstrakcyjna — nie jest rejestrowana jako bezpośrednio instancjonowalna |
| `PyExport` | Eksportuje klasę do Pythona przez pybind11 |
| `PyDerive` | Pozwala na tworzenie podklas tej klasy z Pythona (generuje trampoline) |
| `NoReflection` | Generuje tylko bindingi Pythona; pomija `.generated.h` i rejestrację w `TypeRegistry` |
| `UUID=FieldName` | Wskazuje pole `PluUUID` jako identyfikator klasy (np. `UUID=Uuid`) |

```cpp
// Klasa eksportowana do Pythona, możliwa do podklasowania
PLU_CLASS(PyExport, PyDerive)
class MyComponent : public GameObjectComponent {
    REFLECTION_BODY_MYCOMPONENT()
    ...
};

// Abstrakcyjna klasa bazowa
PLU_CLASS(Abstract)
class IManager : public EngineObject {
    REFLECTION_BODY_IMANAGER()
    ...
};

// Klasa z UUID i tylko bindingami Pythona (bez pełnej refleksji)
PLU_CLASS(NoReflection)
class InputHandler : public EngineObject { ... };
```

---

## PLU_STRUCT(...)

Jak `PLU_CLASS`, ale dla struktur (domyślny access to `public`).

| Specifier | Opis |
|---|---|
| `PyExport` | Eksportuje strukturę do Pythona |
| `UUID=FieldName` | Wskazuje pole `PluUUID` jako identyfikator struktury |
| `NoVirtualClass` | Wymusza niewirtualne `GetClass()` — struct zostaje POD-em bez vtable (patrz niżej) |

```cpp
PLU_STRUCT(PyExport, UUID=Uuid)
struct AssetInfo {
    REFLECTION_BODY_ASSETINFO()

    PLU_PROPERTY(PyExport)
    PluUUID Uuid;

    PLU_PROPERTY(PyExport)
    String Name;
};
```

### Wirtualne `GetClass()` w hierarchiach structów

Struct **uczestniczący w hierarchii zreflektowanych structów** dostaje **wirtualne**
`GetClass()`, dzięki czemu przez wskaźnik/referencję do bazy zwraca właściwy `TypeInfo*`:

- struct dziedziczący po nie-POD zreflektowanym structcie → `virtual GetClass() override;`
- struct będący bazą dla nie-POD zreflektowanego structa → wprowadza `virtual GetClass();`
- samodzielny struct (bez relacji) → zwykłe, niewirtualne `GetClass();`

`GetClass()` klas (`PLU_CLASS`) jest wirtualne zawsze (jak dotąd).

**`NoVirtualClass`** wyłącza to dla danego structa — zostaje bez vtable i przerywa
propagację wirtualności przez ten węzeł. Używaj dla POD-ów o ściśle kontrolowanym
układzie pamięci, np. wierzchołków wysyłanych surowo na GPU (`Vertex`, `SkeletalVertex`
w `StaticMesh.h`/`SkeletalMesh.h`), gdzie dodanie vtable rozjechałoby `offsetof`/`sizeof`
i wysyłkę bufora. Jeśli oznaczysz bazę jako `NoVirtualClass`, oznacz też pochodne structy
w tej gałęzi (nie da się `override` po niewirtualnej bazie).

---

## PLU_ENUM(...)

Oznacza enum do refleksji.

| Specifier | Opis |
|---|---|
| `PyNamespace=X` | **WYMAGANY.** Namespace w Pythonie (prawie zawsze `PyNamespace=Plu`) |
| `PyExport` | Eksportuje enum do Pythona |

> **Uwaga:** pominięcie `PyNamespace=Plu` powoduje błąd kompilacji.

```cpp
PLU_ENUM(PyNamespace=Plu)
enum class EPhysicsBodyType {
    Static,
    Dynamic,
    Kinematic,
};

// Enum eksportowany i dostępny z Pythona jako Plu.EInputKey
PLU_ENUM(PyExport, PyNamespace=Plu)
enum class EInputKey { ... };
```

---

## PLU_INTERFACE(...)

Wariant `PLU_CLASS` dla klas-interfejsów. Parametry analogiczne jak `PLU_CLASS`.

---

## PLU_PROPERTY(...)

Oznacza pole (member variable) do refleksji. Umieszcza się bezpośrednio nad deklaracją pola.

| Specifier | Opis |
|---|---|
| `PyExport` | Eksportuje pole do Pythona (getter + setter) |
| `PyReadOnly` | Pole dostępne z Pythona tylko do odczytu (tylko getter) |
| `UuidFor=ClassName` | Pole musi być typu `PluUUID`; informuje edytor, że UUID odnosi się do danej klasy (picker UI) |

```cpp
PLU_PROPERTY(PyExport)
Vec3 Location;

PLU_PROPERTY(PyExport, PyReadOnly)
float Mass;

PLU_PROPERTY(UuidFor=IShaderCode)
PluUUID VertexShaderUuid;
```

---

## PLU_FUNCTION(...)

Zachowanie różni się zależnie od tego, czy funkcja jest **metodą klasy** czy **funkcją globalną**.

### Metoda klasy

Umieszcza się nad deklaracją metody wewnątrz klasy/struktury.

| Specifier | Opis |
|---|---|
| `PyExport` | Eksportuje metodę do Pythona |
| `PyOverride` | Metoda może być nadpisana w Pythonie (wymaga `PyDerive` na klasie; generuje trampoline). Błąd kompilacji jeśli klasa nie ma `PyDerive` |
| `PyNotCallable` | Metoda zarejestrowana w refleksji, ale **nie** dostępna do wywołania z Pythona |

```cpp
// Metoda nadpisywalna z Pythona
PLU_FUNCTION(PyOverride)
virtual void OnBeginPlay();

// Metoda tylko po stronie C++ (np. wewnętrzny callback)
PLU_FUNCTION(PyNotCallable)
void InternalTick(float DeltaTime);

// Zwykła metoda eksportowana do Pythona
PLU_FUNCTION(PyExport)
void SetVelocity(Vec3 Velocity);
```

#### Parsowanie deklaracji, wartości domyślne i typy parametrów

- Generator parsuje deklarację z **jednej linii**. Ciało inline jest OK, o ile mieści się w tej samej
  linii i nie ma zagnieżdżonych klamr (`virtual void OnPossessed(...) {};`). Ciało rozbite na kilka
  linii → funkcja jest **cicho pomijana**; wtedy trzeba przenieść definicję do `.cpp`.
- **Wartości domyślne** trafiają do `py::arg("x") = ...`, ale tylko gdy pybind11 na pewno je
  skonwertuje przy rejestracji modułu: literały (`1000.0f`, `true`, `nullptr`, `"str"`), puste
  kontenery PluSTL (`DynamicArray<T>{}`) oraz konstrukcje/wartości typów i enumów wystawionych do
  Pythona (`RaycastDebugSettings()`, `CollisionResponse::Block`). Enumy są rejestrowane w module
  przed klasami właśnie po to, żeby mogły być domyślnymi argumentami.
- Parametry, które w bindingach zmieniają typ (`Vec3` → tuple, `TClassPointer<T>` → `py::object`,
  `std::function<>` → `py::function`), domyślnej wartości **nie dostają** — po stronie Pythona
  pozostają wymagane.
- Parametr `TUsePointer<T>` / `TOwningPointer<T>` dla `T` **niebędącego assetem** dyskwalifikuje całą
  funkcję z bindingów (nie ma castera, a smart pointera nie da się odtworzyć z surowego wskaźnika).
  Taka metoda działa w C++ i w refleksji, ale w Pythonie jej nie ma — również jako `PyOverride`.
  Jeśli ma być dostępna z Pythona, przyjmij `T*` zamiast smart pointera.

### Funkcja globalna (poza klasą)

Funkcje globalne oznaczone `PLU_FUNCTION` są **automatycznie eksportowane do Pythona** — `PyExport` nie jest potrzebny ani rozpoznawany. Jedynym dostępnym specifierem jest `PyNotCallable`, który wyklucza funkcję z bindingów.

`PyOverride` nie ma zastosowania dla funkcji globalnych.

Funkcje globalne **nie generują `.generated.h`** — generator tworzy tylko wpis w module Python.

```cpp
// Automatycznie dostępna w Pythonie jako Plu.GetForwardVector(...)
PLU_FUNCTION()
PLU_API Vec3 GetForwardVector(Vec3 rot);

// Zarejestrowana w refleksji, ale wykluczona z Pythona
PLU_FUNCTION(PyNotCallable)
PLU_API void InternalHelper();
```

---

## Reguły walidacji generatora

- `PyOverride` na metodzie bez `PyDerive` na klasie → `#error` w wygenerowanym pliku
- `UuidFor=X` na polu o typie innym niż `PluUUID` → ostrzeżenie w logu generatora
- `PLU_ENUM` bez `PyNamespace=` → błąd kompilacji (puste namespace w bindingu)
- `PyReadOnly` działa tylko razem z `PyExport` (samo `PyReadOnly` bez eksportu jest ignorowane)

---

## REFLECTION_BODY macro

Każda klasa/struct z `PLU_CLASS`/`PLU_STRUCT` musi mieć w ciele klasowym:

```cpp
REFLECTION_BODY_CLASSNAME()   // zamień CLASSNAME na nazwę klasy wielkimi literami
```

Makro to jest generowane do `ReflectionCache/ClassName.generated.h` i zawiera rejestrację `TypeInfo`, metadanych pól i metod.
