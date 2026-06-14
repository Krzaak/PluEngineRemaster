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
