import argparse
import json
import os
import re
import sys
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from typing import List, Optional

# ─────────────────────────────────────────────
#  Typy
# ─────────────────────────────────────────────

class ClassType(Enum):
    CLASS     = 1
    STRUCT    = 2
    ENUM      = 3
    INTERFACE = 4
    UNKNOWN   = 5


@dataclass
class PropertyInfo:
    Name:         str
    Type:         str
    Params:       List[str]
    UuidForClass: str = ""


@dataclass
class TypeInfo:
    Name:             str
    Type:             ClassType
    FilePath:         Path
    Bases:            List[str]
    Project:          str
    ReflectionParams: List[str]
    Properties:       List[PropertyInfo]
    UuidProperty:     Optional[PropertyInfo] = None


@dataclass
class FileData:
    FilePath: Path
    Children: List[TypeInfo]


@dataclass
class StructInfoInternal:
    Name:     str
    Bases:    List[str]
    FilePath: str


# ─────────────────────────────────────────────
#  Konfiguracja ścieżek i argumenty
# ─────────────────────────────────────────────

ScriptDir        = os.path.dirname(os.path.abspath(__file__))
ProjectToScanDir = os.path.dirname(ScriptDir)
ProjectDir       = ProjectToScanDir
OutputDir        = os.path.join(ProjectDir, "ReflectionCache")
QuietMode: bool  = False
ForceMode: bool  = False

ArgParser = argparse.ArgumentParser(
    prog="Reflection Generator",
    description="Generates reflection data for PluEngine (Regex-based)"
)
ArgParser.add_argument("-q", "--quiet",   action="store_true")
ArgParser.add_argument("-p", "--project", type=str)
ArgParser.add_argument("-F", "--force",   action="store_true", help="Ignore ProcessedList.txt?")
Args = ArgParser.parse_args()

QuietMode = bool(Args.quiet)
ForceMode = bool(Args.force)

if Args.project:
    if Args.project != "ALL":
        ProjectToScanDir = os.path.join(ProjectToScanDir, Args.project)
    else:
        print("ALL projects selected")
else:
    print("No project selected!")
    sys.exit(1)

if QuietMode: print("QUIET")
if ForceMode: print("FORCE")

# ─────────────────────────────────────────────
#  Wzorce regex
# ─────────────────────────────────────────────

# Makra refleksji nad typem
RE_PLU_CLASS     = re.compile(r"PLU_CLASS\s*\((.*?)\)")
RE_PLU_STRUCT    = re.compile(r"PLU_STRUCT\s*\((.*?)\)")
RE_PLU_INTERFACE = re.compile(r"PLU_INTERFACE\s*\((.*?)\)")

# Makro nad polem
RE_PLU_PROPERTY  = re.compile(r"PLU_PROPERTY\s*\((.*?)\)")

# Deklaracja klasy / struktury
RE_CLASS_DECL  = re.compile(r"^(?:class|struct)\s+(?:[A-Z_]+\s+)?(\w+)(?:\s+final)?\s*(?::\s*(.+))?$")
# Bazowe klasy – wyciągamy jednotkowe identyfikatory po pominięciu specyfikatorów dostępu
RE_BASE_ENTRY  = re.compile(r"(?:public|protected|private)?\s*(\w[\w:<>]*)")

# Access specifiers
RE_ACCESS_SPEC = re.compile(r"^(public|protected|private)\s*:\s*$")

# Deklaracja pola: np.  "float Speed;" lub "TArray<int32> Items;"
# Obsługuje szablony i wskaźniki; zatrzymuje się przed '{' oraz '('
RE_FIELD_DECL  = re.compile(
    r"^([\w:<>*&\s,]+?)\s+(\w+)\s*(?:=\s*[^;{(]+)?;$"
)


# ─────────────────────────────────────────────
#  Parsowanie parametrów makra
# ─────────────────────────────────────────────

def ParseMacroParams(RawParams: str) -> List[str]:
    """Dzieli surowy string parametrów po przecinku i czyści spacje."""
    if not RawParams.strip():
        return []
    return [P.strip() for P in RawParams.split(",") if P.strip()]


def ParseBases(BasesStr: Optional[str]) -> List[str]:
    """Wyciąga nazwy bazowych klas z fragmentu po ':' w deklaracji."""
    if not BasesStr:
        return []
    Result = []
    for Part in BasesStr.split(","):
        Part = Part.strip()
        # Usuń specyfikator dostępu
        Part = re.sub(r"^(public|protected|private)\s+", "", Part)
        Part = Part.strip()
        if Part:
            Result.append(Part)
    return Result


# ─────────────────────────────────────────────
#  Główny parser pliku
# ─────────────────────────────────────────────

def ProcessFile(FilePath: Path, ProjectName: str) -> List[TypeInfo]:
    """
    Przechodzi plik linia po linii.
    Śledzi:
      - aktualny access specifier
      - makra PLU_CLASS / PLU_STRUCT / PLU_INTERFACE (oczekujemy deklaracji w następnej linii)
      - makro PLU_PROPERTY (oczekujemy deklaracji pola w następnej linii)
    """
    if not QuietMode:
        print(f"Processing {FilePath} ...")

    with open(FilePath, "r", errors="ignore") as F:
        Lines = F.readlines()

    FoundTypes: List[TypeInfo]   = []
    CurrentType: Optional[TypeInfo] = None

    # Stan oczekiwania na następną linię
    PendingTypeMacro: Optional[str]      = None   # "CLASS" | "STRUCT" | "INTERFACE"
    PendingTypeMacroParams: List[str]    = []
    PendingPropertyMacro: bool           = False
    PendingPropertyParams: List[str]     = []

    # Aktualny access specifier (dla klas domyślnie private, dla struktur public)
    CurrentAccess: str = "private"

    # Prosta heurystyka głębokości nawiasów klamrowych
    BraceDepth:     int = 0
    TypeStartDepth: int = -1  # głębokość '{' na której zaczął się CurrentType

    for LineIdx, RawLine in enumerate(Lines):
        Line = RawLine.strip()

        # ── Liczenie nawiasów klamrowych ──────────────────────────────
        BraceDepth += Line.count("{") - Line.count("}")

        # ── Zamknięcie bieżącego typu ─────────────────────────────────
        if CurrentType is not None and BraceDepth < TypeStartDepth:
            if not QuietMode:
                Print_TypeSummary(CurrentType)
            FoundTypes.append(CurrentType)
            CurrentType   = None
            TypeStartDepth = -1
            CurrentAccess  = "private"

        # ── Access specifier ──────────────────────────────────────────
        AccessMatch = RE_ACCESS_SPEC.match(Line)
        if AccessMatch:
            CurrentAccess = AccessMatch.group(1)
            PendingPropertyMacro = False  # makro musi być bezpośrednio nad polem
            continue

        # ── Oczekiwanie na deklarację TYPU ───────────────────────────
        if PendingTypeMacro is not None:
            DeclMatch = RE_CLASS_DECL.match(Line)
            if DeclMatch:
                TypeName  = DeclMatch.group(1)
                BasesStr  = DeclMatch.group(2)
                Bases     = ParseBases(BasesStr)

                ClassKind = ClassType.CLASS
                if PendingTypeMacro == "STRUCT":
                    ClassKind = ClassType.STRUCT
                elif PendingTypeMacro == "INTERFACE":
                    ClassKind = ClassType.INTERFACE

                CurrentType = TypeInfo(
                    Name             = TypeName,
                    Type             = ClassKind,
                    FilePath         = FilePath,
                    Bases            = Bases,
                    Project          = ProjectName,
                    ReflectionParams = PendingTypeMacroParams,
                    Properties       = [],
                    UuidProperty     = None,
                )
                # Głębokość przy otwierającym '{' – szukamy go w tej lub kolejnych liniach
                # Często '{' jest na tej samej linii co deklaracja
                if "{" in Line:
                    TypeStartDepth = BraceDepth
                else:
                    # '{' będzie w następnej linii – ustawimy po kolejnym kroku liczenia
                    TypeStartDepth = BraceDepth + 1

                # Dla struktury domyślny access to public
                CurrentAccess = "public" if ClassKind == ClassType.STRUCT else "private"

                if not QuietMode:
                    print(f"  [FOUND] {ClassKind.name}: {TypeName} -> {Bases}")

            PendingTypeMacro       = None
            PendingTypeMacroParams = []
            continue

        # ── Oczekiwanie na deklarację POLA ───────────────────────────
        if PendingPropertyMacro:
            FieldMatch = RE_FIELD_DECL.match(Line)
            if FieldMatch and CurrentType is not None:
                FieldType = FieldMatch.group(1).strip()
                FieldName = FieldMatch.group(2).strip()
                NewProp = PropertyInfo(
                    Name         = FieldName,
                    Type         = FieldType,
                    Params       = PendingPropertyParams,
                    UuidForClass = "",
                )
                CurrentType.Properties.append(NewProp)
                if not QuietMode:
                    print(f"      [PROP] {FieldType} {FieldName}  params={PendingPropertyParams}  access={CurrentAccess}")
            PendingPropertyMacro  = False
            PendingPropertyParams = []
            continue

        # ── Wykrycie makra PLU_CLASS ──────────────────────────────────
        M = RE_PLU_CLASS.search(Line)
        if M:
            PendingTypeMacro       = "CLASS"
            PendingTypeMacroParams = ParseMacroParams(M.group(1))
            continue

        # ── Wykrycie makra PLU_STRUCT ─────────────────────────────────
        M = RE_PLU_STRUCT.search(Line)
        if M:
            PendingTypeMacro       = "STRUCT"
            PendingTypeMacroParams = ParseMacroParams(M.group(1))
            continue

        # ── Wykrycie makra PLU_INTERFACE ──────────────────────────────
        M = RE_PLU_INTERFACE.search(Line)
        if M:
            PendingTypeMacro       = "INTERFACE"
            PendingTypeMacroParams = ParseMacroParams(M.group(1))
            continue

        # ── Wykrycie makra PLU_PROPERTY ───────────────────────────────
        M = RE_PLU_PROPERTY.search(Line)
        if M and CurrentType is not None:
            PendingPropertyMacro  = True
            PendingPropertyParams = ParseMacroParams(M.group(1))
            continue

    # Koniec pliku – jeśli typ nie został zamknięty (brak '}') dodajemy go
    if CurrentType is not None:
        if not QuietMode:
            Print_TypeSummary(CurrentType)
        FoundTypes.append(CurrentType)

    return FoundTypes


def Print_TypeSummary(T: TypeInfo):
    print(f"    -> {T.Type.name} '{T.Name}'  bases={T.Bases}  props={[P.Name for P in T.Properties]}")


# ─────────────────────────────────────────────
#  Walidacja parametrów (params_check)
# ─────────────────────────────────────────────

def ParamsCheck(Data: List[TypeInfo]):
    for Cls in Data:
        for Prop in Cls.Properties:
            if Prop.Type.endswith("*") or Prop.Type.endswith("&"):
                print(f"Warning: {Cls.Name}::{Prop.Name} jest wskaźnikiem/referencją ({Prop.Type}).")
            if any(C in Prop.Type for C in ("std::vector", "std::map", "std::unordered_map")):
                print(f"Warning: {Cls.Name}::{Prop.Name} używa kontenera STL. Użyj PluSTL.")
            if any(C in Prop.Type for C in ("std::shared_ptr", "std::unique_ptr")):
                print(f"Warning: {Cls.Name}::{Prop.Name} używa smart-pointer STL. Użyj PluSTL.")
            if "glm::" in Prop.Type:
                print(f"Warning: {Cls.Name}::{Prop.Name} używa typu GLM. Użyj PluTypes.h.")
            if Prop.Type == "auto":
                print(f"Error: {Cls.Name}::{Prop.Name} ma typ 'auto' – refleksja niemożliwa.")

            for Param in Prop.Params:
                if Param.startswith("UuidFor="):
                    UuidForClass = Param.split("=", 1)[1]
                    if Prop.Type not in ("PluUUID", "Plu::PluUUID"):
                        print(f"Error: {Cls.Name}::{Prop.Name} – UuidFor={UuidForClass} ale typ to '{Prop.Type}', oczekiwano 'PluUUID'.")
                    else:
                        print(f"Info: {Cls.Name}::{Prop.Name} – prawidłowy UUID prop dla '{UuidForClass}'.")
                        Prop.UuidForClass = UuidForClass

        for Param in Cls.ReflectionParams:
            if Param.startswith("UUID="):
                UuidPropName = Param.split("=", 1)[1]
                MatchingProp = next((P for P in Cls.Properties if P.Name == UuidPropName), None)
                if MatchingProp is None:
                    print(f"Error: {Cls.Name} – UUID={UuidPropName} – brak takiego pola.")
                elif MatchingProp.Type not in ("PluUUID", "Plu::PluUUID"):
                    print(f"Error: {Cls.Name} – UUID prop '{UuidPropName}' ma typ '{MatchingProp.Type}', oczekiwano 'PluUUID'.")
                else:
                    print(f"Info: {Cls.Name} – prawidłowe UUID pole '{UuidPropName}'.")
                    Cls.UuidProperty = MatchingProp


# ─────────────────────────────────────────────
#  Platforma i filtrowanie folderów
# ─────────────────────────────────────────────

# Mapowanie nazwa folderu Platforms/<X> → prefiks sys.platform
PlatformFolderMap: dict = {
    "Windows": "win",
    "Linux":   "linux",
    "MacOS":   "darwin",
    "IOS":     "ios",
}

def IsExcludedPlatformDir(DirPath: str) -> bool:
    """
    Zwraca True jeśli folder jest podfolderem Platforms/<Platforma>
    która nie pasuje do bieżącej platformy.
    Działa zarówno dla ścieżek z separatorem / jak i \\.
    """
    NormDir = DirPath.replace("\\", "/")
    for FolderName, SysPlatformPrefix in PlatformFolderMap.items():
        if f"/Platforms/{FolderName}" in NormDir:
            return not sys.platform.startswith(SysPlatformPrefix)
    return False


# ─────────────────────────────────────────────
#  Skanowanie projektu
# ─────────────────────────────────────────────

FilesToProcess: List[Path] = []

def ScanProject() -> List[Path]:
    print(f"Scanning project at {ProjectToScanDir} ...")
    print(f"Platform: {sys.platform}")
    Found: List[Path] = []
    for DirPath, _, Files in os.walk(ProjectToScanDir):
        if any(Skip in DirPath for Skip in ("cmake-build", ".git", "ReflectionCache")):
            continue
        if IsExcludedPlatformDir(DirPath):
            if not QuietMode:
                print(f"  Skipping dir {DirPath} (other platform)")
            continue
        for FileName in Files:
            if not FileName.endswith(".h") or FileName.endswith("generated.h"):
                continue
            FullPath = Path(DirPath, FileName)
            try:
                Content = FullPath.read_text(errors="ignore")
            except OSError:
                continue
            if "PLU_CLASS" not in Content and "PLU_STRUCT" not in Content and "PLU_INTERFACE" not in Content:
                continue
            Found.append(FullPath)
    print(f"Found {len(Found)} file(s) to process.")
    return Found


# ─────────────────────────────────────────────
#  Generowanie plików refleksji
# ─────────────────────────────────────────────

def IsStructAsset(Cls: StructInfoInternal, AllClasses: List[StructInfoInternal]) -> bool:
    for Base in Cls.Bases:
        for C in AllClasses:
            if C.Name == Base:
                if C.Name == "IAssetInfo":
                    return True
                return IsStructAsset(C, AllClasses)
    return False


def IsTypeDerivedFrom(BaseName: str, Derived: TypeInfo, AllTypes: List[TypeInfo]) -> bool:
    for B in Derived.Bases:
        if B == BaseName:
            return True
        for T in AllTypes:
            if T.Name == B and IsTypeDerivedFrom(BaseName, T, AllTypes):
                return True
    return False


def GenerateReflectionData(Data: List[FileData]):
    if not Data:
        print("No reflection changes.")
        return

    print(f"Generating reflection data for {len(Data)} file(s)...")

    # ── Zbierz projekty ───────────────────────────────────────────────
    ProjectGroups: List[str] = []
    for F in Data:
        Proj = F.Children[0].Project
        if Proj not in ProjectGroups:
            ProjectGroups.append(Proj)

    if ForceMode:
        for Proj in ProjectGroups:
            ClassListPath = os.path.join(OutputDir, Proj, "ClassList.txt")
            if os.path.exists(ClassListPath):
                os.remove(ClassListPath)

    # ── Zapisz ClassList.txt ──────────────────────────────────────────
    for F in Data:
        Proj            = F.Children[0].Project
        ClassListFile   = os.path.join(OutputDir, Proj, "ClassList.txt")
        os.makedirs(os.path.dirname(ClassListFile), exist_ok=True)

        ExistingContent = ""
        if os.path.exists(ClassListFile):
            ExistingContent = open(ClassListFile).read()

        with open(ClassListFile, "a") as CL:
            for Cls in F.Children:
                if Cls.Name not in ExistingContent or ForceMode:
                    CL.write(f"{Cls.Name} - {Cls.Bases} - {Cls.Type} - {Cls.FilePath}\n")

    print(f"Found projects: {ProjectGroups}")

    # ── Wczytaj ALL_CLASSES ───────────────────────────────────────────
    AllClasses: List[TypeInfo] = []
    for Proj in ProjectGroups:
        ClassListFile = os.path.join(OutputDir, Proj, "ClassList.txt")
        if not os.path.exists(ClassListFile):
            continue
        with open(ClassListFile, "r") as CL:
            for RawLine in CL:
                Parts = RawLine.strip().split(" - ")
                if len(Parts) < 4:
                    continue
                ClassName    = Parts[0]
                ClassBases   = eval(Parts[1])
                ClassTypeStr = Parts[2].removeprefix("ClassType.")
                ClassPath    = Parts[3]
                AllClasses.append(TypeInfo(
                    Name             = ClassName,
                    Type             = ClassType[ClassTypeStr],
                    FilePath         = Path(ClassPath),
                    Bases            = ClassBases,
                    Project          = Proj,
                    ReflectionParams = [],
                    Properties       = [],
                    UuidProperty     = None,
                ))

    # ── Odfiltruj interfejsy z baz ────────────────────────────────────
    InterfaceType = next((T for T in AllClasses if T.Name == "PluInterface"), None)
    for F in Data:
        for Cls in F.Children:
            if len(Cls.Bases) > 1 and InterfaceType:
                ToRemove = [B for B in Cls.Bases[1:] if IsTypeDerivedFrom("PluInterface", Cls, AllClasses)]
                for B in ToRemove:
                    print(f"Info: {Cls.Name} implementuje interfejs {B}, usuwam z baz.")
                    Cls.Bases.remove(B)

    # ── Generuj .generated.h i .generated.cpp ─────────────────────────
    for FileEntry in Data:
        FilePath    = FileEntry.FilePath
        Proj        = FileEntry.Children[0].Project
        GenHeader   = os.path.join(OutputDir, Proj, FilePath.stem + ".generated.h")
        GenSource   = os.path.join(OutputDir, Proj, FilePath.stem + ".generated.cpp")

        # --- .generated.h ---
        with open(GenHeader, "w") as H:
            H.write("#pragma once\n")
            H.write("#include <PluEngine/Reflection/ReflectionBase.h>\n\n")

            for Cls in FileEntry.Children:
                IsStruct = Cls.Type == ClassType.STRUCT
                ClsUpper = Cls.Name.upper()
                H.write(f"#define REFLECTION_BODY_{ClsUpper}() \\\n")
                H.write(f"    public: \\\n")
                H.write(f"        static Plu::TypeInfo* GetStaticClass(); \\\n")
                if not IsStruct:
                    H.write(f"        virtual Plu::TypeInfo* GetClass() override; \\\n")
                else:
                    H.write(f"        Plu::TypeInfo* GetClass(); \\\n")
                H.write(f"    private: \\\n")
                if not IsStruct:
                    H.write(f"        friend void Register_Reflection_{Cls.Name}();\n")
                else:
                    H.write(f"        friend void Register_Reflection_{Cls.Name}(); \\\n")
                    H.write(f"        public:\n")

        # --- .generated.cpp ---
        # Zbierz UUID props do dodatkowych include'ów
        UuidProps: dict = {}
        for Cls in FileEntry.Children:
            for Prop in Cls.Properties:
                if Prop.UuidForClass:
                    UuidClassPath = ""
                    for C in AllClasses:
                        if C.Name == Prop.UuidForClass:
                            UuidClassPath = str(C.FilePath)
                    UuidProps[Prop.Name] = {"class": Prop.UuidForClass, "classPath": UuidClassPath}

        with open(GenSource, "w") as S:
            S.write(f'#include "{FilePath}"\n')
            S.write("#include <PluEngine/Reflection/TypeTraits.h>\n\n")

            WrittenIncludes = set()
            for _, Info in UuidProps.items():
                if Info["classPath"] and Info["classPath"] not in WrittenIncludes:
                    WrittenIncludes.add(Info["classPath"])
                    S.write(f'#include "{Info["classPath"]}"\n')

            for Cls in FileEntry.Children:
                for Prop in Cls.Properties:
                    if "TUsePointer<" in Prop.Type:
                        Inner = re.sub(r"(?:Plu::)?TUsePointer<(.+)>", r"\1", Prop.Type)
                        for C in AllClasses:
                            if C.Name == Inner and str(C.FilePath) not in WrittenIncludes:
                                WrittenIncludes.add(str(C.FilePath))
                                S.write(f'#include "{C.FilePath}"\n')

            S.write(f'#include "{FilePath.stem}.generated.h"\n\n')
            S.write(f"using namespace Plu;\n\n")

            for Cls in FileEntry.Children:
                if Cls.Type == ClassType.INTERFACE:
                    print("Skipping interface reflection")
                    continue

                IsStruct = Cls.Type == ClassType.STRUCT

                # GetStaticClass
                S.write(f"TypeInfo* {Cls.Name}::GetStaticClass() {{\n")
                S.write(f"    static TypeInfo* instance = nullptr;\n")
                S.write(f"    if (!instance) {{\n")
                S.write(f'        instance = new TypeInfo(sizeof({Cls.Name}), "{Cls.Name}", TypeType::{Cls.Type.name});\n')

                if "Abstract" not in Cls.ReflectionParams:
                    S.write(f"        instance->Constructor = []() -> void* {{ return new {Cls.Name}(); }};\n")

                if Cls.Bases:
                    S.write(f"        instance->BaseType = {Cls.Bases[0]}::GetStaticClass();\n")

                for Prop in Cls.Properties:
                    S.write(f'        PropertyInfo* prop{Prop.Name} = new PropertyInfo{{ '
                            f'"{Prop.Name}", offsetof({Cls.Name}, {Prop.Name}), sizeof({Prop.Type}), '
                            f'PropertyType::Unknown, "{Prop.Type}" }};\n')
                    S.write(f'        prop{Prop.Name}->SerializePtr = TypeSerializer<{Prop.Type}>::Serialize;\n')
                    S.write(f'        prop{Prop.Name}->DeserializePtr = TypeSerializer<{Prop.Type}>::Deserialize;\n')
                    S.write(f'        prop{Prop.Name}->EditorControlPtr = TypeSerializer<{Prop.Type}>::EditorControl;\n')
                    if Prop.UuidForClass:
                        if Prop.UuidForClass in [C.Name for C in AllClasses]:
                            S.write(f'        prop{Prop.Name}->UuidForClass = {Prop.UuidForClass}::GetStaticClass();\n')
                        else:
                            print(f"Error: UuidFor '{Prop.UuidForClass}' dla '{Prop.Name}' w '{Cls.Name}' nie znaleziono.")
                    S.write(f'        instance->AddProperty(prop{Prop.Name});\n')

                if Cls.UuidProperty:
                    S.write(f'        instance->TypeUuidProp = instance->FindProperty("{Cls.UuidProperty.Name}");\n')

                S.write(f'        instance->SerializeToJson = [](void* obj) -> nlohmann::json {{\n')
                S.write(f'            {Cls.Name}* objPtr = static_cast<{Cls.Name}*>(obj);\n')
                S.write(f'            return TypeSerializer<TypeInfo*>::Serialize(instance, objPtr);\n')
                S.write(f'        }};\n')
                S.write(f'        instance->DeserializeFromJson = [](DeserializationContext* dc, nlohmann::json& j) -> void* {{\n')
                S.write(f'            {Cls.Name}* objPtr = static_cast<{Cls.Name}*>(TypeSerializer<TypeInfo*>::Deserialize(dc, j, instance));\n')
                S.write(f'            return objPtr;\n')
                S.write(f'        }};\n')
                S.write(f"    }}\n")
                S.write(f"    return instance;\n")
                S.write(f"}}\n\n")

                S.write(f"TypeInfo* {Cls.Name}::GetClass() {{ return GetStaticClass(); }}\n\n")

                S.write(f"void Register_Reflection_{Cls.Name}() {{\n")
                S.write(f"    TypeInfo* info = {Cls.Name}::GetStaticClass();\n")
                S.write(f"    TypeRegistry::GetInstance()->AddType(info);\n")
                S.write(f"}}\n\n")

    # ── Generuj EditorAssetObjectsCreators.cpp (tylko gdy projekt Editor istnieje) ──
    if "Editor" in ProjectGroups:
        # Wczytaj wszystkie struktury ze wszystkich ClassList.txt
        AllStructs: List[StructInfoInternal] = []
        for Proj in ProjectGroups:
            ClassListFile = os.path.join(OutputDir, Proj, "ClassList.txt")
            if not os.path.exists(ClassListFile):
                continue
            with open(ClassListFile, "r") as CL:
                for RawLine in CL:
                    Parts = RawLine.strip().split(" - ")
                    if len(Parts) < 4:
                        continue
                    EntryTypeStr = Parts[2].removeprefix("ClassType.")
                    if EntryTypeStr == "STRUCT":
                        AllStructs.append(StructInfoInternal(
                            Name     = Parts[0],
                            Bases    = eval(Parts[1]),
                            FilePath = Parts[3],
                        ))

        # Odfiltruj tylko te struktury które dziedziczą po IAssetInfo
        AssetStructs: List[StructInfoInternal] = [
            S for S in AllStructs if IsStructAsset(S, AllStructs)
        ]

        EditorAssetsPath = os.path.join(OutputDir, "Editor", "EditorAssetObjectsCreators.cpp")
        os.makedirs(os.path.dirname(EditorAssetsPath), exist_ok=True)
        with open(EditorAssetsPath, "w") as EA:
            EA.write('#include <PluEngine/Reflection/ReflectionBase.h>\n')
            EA.write('#include <PluEngine/Reflection/TypeTraits.h>\n\n')
            EA.write('#include "PluEngine/Objects/EngineObjectManager.h"\n')
            EA.write('#include "Managers/Assets/EditorAssetManager.h"\n')
            EA.write('#include "Managers/Assets/EditorAssetObject.h"\n')
            for S in AssetStructs:
                EA.write(f'#include "{S.FilePath.replace(os.sep, "/")}"\n')
            EA.write('using namespace Plu;\n\n')
            EA.write('extern TUsePointer<EngineObjectManager> gEngineObjectManager;\n\n')
            EA.write('void InitEditorAssetObjectCreators()\n{\n')
            for S in AssetStructs:
                EA.write(
                    f'    EditorTypeRegistry::GetInstance()->AddConstructor('
                    f'{S.Name}::GetStaticClass()->TypeName, '
                    f'[](TOwningPointer<IAssetInfo> info) -> TOwningPointer<IEditorAssetObject> {{\n'
                )
                EA.write(f'        EngineObjectHandle handle_{S.Name} = gEngineObjectManager->CreateObject<EditorAssetObject<{S.Name}>>();\n')
                EA.write(f'        TOwningPointer<EditorAssetObject<{S.Name}>> editorAssetObject = gEngineObjectManager->GetObjectAsOwner<IEditorAssetObject>(handle_{S.Name});\n')
                EA.write(f'        editorAssetObject->AssetInfo = StaticCast<{S.Name}>(info);\n')
                EA.write(f'        return editorAssetObject;\n')
                EA.write(f'    }});\n')
            EA.write('}\n')

        print(f"Generated EditorAssetObjectsCreators.cpp with {len(AssetStructs)} asset struct(s).")

    # ── Generuj Init*Reflection.cpp ───────────────────────────────────
    for Proj in ProjectGroups:
        ClassListFile = os.path.join(OutputDir, Proj, "ClassList.txt")
        ProjectClassList: List[List[str]] = []
        with open(ClassListFile, "r") as CL:
            for RawLine in CL:
                Parts = RawLine.strip().split(" - ")
                if len(Parts) >= 3:
                    ProjectClassList.append([Parts[0], Parts[2]])

        InitPath = os.path.join(OutputDir, Proj, f"Init{Proj}Reflection.cpp")
        with open(InitPath, "w") as I:
            I.write("#include <PluEngine/Reflection/ReflectionBase.h>\n\n")
            I.write(f"// Project: {Proj}\n\n")
            for Entry in ProjectClassList:
                if Entry[1].removeprefix("ClassType.") == "INTERFACE":
                    continue
                I.write(f"extern void Register_Reflection_{Entry[0]}();\n")
            I.write("\n")
            if Proj == "Editor":
                I.write("extern void InitEditorAssetObjectCreators();\n\n")
            if Proj == "Editor":
                I.write(f"void Init{Proj}Reflection()\n")
            else:
                I.write(f"void PLU_API Init{Proj}Reflection()\n")
            I.write("{\n")
            if Proj == "Editor":
                I.write("    InitEditorAssetObjectCreators();\n")
            for Entry in ProjectClassList:
                if Entry[1].removeprefix("ClassType.") == "INTERFACE":
                    continue
                I.write(f"    Register_Reflection_{Entry[0]}();\n")
            I.write("}\n")

    print(f"Success! Generated reflection data for {len(Data)} file(s) in {OutputDir}")


# ─────────────────────────────────────────────
#  Punkt wejścia
# ─────────────────────────────────────────────

if __name__ == "__main__":
    Files = ScanProject()
    AllReflectionData: List[FileData] = []

    for FilePath in Files:
        # Wyznacz nazwę projektu (pierwszy subfolder względem korzenia projektu)
        try:
            Relative     = FilePath.relative_to(Path(os.path.dirname(ScriptDir)))
            ProjectName  = Relative.parts[0]
        except ValueError:
            ProjectName = "Unknown"

        # Obsługa ProcessedList.txt (cache modyfikacji)
        ProjectOutputDir     = os.path.join(OutputDir, ProjectName)
        os.makedirs(ProjectOutputDir, exist_ok=True)
        ProcessedListPath    = os.path.join(ProjectOutputDir, "ProcessedList.txt")
        FileMtime            = str(os.path.getmtime(FilePath))
        FileName             = FilePath.name

        # Wczytaj istniejący cache (niezależnie od tego czy plik istnieje)
        CacheData: dict = {}
        if os.path.exists(ProcessedListPath):
            try:
                with open(ProcessedListPath, "r") as PL:
                    CacheData = json.load(PL)
            except (json.JSONDecodeError, OSError):
                CacheData = {}

        if not ForceMode and CacheData.get(FileName) == FileMtime:
            if not QuietMode:
                print(f"Skipping {FileName} (not modified)")
            continue

        # Zaktualizuj wpis i zapisz cały słownik z powrotem
        CacheData[FileName] = FileMtime
        try:
            with open(ProcessedListPath, "w") as PL:
                PL.write(json.dumps(CacheData, indent=2))
        except OSError as E:
            print(f"Warning: nie udało się zapisać ProcessedList.txt: {E}")

        Types = ProcessFile(FilePath, ProjectName)
        if Types:
            ParamsCheck(Types)
            AllReflectionData.append(FileData(FilePath=FilePath, Children=Types))
            for T in Types:
                print(f"  Found {T.Type.name}: {T.Name} in {FileName} (project: {T.Project})")

    GenerateReflectionData(AllReflectionData)