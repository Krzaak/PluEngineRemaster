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
class ParamInfo:
    """Pojedynczy parametr funkcji."""
    Type: str
    Name: str


@dataclass
class FunctionInfo:
    """Funkcja/metoda oznaczona PLU_FUNCTION."""
    Name:        str
    ReturnType:  str
    Params:      List[ParamInfo]
    MacroParams: List[str]
    IsVirtual:   bool = False
    IsConst:     bool = False


@dataclass
class GlobalFunctionInfo:
    """Funkcja globalna (poza klasą) oznaczona PLU_FUNCTION."""
    Name:        str
    ReturnType:  str
    Params:      List[ParamInfo]
    MacroParams: List[str]
    FilePath:    Path


@dataclass
class TypeInfo:
    Name:             str
    Type:             ClassType
    FilePath:         Path
    Bases:            List[str]
    Project:          str
    ReflectionParams: List[str]
    Properties:       List[PropertyInfo]
    Functions:        List[FunctionInfo] = field(default_factory=list)
    UuidProperty:     Optional[PropertyInfo] = None


@dataclass
class FileData:
    FilePath:        Path
    Children:        List[TypeInfo]
    GlobalFunctions: List[GlobalFunctionInfo] = field(default_factory=list)


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
QuietMode:    bool = False
ForceMode:    bool = False
BindingsMode: bool = False

ArgParser = argparse.ArgumentParser(
    prog="Reflection Generator",
    description="Generates reflection data for PluEngine (Regex-based)"
)
ArgParser.add_argument("-q", "--quiet",    action="store_true")
ArgParser.add_argument("-p", "--project",  type=str)
ArgParser.add_argument("-F", "--force",    action="store_true", help="Ignore ProcessedList.txt?")
ArgParser.add_argument("-b", "--bindings", action="store_true", help="Generate pybind11 bindings + .pyi stubs")
Args = ArgParser.parse_args()

QuietMode    = bool(Args.quiet)
ForceMode    = bool(Args.force)
BindingsMode = bool(Args.bindings)

if Args.project:
    if Args.project != "ALL":
        ProjectToScanDir = os.path.join(ProjectToScanDir, Args.project)
    else:
        print("ALL projects selected")
else:
    print("No project selected!")
    sys.exit(1)

if QuietMode:    print("QUIET")
if ForceMode:    print("FORCE")
if BindingsMode: print("BINDINGS (pybind11)")

# ─────────────────────────────────────────────
#  Wzorce regex
# ─────────────────────────────────────────────

# Makra refleksji nad typem
RE_PLU_CLASS     = re.compile(r"PLU_CLASS\s*\((.*?)\)")
RE_PLU_STRUCT    = re.compile(r"PLU_STRUCT\s*\((.*?)\)")
RE_PLU_INTERFACE = re.compile(r"PLU_INTERFACE\s*\((.*?)\)")

# Makro nad polem
RE_PLU_PROPERTY  = re.compile(r"PLU_PROPERTY\s*\((.*?)\)")

# Makro nad funkcją
RE_PLU_FUNCTION  = re.compile(r"PLU_FUNCTION\s*\((.*?)\)")

# Deklaracja funkcji – pełna wersja wyciągająca virtual, return type, nazwę, parametry, const
# Grupy: (1) "virtual " lub None, (2) typ zwracany, (3) nazwa, (4) raw params, (5) " const" lub None
RE_FUNC_DECL_FULL = re.compile(
    r"^(virtual\s+)?"
    r"(?:\[\[\w+\]\]\s+)*"       # atrybuty [[nodiscard]] itp.
    r"((?:[\w:<>*&,\s]+?))"          # (2) typ zwracany (non-greedy)
    r"\s+(\w+)\s*"                  # (3) nazwa funkcji
    r"\(([^)]*)\)"                   # (4) surowe parametry
    r"(\s*const)?",                   # (5) const qualifier
    re.MULTILINE
)

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


def ParseFunctionParams(RawParams: str) -> List[ParamInfo]:
    """Parsuje listę parametrów funkcji C++ na listę ParamInfo."""
    RawParams = RawParams.strip()
    if not RawParams or RawParams == "void":
        return []
    Result: List[ParamInfo] = []
    # Dzielimy po przecinkach z uwzględnieniem szablonów (głębokość '<>')
    Depth, Cur = 0, []
    Segments: List[str] = []
    for Ch in RawParams:
        if Ch in "<(": Depth += 1
        elif Ch in ")>": Depth -= 1
        if Ch == "," and Depth == 0:
            Segments.append("".join(Cur).strip()); Cur = []
        else:
            Cur.append(Ch)
    if Cur:
        Segments.append("".join(Cur).strip())

    for Seg in Segments:
        Seg = Seg.strip()
        if not Seg:
            continue
        # Usuń default value
        if "=" in Seg:
            Seg = Seg[:Seg.index("=")].strip()
        # Usuń const ref/ptr kwalifikatory (zostawiamy w typie)
        # Ostatni token to nazwa parametru, reszta to typ
        # Wyjątek: jeśli jeden token – to tylko typ bez nazwy
        Tokens = Seg.split()
        if len(Tokens) == 1:
            Result.append(ParamInfo(Type=Tokens[0], Name=""))
            continue
        # Nazwa to ostatni token (może mieć * lub & na końcu → należy do typu)
        ParamName = Tokens[-1].lstrip("*&")
        ParamType = Seg[:Seg.rfind(Tokens[-1])].strip()
        # Jeśli ostatni token zaczyna się od * lub & to zostawiamy przy typie
        if Tokens[-1][0] in ("*", "&"):
            ParamName = ""
            ParamType = Seg
        Result.append(ParamInfo(Type=ParamType, Name=ParamName))
    return Result


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
    PendingFunctionMacro: bool           = False
    PendingFunctionParams: List[str]     = []

    # Funkcje globalne (poza klasą)
    GlobalFunctions: List[GlobalFunctionInfo] = []

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

        # ── Wykrycie makra PLU_FUNCTION ───────────────────────────────
        M = RE_PLU_FUNCTION.search(Line)
        if M:
            PendingFunctionMacro  = True
            PendingFunctionParams = ParseMacroParams(M.group(1))
            continue

        # ── Oczekiwanie na deklarację FUNKCJI ────────────────────────
        if PendingFunctionMacro:
            PendingFunctionMacro = False
            FM = RE_FUNC_DECL_FULL.match(Line)
            if FM:
                IsVirt     = bool(FM.group(1))
                ReturnType = FM.group(2).strip()
                # Usuń makra eksportu DLL i specyfikatory C++ (PLU_API, inline, static, itp.)
                ReturnType = re.sub(r"\bPLU_API\b|\b\w+_API\b|__declspec\s*\([^)]*\)|__cdecl|__stdcall|__fastcall|\binline\b|\bstatic\b|\bextern\b|\bexplicit\b|\bconstexpr\b|\bconsteval\b|\bconstinit\b|\bfriend\b", "", ReturnType).strip()
                FuncName   = FM.group(3).strip()
                RawParams  = FM.group(4).strip()
                IsConst    = bool(FM.group(5))
                Params     = ParseFunctionParams(RawParams)
                FuncInfo   = FunctionInfo(
                    Name        = FuncName,
                    ReturnType  = ReturnType,
                    Params      = Params,
                    MacroParams = PendingFunctionParams,
                    IsVirtual   = IsVirt,
                    IsConst     = IsConst,
                )
                if CurrentType is not None:
                    CurrentType.Functions.append(FuncInfo)
                    if not QuietMode:
                        print(f"      [FUNC] {ReturnType} {FuncName}({RawParams})  params={PendingFunctionParams}")
                else:
                    # Funkcja globalna
                    GlobalFunctions.append(GlobalFunctionInfo(
                        Name        = FuncName,
                        ReturnType  = ReturnType,
                        Params      = Params,
                        MacroParams = PendingFunctionParams,
                        FilePath    = FilePath,
                    ))
                    if not QuietMode:
                        print(f"  [GLOBAL FUNC] {ReturnType} {FuncName}({RawParams})  params={PendingFunctionParams}")
            PendingFunctionParams = []
            continue

    # Koniec pliku – jeśli typ nie został zamknięty (brak '}') dodajemy go
    if CurrentType is not None:
        if not QuietMode:
            Print_TypeSummary(CurrentType)
        FoundTypes.append(CurrentType)

    return FoundTypes, GlobalFunctions


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

# Makra których obecność w pliku .h kwalifikuje go do przetwarzania przez generator.
# Wystarczy że jedno z nich wystąpi w pliku – dodaj tutaj nowe makra kiedy silnik je dostanie.
REFLECTION_TRIGGER_MACROS: List[str] = [
    "PLU_CLASS",
    "PLU_STRUCT",
    "PLU_INTERFACE",
    "PLU_FUNCTION",   # pliki z samymi funkcjami globalnymi
]


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
            if not any(Macro in Content for Macro in REFLECTION_TRIGGER_MACROS):
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


# ─────────────────────────────────────────────
#  Pomocniki dla pybind11
# ─────────────────────────────────────────────

# Mapowanie prostych typów C++ → Python dla stubów .pyi
CPP_TO_PY_TYPE: dict = {
    # Typy całkowite
    "int":     "int",  "int8":    "int",  "int16":  "int",  "int32":  "int",  "int64":  "int",
    "uint8":   "int",  "uint16":  "int",  "uint32": "int",  "uint64": "int",
    "size_t":  "int",  "ptrdiff_t": "int",
    # Aliasy silnika (using Int32 = std::int32_t; itp.)
    "Int8":    "int",  "Int16":   "int",  "Int32":  "int",  "Int64":  "int",
    "UInt8":   "int",  "UInt16":  "int",  "UInt32": "int",  "UInt64": "int",
    # Zmiennoprzecinkowe
    "float":   "float", "double": "float",
    # Logiczne
    "bool":    "bool",
    # Stringi – wszystkie warianty BasicString/String
    "String":        "str",  "StringW":       "str",
    "BasicString":   "str",
    "std::string":   "str",  "std::wstring":  "str",
    # UUID
    "PluUUID": "str",
    # Void
    "void":    "None",
    # pybind11 typy (np. parametry RegisterPluClass)
    "pybind11::type": "type",  "py::type": "type",
    "pybind11::object": "object", "py::object": "object",
    # glm – tymczasowo jako Tuple, docelowo własne bindingi
    "Vec2":      "Tuple[float, float]",
    "Vec3":      "Tuple[float, float, float]",
    "Vec4":      "Tuple[float, float, float, float]",
    "Quat":      "Tuple[float, float, float, float]",
    "glm::vec2": "Tuple[float, float]",
    "glm::vec3": "Tuple[float, float, float]",
    "glm::vec4": "Tuple[float, float, float, float]",
    "glm::quat": "Tuple[float, float, float, float]",
}

# Regex do rozpoznawania szablonów kontenerów (kolejność ma znaczenie – bardziej szczegółowe pierwsze)
_RE_DYNAMIC_ARRAY   = re.compile(r"^DynamicArray\s*<(.+)>$")
_RE_GAME_HASH_MAP   = re.compile(r"^GameHashMap\s*<(.+),\s*(.+?)(?:,.*)?>\s*$")  # K, V[, hasher, alloc]
_RE_BASIC_STRING    = re.compile(r"^BasicString\s*<.*>$")
_RE_USE_POINTER     = re.compile(r"^TUsePointer\s*<(.+)>$")
_RE_OWNING_POINTER  = re.compile(r"^TOwningPointer\s*<(.+)>$")

def _StripOuterTemplate(s: str) -> Optional[str]:
    """Jeśli string to Foo<X>, zwraca X. Uwzględnia zagnieżdżone nawiasy."""
    s = s.strip()
    Open = s.find("<")
    if Open == -1 or not s.endswith(">"):
        return None
    Depth = 0
    for I, C in enumerate(s):
        if C == "<": Depth += 1
        elif C == ">": Depth -= 1
    return s[Open + 1:-1].strip() if Depth == 0 else None

def _SplitTemplateArgs(s: str) -> List[str]:
    """Dzieli argumenty szablonu po najwyższym poziomie przecinków."""
    Args, Depth, Cur = [], 0, []
    for C in s:
        if C == "<": Depth += 1
        elif C == ">": Depth -= 1
        if C == "," and Depth == 0:
            Args.append("".join(Cur).strip()); Cur = []
        else:
            Cur.append(C)
    if Cur:
        Args.append("".join(Cur).strip())
    return Args

def CppTypeToPy(CppType: str) -> str:
    """Konwertuje typ C++ → Python do stubów .pyi. Obsługuje szablony silnika."""
    Raw   = CppType.strip()
    # Usuń namespace Plu::
    Clean = re.sub(r"\bPlu::", "", Raw).strip()

    # ── Strippuj const, &, * (np. "const Vec3&" → "Vec3") ────────────
    Clean = re.sub(r"\bconst\b", "", Clean).replace("&", "").replace("*", "")
    Clean = " ".join(Clean.split())  # normalizuj spacje

    # ── Kontenery silnika ──────────────────────────────────────────────

    # TClassPointer<T> → Type[T]  (używane jako PLU_PROPERTY)
    M = re.match(r"^(?:Plu::)?TClassPointer\s*<(.+)>$", Clean)
    if M:
        return f"Type[{CppTypeToPy(M.group(1))}]"

    # DynamicArray<T> → List[T]
    M = _RE_DYNAMIC_ARRAY.match(Clean)
    if M:
        return f"List[{CppTypeToPy(M.group(1))}]"

    # GameHashMap<K, V, ...> → Dict[K, V]
    Inner = _StripOuterTemplate(Clean)
    if Clean.startswith("GameHashMap") and Inner:
        Parts = _SplitTemplateArgs(Inner)
        if len(Parts) >= 2:
            return f"Dict[{CppTypeToPy(Parts[0])}, {CppTypeToPy(Parts[1])}]"

    # BasicString<char/wchar_t, ...> → str
    if _RE_BASIC_STRING.match(Clean):
        return "str"

    # TUsePointer<X> / TOwningPointer<X> → Optional[X]
    M = _RE_USE_POINTER.match(Clean) or _RE_OWNING_POINTER.match(Clean)
    if M:
        return f"Optional[{CppTypeToPy(M.group(1))}]"

    return CPP_TO_PY_TYPE.get(Clean, Clean)


def HasPyParam(Params: List[str], ParamName: str) -> bool:
    return ParamName in Params


def GetPyParamValue(Params: List[str], Key: str) -> Optional[str]:
    for P in Params:
        if P.startswith(f"{Key}="):
            return P.split("=", 1)[1]
    return None


def IsPyExported(TypeOrProp) -> bool:
    """Zwraca True jeśli typ/pole ma parametr PyExport."""
    return HasPyParam(TypeOrProp.Params if isinstance(TypeOrProp, PropertyInfo)
                      else TypeOrProp.ReflectionParams, "PyExport")


def GetPyName(Cls: TypeInfo) -> str:
    """Zwraca nazwę Pythonową klasy (PyName=X lub oryginalna)."""
    Name = GetPyParamValue(Cls.ReflectionParams, "PyName")
    return Name if Name else Cls.Name


def GetPyDoc(Params: List[str]) -> str:
    """Zwraca napis PyDoc= jeśli podany, inaczej pusty string."""
    Val = GetPyParamValue(Params, "PyDoc")
    return Val.replace("_", " ") if Val else ""


# Mapowanie typów glm → tuple C++ używane w lambdach konwerterów
GLM_TYPE_MAP: dict = {
    "Vec2":      ("glm::vec2", "float x, float y",                    "glm::vec2{x, y}",             "std::tuple<float,float>"),
    "Vec3":      ("glm::vec3", "float x, float y, float z",           "glm::vec3{x, y, z}",          "std::tuple<float,float,float>"),
    "Vec4":      ("glm::vec4", "float x, float y, float z, float w",  "glm::vec4{x, y, z, w}",       "std::tuple<float,float,float,float>"),
    "Quat":      ("glm::quat", "float w, float x, float y, float z",  "glm::quat{w, x, y, z}",       "std::tuple<float,float,float,float>"),
    "glm::vec2": ("glm::vec2", "float x, float y",                    "glm::vec2{x, y}",             "std::tuple<float,float>"),
    "glm::vec3": ("glm::vec3", "float x, float y, float z",           "glm::vec3{x, y, z}",          "std::tuple<float,float,float>"),
    "glm::vec4": ("glm::vec4", "float x, float y, float z, float w",  "glm::vec4{x, y, z, w}",       "std::tuple<float,float,float,float>"),
    "glm::quat": ("glm::quat", "float w, float x, float y, float z",  "glm::quat{w, x, y, z}",       "std::tuple<float,float,float,float>"),
}

_RE_CLASS_POINTER = re.compile(r"(?:Plu::)?TClassPointer\s*<(.+)>")

def _HasClassPointer(Params: List[ParamInfo]) -> bool:
    """Zwraca True jeśli którykolwiek parametr to TClassPointer<T> (obsługuje const T&, Plu::)."""
    for P in Params:
        Clean = re.sub(r"\bPlu::", "", _StripQualifiers(P.Type)).strip()
        if _RE_CLASS_POINTER.match(Clean):
            return True
    return False

def _NeedsLambda(Params: List[ParamInfo], ReturnType: str, AllClasses: List = []) -> bool:
    """Zwraca True jeśli funkcja wymaga lambdy (glm, TClassPointer, TUsePointer asset param/return)."""
    if _NeedsGlmLambda(Params, ReturnType) or _HasClassPointer(Params):
        return True
    # Return type: TUsePointer/TOwningPointer → .GetRaw()
    CleanRet = re.sub(r"\bPlu::", "", _StripQualifiers(ReturnType)).strip()
    if _RE_USE_POINTER.match(CleanRet) or _RE_OWNING_POINTER.match(CleanRet):
        return True
    # Parametr: TUsePointer<T> gdzie T dziedziczy po IAssetInfo → GetAssetUserAsRaw
    for P in Params:
        CleanP = re.sub(r"\bPlu::", "", _StripQualifiers(P.Type)).strip()
        MUP = _RE_USE_POINTER.match(CleanP) or _RE_OWNING_POINTER.match(CleanP)
        if MUP and AllClasses:
            InnerT = MUP.group(1).strip()
            InnerTClean = re.sub(r"\bPlu::", "", InnerT).strip()
            InnerTypeInfo = next((C for C in AllClasses if C.Name == InnerTClean), None)
            if InnerTypeInfo and IsTypeDerivedFrom("IAssetInfo", InnerTypeInfo, AllClasses):
                return True
    return False

def _StripQualifiers(CppType: str) -> str:
    """Usuwa const, &, * z typu C++ żeby uzyskać czysty identyfikator."""
    return re.sub(r"\bconst\b", "", CppType).replace("&", "").replace("*", "").strip()

def _NeedsGlmLambda(Params: List[ParamInfo], ReturnType: str) -> bool:
    """Zwraca True jeśli funkcja ma parametr lub return typu glm – wymaga lambdy konwertera."""
    for P in Params:
        if _StripQualifiers(P.Type) in GLM_TYPE_MAP:
            return True
    if _StripQualifiers(ReturnType) in GLM_TYPE_MAP:
        return True
    return False

def _BuildParamList(Params: List[ParamInfo], SelfDecl: str, AllClasses: List = []) -> tuple:
    """
    Buduje listy parametrów lambdy, wywołań i rozpakowań.
    Obsługuje: TClassPointer<T> → py::object, glm → tuple,
               TUsePointer<T> gdzie T:IAssetInfo → T* + GetAssetUserAsRaw, reszta bez zmian.
    """
    LambdaParams: List[str] = ([SelfDecl] if SelfDecl else [])
    BindingCalls: List[str] = []
    Unpacks:      List[str] = []
    TupleIdx = 0

    for I, P in enumerate(Params):
        Raw     = P.Type.strip()
        Clean   = _StripQualifiers(Raw)
        # Usuń też namespace Plu:: żeby regex matchował
        CleanNoNs = re.sub(r"\bPlu::", "", Clean).strip()
        ArgName = P.Name if P.Name else f"arg{I}"

        # TUsePointer<T> gdzie T dziedziczy po IAssetInfo → parametr jako T*, owija GetAssetUserAsRaw
        MUP = _RE_USE_POINTER.match(CleanNoNs) or _RE_OWNING_POINTER.match(CleanNoNs)
        if MUP:
            InnerT = MUP.group(1).strip()
            InnerTClean = re.sub(r"\bPlu::", "", InnerT).strip()
            # Sprawdź czy InnerT dziedziczy po IAssetInfo
            InnerTypeInfo = next((C for C in AllClasses if C.Name == InnerTClean), None)
            IsAsset = InnerTypeInfo is not None and IsTypeDerivedFrom("IAssetInfo", InnerTypeInfo, AllClasses)
        else:
            IsAsset = False

        # Matchuj TClassPointer po oczyszczonym typie (obsługa const T&)
        MCP = _RE_CLASS_POINTER.match(CleanNoNs)
        if MUP and IsAsset:
            # Python przekazuje IAssetInfo* (nie T*) – nie trzeba .__class__ po stronie Pythona
            LambdaParams.append(f"IAssetInfo* {ArgName}")
            BindingCalls.append(f"GetAssetUserAsRaw({ArgName})")
        elif MCP:
            # TClassPointer<T> → py::object zawierający klasę Pythona
            # Wyciągamy TypeInfo* przez TypeRegistry używając __name__
            LambdaParams.append(f"py::object {ArgName}_pytype")
            Unpacks.append(f"std::string {ArgName}_name = pybind11::str({ArgName}_pytype.attr(\"__name__\"));")
            Unpacks.append(f"TypeInfo* {ArgName} = TypeRegistry::GetInstance()->GetTypeOfName({ArgName}_name.c_str());")
            BindingCalls.append(ArgName)
        elif Clean in GLM_TYPE_MAP:
            GlmType, _, Construct, TupleType = GLM_TYPE_MAP[Clean]
            TupleName = f"t{TupleIdx}" if TupleIdx > 0 else "t"
            TupleIdx += 1
            VarNames = re.findall(r"\b([a-z])\b", Construct)
            LambdaParams.append(f"{TupleType} {TupleName}")
            Unpacks.append(f"auto [{', '.join(VarNames)}] = {TupleName};")
            BindingCalls.append(f"{GlmType}{{{', '.join(VarNames)}}}")
        else:
            LambdaParams.append(f"{Raw} {ArgName}")
            BindingCalls.append(ArgName)

    return LambdaParams, BindingCalls, Unpacks


def _BuildReturnLine(ReturnType: str, CallExpr: str) -> tuple:
    """
    Buduje linię return z konwersją glm → tuple i TUsePointer/TOwningPointer → raw ptr.
    Zwraca (line: str, needs_reference: bool).
    needs_reference=True → dodaj py::return_value_policy::reference żeby C++ rządził lifetime.
    """
    CleanRet   = _StripQualifiers(ReturnType)
    CleanRetNs = re.sub(r"\bPlu::", "", CleanRet).strip()
    if CleanRet in GLM_TYPE_MAP or CleanRetNs in GLM_TYPE_MAP:
        Key = CleanRet if CleanRet in GLM_TYPE_MAP else CleanRetNs
        GlmType, _, Construct, TupleType = GLM_TYPE_MAP[Key]
        VarNames = re.findall(r"\b([a-z])\b", Construct)
        return f"auto _r = {CallExpr}; return {TupleType}{{{', '.join(f'_r.{v}' for v in VarNames)}}};", False
    elif ReturnType.strip() == "void":
        return f"{CallExpr};", False
    elif _RE_USE_POINTER.match(CleanRetNs) or _RE_OWNING_POINTER.match(CleanRetNs):
        # Zwracamy surowy wskaźnik ale C++ nadal rządzi pamięcią – reference policy
        return f"return {CallExpr}.GetRaw();", True
    else:
        return f"return {CallExpr};", False


def _BuildGlmLambda(ClsName: str, FuncName: str, Params: List[ParamInfo],
                    ReturnType: str, IsConst: bool, AllClasses: List = []) -> tuple:
    """Generuje lambdę C++ opakowującą metodę. Zwraca (lambda_str, needs_reference)."""
    ConstRef = "const " if IsConst else ""
    SelfDecl = f"{ConstRef}{ClsName}& self"
    LambdaParams, BindingCalls, Unpacks = _BuildParamList(Params, SelfDecl, AllClasses)
    CallExpr             = f"self.{FuncName}({', '.join(BindingCalls)})"
    ReturnLine, NeedsRef = _BuildReturnLine(ReturnType, CallExpr)
    Body                 = (" ".join(Unpacks) + " " if Unpacks else "") + ReturnLine
    return f"[]({', '.join(LambdaParams)}) {{ {Body} }}", NeedsRef


def _BuildGlmLambdaGlobal(FuncName: str, Params: List[ParamInfo], ReturnType: str, AllClasses: List = []) -> tuple:
    """Generuje lambdę C++ dla funkcji globalnej. Zwraca (lambda_str, needs_reference)."""
    LambdaParams, BindingCalls, Unpacks = _BuildParamList(Params, "", AllClasses)
    CallExpr             = f"{FuncName}({', '.join(BindingCalls)})"
    ReturnLine, NeedsRef = _BuildReturnLine(ReturnType, CallExpr)
    Body                 = (" ".join(Unpacks) + " " if Unpacks else "") + ReturnLine
    return f"[]({', '.join(LambdaParams)}) {{ {Body} }}", NeedsRef

def _BuildPySignature(Params: List[ParamInfo]) -> str:
    """Buduje string argumentów pybind11 py::arg("name") dla funkcji."""
    return ", ".join(f'py::arg("{P.Name}")' for P in Params if P.Name)


def _FuncPyCallArgs(Params: List[ParamInfo]) -> str:
    """Lista nazw parametrów do przekazania w lambdzie C++."""
    return ", ".join(P.Name if P.Name else f"arg{I}" for I, P in enumerate(Params))


def _CollectAllOverrideFns(Cls: TypeInfo, AllTypes: List[TypeInfo],
                           ExportedTypes: List[TypeInfo] = []) -> List[FunctionInfo]:
    """
    Zbiera wszystkie funkcje z PyOverride z całej hierarchii dziedziczenia (Cls + bazy).
    Szuka najpierw w ExportedTypes (mają pełne Functions z parsowania),
    potem w AllTypes (ClassList.txt – Functions puste, ale służą do nawigacji hierarchii).
    Funkcje z klas pochodnych mają priorytet – nie duplikujemy po nazwie.
    """
    # Połącz oba źródła – ExportedTypes ma pierwszeństwo (pełne dane)
    AllSources = {T.Name: T for T in AllTypes}
    AllSources.update({T.Name: T for T in ExportedTypes})

    NameToFunc: dict = {}

    def Collect(T: TypeInfo):
        for BaseName in T.Bases:
            BaseType = AllSources.get(BaseName)
            if BaseType:
                Collect(BaseType)
        for F in T.Functions:
            if HasPyParam(F.MacroParams, "PyOverride"):
                NameToFunc[F.Name] = F

    Collect(Cls)
    return list(NameToFunc.values())


def GeneratePybindBindings(Data: List[FileData], AllClasses: List[TypeInfo] = []):
    """
    Generuje:
      ReflectionCache/PluEngineBindings.cpp  – kod pybind11
      ReflectionCache/PluEngine.pyi          – stuby Pythona
    """

    # Zbierz wszystkie typy z PyExport
    ExportedTypes: List[TypeInfo] = []
    for FileEntry in Data:
        for Cls in FileEntry.Children:
            if Cls.Type == ClassType.INTERFACE:
                continue
            if IsPyExported(Cls):
                ExportedTypes.append(Cls)

    # Sortuj topologicznie – bazy muszą być zarejestrowane przed klasami pochodnymi
    def TopoSort(Types: List[TypeInfo]) -> List[TypeInfo]:
        NameToType = {T.Name: T for T in Types}
        Visited:    set = set()
        Result:     List[TypeInfo] = []

        def Visit(T: TypeInfo):
            if T.Name in Visited:
                return
            Visited.add(T.Name)
            # Najpierw odwiedź wszystkie bazy które są w ExportedTypes
            for Base in T.Bases:
                if Base in NameToType:
                    Visit(NameToType[Base])
            Result.append(T)

        for T in Types:
            Visit(T)
        return Result

    ExportedTypes = TopoSort(ExportedTypes)

    # Zbierz funkcje globalne (wszystkie PLU_FUNCTION poza klasą, bez PyNotCallable)
    AllGlobalFuncs: List[GlobalFunctionInfo] = []
    for FileEntry in Data:
        for GF in FileEntry.GlobalFunctions:
            if not HasPyParam(GF.MacroParams, "PyNotCallable"):
                AllGlobalFuncs.append(GF)

    if not ExportedTypes and not AllGlobalFuncs:
        print("Bindings: nothing to export – nothing generated.")
        return

    os.makedirs(OutputDir, exist_ok=True)
    BindingsCpp = os.path.join(OutputDir, "PluEngineBindings.cpp")
    StubPyi     = os.path.join(OutputDir, "PluEngine.pyi")

    # ── PluEngineBindings.cpp ─────────────────────────────────────────
    with open(BindingsCpp, "w") as B:
        B.write("// AUTO-GENERATED by ReflectionGenerator – DO NOT EDIT\n")
        B.write("// pybind11 bindings for PluEngine\n\n")
        B.write('#include <pybind11/embed.h>\n')
        B.write('#include <pybind11/stl.h>\n')
        B.write('#include <pybind11/operators.h>\n\n')
        B.write("namespace py = pybind11;\n\n")

        # Includes per plik źródłowy
        IncludedFiles: set = set()
        for Cls in ExportedTypes:
            FP = str(Cls.FilePath).replace("\\", "/")
            if FP not in IncludedFiles:
                B.write(f'#include "{FP}"\n')
                IncludedFiles.add(FP)
        for GF in AllGlobalFuncs:
            FP = str(GF.FilePath).replace("\\", "/")
            if FP not in IncludedFiles:
                B.write(f'#include "{FP}"\n')
                IncludedFiles.add(FP)

        B.write("\nusing namespace Plu;\n\n")

        # ── Trampoline klasy dla PyDerive + PyOverride ─────────────────
        for Cls in ExportedTypes:
            IsDerive    = HasPyParam(Cls.ReflectionParams, "PyDerive")
            OwnOverride = [F for F in Cls.Functions if HasPyParam(F.MacroParams, "PyOverride")]
            if not IsDerive:
                # Walidacja: PyOverride bez PyDerive → #pragma error
                for F in OwnOverride:
                    B.write(f'#pragma error "{Cls.Name}::{F.Name} has PyOverride but {Cls.Name} is missing PyDerive"\n')
                continue

            # Zbierz override'y z całej hierarchii (własne + odziedziczone)
            AllOverrideFns = _CollectAllOverrideFns(Cls, AllClasses, ExportedTypes)
            if not AllOverrideFns:
                continue

            TmpName = f"Py{Cls.Name}"
            B.write(f"// Trampoline for {Cls.Name}\n")
            B.write(f"struct {TmpName} : public {Cls.Name} {{\n")
            B.write(f"    using {Cls.Name}::{Cls.Name};\n\n")
            for F in AllOverrideFns:
                ParamDecl = ", ".join(
                    f"{P.Type} {P.Name if P.Name else f'arg{I}'}"
                    for I, P in enumerate(F.Params)
                )
                CallArgs  = ", ".join(P.Name if P.Name else f"arg{I}" for I, P in enumerate(F.Params))
                ConstQual = " const" if F.IsConst else ""
                B.write(f"    {F.ReturnType} {F.Name}({ParamDecl}){ConstQual} override {{\n")
                B.write(f"        PYBIND11_OVERRIDE(\n")
                B.write(f"            {F.ReturnType},\n")
                B.write(f"            {Cls.Name},\n")
                B.write(f"            {F.Name}")
                if CallArgs:
                    B.write(f",\n            {CallArgs}")
                B.write(f"\n        );\n")
                B.write(f"    }}\n\n")
            B.write(f"}};\n\n")

        B.write("PYBIND11_EMBEDDED_MODULE(PluEngine, m) {\n")
        B.write('    m.doc() = "PluEngine Python bindings";\n\n')

        for Cls in ExportedTypes:
            PyName      = GetPyName(Cls)
            PyDoc       = GetPyDoc(Cls.ReflectionParams)
            IsAbstr     = HasPyParam(Cls.ReflectionParams, "Abstract")
            IsDerive    = HasPyParam(Cls.ReflectionParams, "PyDerive")
            # Własne PyOverride – do .def rejestracji
            OwnOverrideFns = [F for F in Cls.Functions if HasPyParam(F.MacroParams, "PyOverride")]
            # Wszystkie PyOverride z hierarchii – decyduje o TmpName
            AllOverrideFns = _CollectAllOverrideFns(Cls, AllClasses, ExportedTypes) if IsDerive else []
            TmpName     = f"Py{Cls.Name}" if (IsDerive and AllOverrideFns) else ""

            # Bazowa klasa
            BaseDecl = ""
            if Cls.Bases:
                FirstBase = Cls.Bases[0]
                if any(C.Name == FirstBase and IsPyExported(C) for C in ExportedTypes):
                    BaseDecl = f", {FirstBase}"

            # Dodatkowe opcje py::class_
            ClassOpts = ""
            if IsDerive:
                ClassOpts += ", py::dynamic_attr()"

            ClsTemplate = f"{Cls.Name}, {TmpName}{BaseDecl}" if TmpName else f"{Cls.Name}{BaseDecl}"

            B.write(f'    // {Cls.Type.name}: {Cls.Name}\n')
            B.write(f'    py::class_<{ClsTemplate}>(m, "{PyName}"')
            if PyDoc:
                B.write(f', "{PyDoc}"')
            B.write(f'{ClassOpts})\n')

            if not IsAbstr:
                B.write(f'        .def(py::init<>())\n')

            # Pola z PyExport
            PyProps = [P for P in Cls.Properties if IsPyExported(P)]
            for Prop in PyProps:
                PropDoc  = GetPyDoc(Prop.Params)
                ReadOnly = HasPyParam(Prop.Params, "PyReadOnly")
                # Sprawdź czy typ to TClassPointer<T> – wymaga custom settera przez TypeRegistry
                CleanPropType = re.sub(r"\bPlu::", "", _StripQualifiers(Prop.Type)).strip()
                MCP = re.match(r"^(?:Plu::)?TClassPointer\s*<(.+)>$", CleanPropType)
                if MCP:
                    # def_property z getterem zwracającym pole i setterem przez TypeRegistry
                    Getter = f"[](const {Cls.Name}& self) {{ return self.{Prop.Name}; }}"
                    if ReadOnly:
                        B.write(f'        .def_property_readonly("{Prop.Name}", {Getter}')
                    else:
                        Setter = (
                            f"[]({Cls.Name}& self, py::object _pytype) {{ "
                            f"std::string _name = pybind11::str(_pytype.attr(\"__name__\")); "
                            f"self.{Prop.Name} = TypeRegistry::GetInstance()->GetTypeOfName(_name.c_str()); "
                            f"}}"
                        )
                        B.write(f'        .def_property("{Prop.Name}", {Getter}, {Setter}')
                    if PropDoc:
                        B.write(f', "{PropDoc}"')
                    B.write(")\n")
                else:
                    Accessor = "def_readonly" if ReadOnly else "def_readwrite"
                    B.write(f'        .{Accessor}("{Prop.Name}", &{Cls.Name}::{Prop.Name}')
                    if PropDoc:
                        B.write(f', "{PropDoc}"')
                    B.write(")\n")

            # Metody – wszystkie PLU_FUNCTION bez PyNotCallable
            for F in Cls.Functions:
                if HasPyParam(F.MacroParams, "PyNotCallable"):
                    continue
                if HasPyParam(F.MacroParams, "PyOverride"):
                    continue  # obsłużone przez trampoline – def nadal potrzebne
                ArgList = _BuildPySignature(F.Params)
                if _NeedsLambda(F.Params, F.ReturnType, AllClasses):
                    Callable, NeedsRef = _BuildGlmLambda(Cls.Name, F.Name, F.Params, F.ReturnType, F.IsConst, AllClasses)
                else:
                    Callable, NeedsRef = f"&{Cls.Name}::{F.Name}", False
                B.write(f'        .def("{F.Name}", {Callable}')
                if ArgList:
                    B.write(f', {ArgList}')
                if NeedsRef:
                    B.write(', py::return_value_policy::reference')
                B.write(")\n")

            # PyOverride metody też potrzebują .def dla wywołania z C++
            for F in OwnOverrideFns:
                ArgList = _BuildPySignature(F.Params)
                if _NeedsLambda(F.Params, F.ReturnType, AllClasses):
                    Callable, NeedsRef = _BuildGlmLambda(Cls.Name, F.Name, F.Params, F.ReturnType, F.IsConst, AllClasses)
                else:
                    Callable, NeedsRef = f"&{Cls.Name}::{F.Name}", False
                B.write(f'        .def("{F.Name}", {Callable}')
                if ArgList:
                    B.write(f', {ArgList}')
                if NeedsRef:
                    B.write(', py::return_value_policy::reference')
                B.write(")\n")

            B.write(f'        ;\n\n')

        # ── Funkcje globalne ───────────────────────────────────────────
        if AllGlobalFuncs:
            B.write("    // Global functions\n")
        for GF in AllGlobalFuncs:
            ArgList = _BuildPySignature(GF.Params)
            if _NeedsLambda(GF.Params, GF.ReturnType, AllClasses):
                Callable, NeedsRef = _BuildGlmLambdaGlobal(GF.Name, GF.Params, GF.ReturnType, AllClasses)
            else:
                Callable, NeedsRef = f"&{GF.Name}", False
            B.write(f'    m.def("{GF.Name}", {Callable}')
            if ArgList:
                B.write(f', {ArgList}')
            if NeedsRef:
                B.write(', py::return_value_policy::reference')
            FDoc = GetPyDoc(GF.MacroParams)
            if FDoc:
                B.write(f', "{FDoc}"')
            B.write(");\n")

        B.write("}\n")

    print(f"Generated {BindingsCpp} ({len(ExportedTypes)} type(s), {len(AllGlobalFuncs)} global func(s))")

    # ── PluEngine.pyi (stuby) ─────────────────────────────────────────
    with open(StubPyi, "w") as P:
        P.write("# AUTO-GENERATED by ReflectionGenerator – DO NOT EDIT\n")
        P.write("# Python type stubs for PluEngine\n\n")
        P.write("from __future__ import annotations\n")
        P.write("from typing import Dict, List, Optional, Tuple, Type, overload\n\n")

        def WriteFuncStub(indent: str, F, ClsName: str = ""):
            """Zapisuje stub metody lub funkcji globalnej."""
            if HasPyParam(F.MacroParams, "PyNotCallable"):
                return
            RetPy  = CppTypeToPy(F.ReturnType)
            IsOver = HasPyParam(F.MacroParams, "PyOverride")
            # Buduj listę argumentów
            PyArgs = []
            for I, Param in enumerate(F.Params):
                ArgName = Param.Name if Param.Name else f"arg{I}"
                # TClassPointer<T> → Type[T]
                ArgType = Param.Type.strip()
                M = re.match(r"(?:Plu::)?TClassPointer\s*<(.+)>", ArgType)
                if M:
                    InnerT = CppTypeToPy(M.group(1))
                    ArgType = f"Type[{InnerT}]"
                else:
                    # TUsePointer<T>/TOwningPointer<T> gdzie T:IAssetInfo → IAssetInfo w stubbie
                    CleanArg = re.sub(r"\bPlu::", "", _StripQualifiers(ArgType)).strip()
                    MUP = _RE_USE_POINTER.match(CleanArg) or _RE_OWNING_POINTER.match(CleanArg)
                    if MUP and AllClasses:
                        InnerT = re.sub(r"\bPlu::", "", MUP.group(1)).strip()
                        InnerTypeInfo = next((C for C in AllClasses if C.Name == InnerT), None)
                        if InnerTypeInfo and IsTypeDerivedFrom("IAssetInfo", InnerTypeInfo, AllClasses):
                            ArgType = "IAssetInfo"
                        else:
                            ArgType = CppTypeToPy(ArgType)
                    else:
                        ArgType = CppTypeToPy(ArgType)
                PyArgs.append(f"{ArgName}: {ArgType}")
            ArgStr = ", ".join(PyArgs)
            SelfStr = "self, " if ClsName else ""
            if IsOver:
                P.write(f"{indent}def {F.Name}(self, {ArgStr}) -> {RetPy}: ...\n")
            else:
                P.write(f"{indent}def {F.Name}({SelfStr}{ArgStr}) -> {RetPy}: ...\n")

        for Cls in ExportedTypes:
            PyName   = GetPyName(Cls)
            PyDoc    = GetPyDoc(Cls.ReflectionParams)
            IsAbstr  = HasPyParam(Cls.ReflectionParams, "Abstract")
            IsDerive = HasPyParam(Cls.ReflectionParams, "PyDerive")

            BaseDecl = ""
            if Cls.Bases:
                FirstBase = Cls.Bases[0]
                if any(C.Name == FirstBase and IsPyExported(C) for C in ExportedTypes):
                    BaseDecl = GetPyName(next(C for C in ExportedTypes if C.Name == FirstBase))
            PyClass = f"class {PyName}({BaseDecl}):" if BaseDecl else f"class {PyName}:"
            P.write(f"{PyClass}\n")
            if PyDoc or IsDerive:
                DocLines = []
                if PyDoc:
                    DocLines.append(PyDoc)
                if IsDerive:
                    DocLines.append("Subclassable from Python (PyDerive).")
                P.write(f'    """{" ".join(DocLines)}"""\n')

            HasContent = False

            if not IsAbstr:
                P.write("    def __init__(self) -> None: ...\n")
                HasContent = True

            # Pola z PyExport
            for Prop in Cls.Properties:
                if not IsPyExported(Prop):
                    continue
                HasContent = True
                PropDoc  = GetPyDoc(Prop.Params)
                PyType   = CppTypeToPy(Prop.Type)
                ReadOnly = HasPyParam(Prop.Params, "PyReadOnly")
                if ReadOnly:
                    P.write(f"    @property\n")
                    P.write(f"    def {Prop.Name}(self) -> {PyType}:\n")
                    if PropDoc:
                        P.write(f'        """{PropDoc}"""\n')
                    P.write(f"        ...\n")
                else:
                    P.write(f"    {Prop.Name}: {PyType}")
                    if PropDoc:
                        P.write(f"  # {PropDoc}")
                    P.write("\n")

            # Metody (bez PyNotCallable)
            VisibleFuncs = [F for F in Cls.Functions if not HasPyParam(F.MacroParams, "PyNotCallable")]
            for F in VisibleFuncs:
                HasContent = True
                WriteFuncStub("    ", F, ClsName=Cls.Name)

            if not HasContent:
                P.write("    ...\n")
            P.write("\n")

        # Funkcje globalne
        if AllGlobalFuncs:
            P.write("# ── Global functions ──────────────────────────────────────\n")
        for GF in AllGlobalFuncs:
            WriteFuncStub("", GF)
        if AllGlobalFuncs:
            P.write("\n")

    print(f"Generated {StubPyi} ({len(ExportedTypes)} type(s), {len(AllGlobalFuncs)} global func(s))")


def GenerateReflectionData(Data: List[FileData]):
    if not Data:
        print("No reflection changes.")
        return

    def GetFileProject(F: FileData) -> str:
        """Wyciąga nazwę projektu z FileData – działa też gdy Children jest puste (tylko globalne funkcje)."""
        if F.Children:
            return F.Children[0].Project
        # Fallback: wyznacz projekt ze ścieżki pliku (pierwszy segment względem korzenia)
        try:
            Relative = F.FilePath.relative_to(Path(os.path.dirname(ScriptDir)))
            return Relative.parts[0]
        except ValueError:
            return "Unknown"

    print(f"Generating reflection data for {len(Data)} file(s)...")

    # ── Zbierz projekty ───────────────────────────────────────────────
    ProjectGroups: List[str] = []
    for F in Data:
        Proj = GetFileProject(F)
        if Proj not in ProjectGroups:
            ProjectGroups.append(Proj)

    if ForceMode:
        for Proj in ProjectGroups:
            ClassListPath = os.path.join(OutputDir, Proj, "ClassList.txt")
            if os.path.exists(ClassListPath):
                os.remove(ClassListPath)

    # ── Zapisz ClassList.txt ──────────────────────────────────────────
    for F in Data:
        if not F.Children:
            continue  # brak klas – pomijamy ClassList
        Proj            = GetFileProject(F)
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
        if not FileEntry.Children:
            continue  # plik zawiera tylko globalne funkcje, brak klas do wygenerowania
        FilePath    = FileEntry.FilePath
        Proj        = GetFileProject(FileEntry)
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

                if IsTypeDerivedFrom("EngineObject", Cls, AllClasses):
                    S.write(f"TypeInfo* {Cls.Name}::GetClass() {{ return GetPythonType() ? GetPythonType() : GetStaticClass(); }}\n\n")
                else:
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
        if not os.path.exists(ClassListFile):
            continue  # projekt zawiera tylko globalne funkcje, brak klas
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
    return AllClasses


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

        Types, GlobalFuncs = ProcessFile(FilePath, ProjectName)
        if Types or GlobalFuncs:
            if Types:
                ParamsCheck(Types)
            AllReflectionData.append(FileData(FilePath=FilePath, Children=Types, GlobalFunctions=GlobalFuncs))
            for T in Types:
                print(f"  Found {T.Type.name}: {T.Name} in {FileName} (project: {T.Project})")
            for GF in GlobalFuncs:
                print(f"  Found GLOBAL FUNC: {GF.ReturnType} {GF.Name}(...) in {FileName}")

    ReflAllClasses = GenerateReflectionData(AllReflectionData)
    if BindingsMode:
        GeneratePybindBindings(AllReflectionData, ReflAllClasses or [])